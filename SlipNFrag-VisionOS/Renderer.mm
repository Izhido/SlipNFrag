//
//  Renderer.mm
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 30/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import "Renderer.h"
#import <Spatial/Spatial.h>
#import <ARKit/ARKit.h>
#include "sys_visionos.h"
#include "vid_visionos.h"
#include "in_visionos.h"
#include "Locks.h"
#include "DirectRect.h"
#import "PerDrawable.h"
#include "AppState.h"
#include "r_local.h"
#include "d_lists.h"
#include "Input.h"
#import "Pipelines.h"
#import "Sky.h"
#import "Skybox.h"
#import "Surfaces.h"
#import "SurfacesRotated.h"
#import "Fences.h"
#import "FencesRotated.h"
#import "TurbulentLit.h"
#import "Turbulent.h"
#import "Alias.h"
#import "Viewmodel.h"
#import "Sprites.h"
#import "Particles.h"

@interface Renderer ()

@end

@implementation Renderer

static std::vector<unsigned> vid_converted;
static CGDataProviderRef vid_provider;

static std::vector<unsigned> con_converted;
static CGDataProviderRef con_provider;

-(void)startRenderer:(CP_OBJECT_cp_layer_renderer*)layerRenderer engineStop:(EngineStop*)engineStop
{
	while (!host_initialized && !engineStop.stopEngine)
	{
		[NSThread sleepForTimeInterval:0];
	}
	
	auto floors = [NSMutableDictionary<NSString*, ar_plane_anchor_t> new];
	
	auto session = ar_session_create();
	
	ar_world_tracking_configuration_t world_tracking_config = ar_world_tracking_configuration_create();
	auto world_tracking = ar_world_tracking_provider_create(world_tracking_config);
	
	auto plane_detection_config = ar_plane_detection_configuration_create();
	ar_plane_detection_configuration_set_alignment(plane_detection_config, ar_plane_alignment_horizontal);
	auto plane_detection = ar_plane_detection_provider_create(plane_detection_config);
	ar_plane_detection_provider_set_update_handler(plane_detection, NULL,
												   ^(ar_plane_anchors_t _Nonnull added_anchors,
													 ar_plane_anchors_t _Nonnull updated_anchors,
													 ar_plane_anchors_t _Nonnull removed_anchors) {
		std::lock_guard<std::mutex> lock(Locks::RenderInputMutex);
		
		ar_plane_anchors_enumerate_anchors(added_anchors,
										   ^bool(ar_plane_anchor_t _Nonnull plane_anchor) {
			auto classification = ar_plane_anchor_get_plane_classification(plane_anchor);
			if (classification == ar_plane_classification_floor)
			{
				unsigned char identifier[64] { };
				ar_anchor_get_identifier(plane_anchor, identifier);
				auto identifierAsString = [NSString stringWithCString:(const char*)identifier encoding:NSString.defaultCStringEncoding];
				[floors setObject:plane_anchor forKey:identifierAsString];
			}
			return true;
		});
		
		ar_plane_anchors_enumerate_anchors(updated_anchors,
										   ^bool(ar_plane_anchor_t _Nonnull plane_anchor) {
			auto classification = ar_plane_anchor_get_plane_classification(plane_anchor);
			if (classification == ar_plane_classification_floor)
			{
				unsigned char identifier[64] { };
				ar_anchor_get_identifier(plane_anchor, identifier);
				auto identifierAsString = [NSString stringWithCString:(const char*)identifier encoding:NSString.defaultCStringEncoding];
				[floors setObject:plane_anchor forKey:identifierAsString];
			}
			return true;
		});
		
		ar_plane_anchors_enumerate_anchors(removed_anchors,
										   ^bool(ar_plane_anchor_t _Nonnull plane_anchor) {
			auto classification = ar_plane_anchor_get_plane_classification(plane_anchor);
			if (classification == ar_plane_classification_floor)
			{
				unsigned char identifier[64] { };
				ar_anchor_get_identifier(plane_anchor, identifier);
				auto identifierAsString = [NSString stringWithCString:(const char*)identifier encoding:NSString.defaultCStringEncoding];
				[floors removeObjectForKey:identifierAsString];
			}
			return true;
		});
	});
	
	auto providers = ar_data_providers_create();
	ar_data_providers_add_data_provider(providers, world_tracking);
	
	ar_session_request_authorization(session, ar_authorization_type_world_sensing,
									 ^(ar_authorization_results_t _Nonnull authorization_results,
									   ar_error_t _Nullable error) {
		ar_error_code_t error_code = 0;
		if (error != nil)
		{
			error_code = ar_error_get_error_code(error);
		}
		if (error_code == 0)
		{
			ar_authorization_results_enumerate_results(authorization_results, ^bool(ar_authorization_result_t _Nonnull authorization_result) {
				auto result = ar_authorization_result_get_status(authorization_result);
				if (result == ar_authorization_status_allowed)
				{
					ar_data_providers_add_data_provider(providers, plane_detection);
				}
				return true;
			});
		}
	});
	
	ar_session_run(session, providers);
	
	auto configuration = cp_layer_renderer_get_configuration(layerRenderer);
	auto colorPixelFormat = cp_layer_renderer_configuration_get_color_format(configuration);
	auto depthPixelFormat = cp_layer_renderer_configuration_get_depth_format(configuration);
	
	id<MTLDevice> device = MTLCreateSystemDefaultDevice();
	id<MTLCommandQueue> commandQueue = [device newCommandQueue];
	
	auto pipelines = [Pipelines new];
	if (![pipelines create:device colorPixelFormat:colorPixelFormat depthPixelFormat:depthPixelFormat engineStop:engineStop])
	{
		return;
	}
	
	auto depthStencilDescriptor = [MTLDepthStencilDescriptor new];
	depthStencilDescriptor.depthWriteEnabled = false;
	depthStencilDescriptor.depthCompareFunction = MTLCompareFunctionAlways;
	
	auto consoleDepthStencilState = [device newDepthStencilStateWithDescriptor:depthStencilDescriptor];
	
	depthStencilDescriptor.depthWriteEnabled = true;
	depthStencilDescriptor.depthCompareFunction = MTLCompareFunctionGreater;

	auto surfaceDepthStencilState = [device newDepthStencilStateWithDescriptor:depthStencilDescriptor];

	auto samplerDescriptor = [MTLSamplerDescriptor new];

	auto planarSamplerState = [device newSamplerStateWithDescriptor:samplerDescriptor];

	samplerDescriptor.sAddressMode = MTLSamplerAddressModeRepeat;
	samplerDescriptor.tAddressMode = MTLSamplerAddressModeRepeat;
	samplerDescriptor.mipFilter = MTLSamplerMipFilterNearest;

	auto textureSamplerState = [device newSamplerStateWithDescriptor:samplerDescriptor];

	samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
	samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
	samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
	samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
	samplerDescriptor.mipFilter = MTLSamplerMipFilterNotMipmapped;

	auto lightmapSamplerState = [device newSamplerStateWithDescriptor:samplerDescriptor];

	float screenPlaneVertices[] = {
		-1.6,  1,    -2, 1,  0, 0,
		1.6,   1,    -2, 1,  1, 0,
		-1.6, -1,    -2, 1,  0, 1,
		1.6,  -1,    -2, 1,  1, 1
	};
	
	id<MTLBuffer> screenPlane = [device newBufferWithBytes:screenPlaneVertices length:sizeof(screenPlaneVertices) options:0];
	
	float consoleTopPlaneVertices[] = {
		-1.6,  1,    -2, 1,  0, 0,
		1.6,   1,    -2, 1,  1, 0,
		-1.6, -0.52, -2, 1,  0, 0.76,
		1.6,  -0.52, -2, 1,  1, 0.76
	};
	
	id<MTLBuffer> consoleTopPlane = [device newBufferWithBytes:consoleTopPlaneVertices length:sizeof(consoleTopPlaneVertices) options:0];
	
	float consoleBottomPlaneVertices[] = {
		-1.6, -0.88, -2, 1,  0, 0.76,
		-0.8, -0.88, -2, 1,  1, 0.76,
		-1.6, -1,    -2, 1,  0, 1,
		-0.8, -1,    -2, 1,  1, 1
	};
	
	id<MTLBuffer> consoleBottomPlane = [device newBufferWithBytes:consoleBottomPlaneVertices length:sizeof(consoleBottomPlaneVertices) options:0];
	
	auto locked_head_pose = matrix_identity_float4x4;
	locked_head_pose.columns[3][2] = 0.5;

	auto drawables = [NSMutableArray<PerDrawable*> new];

	std::vector<std::unordered_map<void*, NSUInteger>> lightmapIndices;
	
	std::vector<std::vector<float>> rotations;
	
	std::unordered_map<void*, NSUInteger> textureIndex;
	auto textureCache = [NSMutableArray<Texture*> new];

	std::unordered_map<void*, NSUInteger> aliasIndicesIndex;
	auto aliasIndicesCache = [NSMutableArray<id<MTLBuffer>> new];

	std::unordered_map<void*, NSUInteger> colormapIndex;
	auto colormapCache = [NSMutableArray<Texture*> new];

	auto clearCount = 0;
	
	simd_float4x4 vertexTransformMatrix { };
	vertexTransformMatrix.columns[3][3] = 1;
	
	std::vector<unsigned char> skyBuffer;

	while (!engineStop.stopEngine) @autoreleasepool
	{
		switch (cp_layer_renderer_get_state(layerRenderer))
		{
			case cp_layer_renderer_state_paused:
				[NSThread sleepForTimeInterval:0];
				break;
				
			case cp_layer_renderer_state_running:
			{
				auto frame = cp_layer_renderer_query_next_frame(layerRenderer);
				if (frame != nullptr)
				{
					auto timing = cp_frame_predict_timing(frame);
					if (timing != nullptr)
					{
						cp_frame_start_update(frame);
						
						AppMode mode;
						{
							std::lock_guard<std::mutex> lock(Locks::ModeChangeMutex);

							if (cls.demoplayback || cl.intermission || con_forcedup || scr_disabled_for_loading)
							{
								appState.Mode = AppScreenMode;
							}
							else
							{
								appState.Mode = AppWorldMode;
							}

							if (appState.PreviousMode != appState.Mode)
							{
								if (appState.PreviousMode == AppStartupMode)
								{
									appState.DefaultFOV = (int)Cvar_VariableValue("fov");
									//r_load_as_rgba = true;
									d_skipfade = true;
								}

								if (appState.Mode == AppScreenMode)
								{
									std::lock_guard<std::mutex> lock(Locks::RenderMutex);

									D_ResetLists();
									d_uselists = false;
									r_skip_fov_check = false;
									sb_onconsole = false;
									Cvar_SetValue("joyadvanced", 1);
									Cvar_SetValue("joyadvaxisx", AxisSide);
									Cvar_SetValue("joyadvaxisy", AxisForward);
									Cvar_SetValue("joyadvaxisz", AxisTurn);
									Cvar_SetValue("joyadvaxisr", AxisLook);
									Joy_AdvancedUpdate_f();
									Cvar_SetValue("fov", appState.DefaultFOV);
									VID_Resize(320.0 / 240.0);
								}
								else if (appState.Mode == AppWorldMode)
								{
									std::lock_guard<std::mutex> lock(Locks::RenderMutex);

									D_ResetLists();
									d_uselists = true;
									r_skip_fov_check = true;
									sb_onconsole = true;
									Cvar_SetValue("joyadvanced", 1);
									Cvar_SetValue("joyadvaxisx", AxisSide);
									Cvar_SetValue("joyadvaxisy", AxisForward);
									Cvar_SetValue("joyadvaxisz", AxisNada);
									Cvar_SetValue("joyadvaxisr", AxisNada);
									Joy_AdvancedUpdate_f();

									// The following is to prevent having stuck arrow keys at transition time:
									Input::AddKeyInput(K_DOWNARROW, false);
									Input::AddKeyInput(K_UPARROW, false);
									Input::AddKeyInput(K_LEFTARROW, false);
									Input::AddKeyInput(K_RIGHTARROW, false);

									Cvar_SetValue("fov", appState.FOV);
									VID_Resize(1);
								}

								appState.PreviousMode = appState.Mode;
							}

							mode = appState.Mode;
						}

						if (clearCount != host_clearcount)
						{
							colormapIndex.clear();
							[colormapCache removeAllObjects];

							textureIndex.clear();
							[textureCache removeAllObjects];

							for (NSUInteger d = 0; d < drawables.count; d++)
							{
								rotations[d].clear();
								
								lightmapIndices[d].clear();

								[drawables[d].lightmapCache removeAllObjects];
								drawables[d].aliasVertices = nil;
								drawables[d].surfaceVertices = nil;
								drawables[d].skyboxVertices = nil;
								drawables[d].texturedVertices = nil;
								drawables[d].indices = nil;
							}
							
							clearCount = host_clearcount;
						}

						cp_frame_end_update(frame);
						
						cp_time_wait_until(cp_frame_timing_get_optimal_input_time(timing));
						
						cp_frame_start_submission(frame);
						auto drawable = cp_frame_query_drawable(frame);
						if (drawable != nullptr)
						{
							auto viewCount = (NSUInteger)cp_drawable_get_view_count(drawable);
							
							auto timing_info = cp_drawable_get_frame_timing(drawable);
							auto time = cp_time_to_cf_time_interval(cp_frame_timing_get_presentation_time(timing_info));
							auto device_anchor = ar_device_anchor_create();
							const ar_device_anchor_query_status_t status = ar_world_tracking_provider_query_device_anchor_at_timestamp(world_tracking, time, device_anchor);

							if (status == ar_device_anchor_query_status_success)
							{
								cp_drawable_set_device_anchor(drawable, device_anchor);
								
								auto head_pose = ar_anchor_get_origin_from_anchor_transform(device_anchor);

								{
									std::lock_guard<std::mutex> lock(Locks::RenderInputMutex);
									
									auto quaternion = simd_quaternion(head_pose);
									
									auto x = quaternion.vector[0];
									auto y = quaternion.vector[1];
									auto z = quaternion.vector[2];
									auto w = quaternion.vector[3];

									float Q[3] = { x, y, z };
									float ww = w * w;
									float Q11 = Q[1] * Q[1];
									float Q22 = Q[0] * Q[0];
									float Q33 = Q[2] * Q[2];
									const float psign = -1;
									float s2 = psign * 2 * (psign * w * Q[0] + Q[1] * Q[2]);
									const float singularityRadius = 1e-12;
									if (s2 < singularityRadius - 1)
									{
										appState.Yaw = 0;
										appState.Pitch = -M_PI / 2;
										appState.Roll = atan2(2 * (psign * Q[1] * Q[0] + w * Q[2]), ww + Q22 - Q11 - Q33);
									}
									else if (s2 > 1 - singularityRadius)
									{
										appState.Yaw = 0;
										appState.Pitch = M_PI / 2;
										appState.Roll = atan2(2 * (psign * Q[1] * Q[0] + w * Q[2]), ww + Q22 - Q11 - Q33);
									}
									else
									{
										appState.Yaw = -(atan2(-2 * (w * Q[1] - psign * Q[0] * Q[2]), ww + Q33 - Q11 - Q22));
										appState.Pitch = asin(s2);
										appState.Roll = atan2(2 * (w * Q[2] - psign * Q[1] * Q[0]), ww + Q11 - Q22 - Q33);
									}

									appState.PositionX = head_pose.columns[3][0];
									appState.PositionY = head_pose.columns[3][1];
									appState.PositionZ = head_pose.columns[3][2];

									auto playerHeight = 32;
									if (host_initialized && cl.viewentity >= 0 && cl.viewentity < cl_entities.size())
									{
										auto player = &cl_entities[cl.viewentity];
										if (player != nullptr)
										{
											auto model = player->model;
											if (model != nullptr)
											{
												playerHeight = model->maxs[1] - model->mins[1];
											}
										}
									}

									auto distanceToFloor = 1.6;
									for(NSString* floorKey in floors)
									{
										auto geometry = ar_plane_anchor_get_geometry([floors objectForKey:floorKey]);
										auto extent = ar_plane_geometry_get_plane_extent(geometry);
										auto transform = ar_plane_extent_get_plane_anchor_from_plane_extent_transform(extent);
										
										distanceToFloor = transform.columns[3][3];
										NSLog(@"Distance to floor: %f", distanceToFloor);

										// Stop at the first found key in floors:
										break;
									}

									appState.Scale = -distanceToFloor / playerHeight;

									float horizontalAngle = 0;
									for (NSUInteger v = 0; v < viewCount; v++)
									{
										cp_view_t view = cp_drawable_get_view(drawable, v);
										simd_float4 tangents = cp_view_get_tangents(view);
										
										horizontalAngle = std::max(horizontalAngle, atan(tangents[0]));
										horizontalAngle = std::max(horizontalAngle, atan(tangents[1]));
									}
									
									appState.FOV = 2 * horizontalAngle * 180 / M_PI;
								}
								
								PerDrawable* perDrawable = nil;
								std::unordered_map<void*, NSUInteger>* lightmapIndex = nullptr;
								std::vector<float>* rotation = nullptr;

								for (NSUInteger d = 0; d < drawables.count; d++)
								{
									if (drawables[d].drawable == drawable)
									{
										perDrawable = drawables[d];
										lightmapIndex = &lightmapIndices[d];
										rotation = &rotations[d];

										break;
									}
								}
								
								if (perDrawable == nil)
								{
									perDrawable = [PerDrawable new];
									perDrawable.drawable = drawable;
									perDrawable.lightmapCache = [NSMutableArray<Lightmap*> new];
									[drawables addObject:perDrawable];
									
									lightmapIndices.emplace_back();
									lightmapIndex = &lightmapIndices.back();
									
									rotations.emplace_back();
									rotation = &rotations.back();
									
									{
										std::lock_guard<std::mutex> lock(Locks::RenderMutex);
										
										perDrawable.palette = [Texture new];
										perDrawable.palette.descriptor = [MTLTextureDescriptor new];
										perDrawable.palette.descriptor.textureType = MTLTextureType1D;
										perDrawable.palette.descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm_sRGB;
										perDrawable.palette.descriptor.width = 256;
										perDrawable.palette.texture = [device newTextureWithDescriptor:perDrawable.palette.descriptor];
										perDrawable.palette.region = MTLRegionMake1D(0, 256);
										
										perDrawable.colormap = [Texture new];
										perDrawable.colormap.descriptor = [MTLTextureDescriptor new];
										perDrawable.colormap.descriptor.pixelFormat = MTLPixelFormatR8Unorm;
										perDrawable.colormap.descriptor.width = 256;
										perDrawable.colormap.descriptor.height = 64;
										perDrawable.colormap.texture = [device newTextureWithDescriptor:perDrawable.colormap.descriptor];
										perDrawable.colormap.region = MTLRegionMake2D(0, 0, 256, 64);
										
										perDrawable.screen = [Texture new];
										perDrawable.screen.descriptor = [MTLTextureDescriptor new];
										perDrawable.screen.descriptor.pixelFormat = MTLPixelFormatR8Unorm;
										perDrawable.screen.descriptor.width = vid_width;
										perDrawable.screen.descriptor.height = vid_height;
										perDrawable.screen.texture = [device newTextureWithDescriptor:perDrawable.screen.descriptor];
										perDrawable.screen.region = MTLRegionMake2D(0, 0, vid_width, vid_height);
										
										perDrawable.console = [Texture new];
										perDrawable.console.descriptor = [MTLTextureDescriptor new];
										perDrawable.console.descriptor.pixelFormat = MTLPixelFormatR8Unorm;
										perDrawable.console.descriptor.width = con_width;
										perDrawable.console.descriptor.height = con_height;
										perDrawable.console.texture = [device newTextureWithDescriptor:perDrawable.console.descriptor];
										perDrawable.console.region = MTLRegionMake2D(0, 0, con_width, con_height);
									}
								}
								
								if (perDrawable.views == nil)
								{
									perDrawable.views = [NSMutableDictionary<NSNumber*, PerView*> new];
								}

								std::vector<SkyEntry> sky;
								
								std::vector<SkyEntry> skybox;

								NSUInteger surfacesIndexBase = 0;
								std::unordered_map<void*, SortedSurfaceLightmap> sortedSurfaces;

								NSUInteger surfacesRotatedIndexBase = 0;
								std::unordered_map<void*, SortedSurfaceRotatedLightmap> sortedSurfacesRotated;

								NSUInteger fencesIndexBase = 0;
								std::unordered_map<void*, SortedSurfaceLightmap> sortedFences;

								NSUInteger fencesRotatedIndexBase = 0;
								std::unordered_map<void*, SortedSurfaceRotatedLightmap> sortedFencesRotated;

								NSUInteger turbulentLitIndexBase = 0;
								std::unordered_map<void*, SortedSurfaceLightmap> sortedTurbulentLit;
								
								NSUInteger turbulentIndexBase = 0;
								std::unordered_map<void*, SortedTurbulentTexture> sortedTurbulent;
								
								std::unordered_map<void*, SortedAliasTexture> sortedAlias;

								std::unordered_map<void*, SortedAliasTexture> sortedViewmodel;
								
								NSUInteger spritesIndexBase = 0;
								std::unordered_map<void*, SortedSpriteTexture> sortedSprites;

								NSUInteger particlesIndexBase = 0;
								std::vector<ParticlesEntry> particles;
								
								std::vector<LightmapCopying> lightmapCopying;

								if (mode == AppWorldMode)
								{
									std::lock_guard<std::mutex> lock(Locks::RenderMutex);

									[perDrawable.palette.texture replaceRegion:perDrawable.palette.region mipmapLevel:0 withBytes:d_8to24table bytesPerRow:perDrawable.palette.region.size.width * sizeof(unsigned)];
									[perDrawable.colormap.texture replaceRegion:perDrawable.colormap.region mipmapLevel:0 withBytes:host_colormap.data() bytesPerRow:perDrawable.colormap.region.size.width];
									[perDrawable.console.texture replaceRegion:perDrawable.console.region mipmapLevel:0 withBytes:con_buffer.data() bytesPerRow:perDrawable.console.region.size.width];

									{
										std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
										
										for (auto& directRect : DirectRect::directRects)
										{
											auto x = directRect.x * (perDrawable.console.region.size.width - directRect.width) / (directRect.vid_width - directRect.width);
											auto y = directRect.y * (perDrawable.console.region.size.height - directRect.width) / (directRect.vid_height - directRect.height);
											
											MTLRegion directRegion = MTLRegionMake2D(x, y, directRect.width, directRect.height);
											[perDrawable.console.texture replaceRegion:directRegion mipmapLevel:0 withBytes:directRect.data bytesPerRow:directRect.width];
										}
									}

									vertexTransformMatrix.columns[0][0] = -appState.Scale;
									vertexTransformMatrix.columns[1][2] = appState.Scale;
									vertexTransformMatrix.columns[2][1] = -appState.Scale;
									vertexTransformMatrix.columns[3][0] = d_lists.vieworg0 * appState.Scale;
									vertexTransformMatrix.columns[3][1] = d_lists.vieworg2 * appState.Scale;
									vertexTransformMatrix.columns[3][2] = -d_lists.vieworg1 * appState.Scale;
									
									NSUInteger indicesSize = 0;
									NSUInteger texturedVerticesSize = 0;
									NSUInteger skyboxVerticesSize = 0;
									NSUInteger surfacesVerticesSize = 0;
									NSUInteger aliasVerticesSize = 0;
									NSUInteger particlesVerticesSize = 0;

									Sky::Sort(sky, texturedVerticesSize, indicesSize);

									Skybox::Sort(skybox, skyboxVerticesSize, indicesSize);
									
									surfacesIndexBase = indicesSize;
									Surfaces::Sort(sortedSurfaces, surfacesVerticesSize, indicesSize);
									
									surfacesRotatedIndexBase = indicesSize;
									SurfacesRotated::Sort(sortedSurfacesRotated, surfacesVerticesSize, indicesSize);
									
									fencesIndexBase = indicesSize;
									Fences::Sort(sortedFences, surfacesVerticesSize, indicesSize);
									
									fencesRotatedIndexBase = indicesSize;
									FencesRotated::Sort(sortedFencesRotated, surfacesVerticesSize, indicesSize);
									
									turbulentLitIndexBase = indicesSize;
									TurbulentLit::Sort(sortedTurbulentLit, surfacesVerticesSize, indicesSize);
									
									turbulentIndexBase = indicesSize;
									Turbulent::Sort(sortedTurbulent, surfacesVerticesSize, indicesSize);
									
									Alias::Sort(sortedAlias, aliasVerticesSize);
									
									Viewmodel::Sort(sortedViewmodel, aliasVerticesSize);
									
									spritesIndexBase = indicesSize;
									Sprites::Sort(sortedSprites, texturedVerticesSize, indicesSize);
									
									particlesIndexBase = indicesSize;
									Particles::Sort(particles, particlesVerticesSize, indicesSize);

									if (indicesSize > 0 && (perDrawable.indices == nil || perDrawable.indices.length < indicesSize || perDrawable.indices.length > indicesSize * 2))
									{
										perDrawable.indices = [device newBufferWithLength:indicesSize * 3 / 2 options:MTLResourceCPUCacheModeWriteCombined];
									}
									
									if (texturedVerticesSize > 0 && (perDrawable.texturedVertices == nil || perDrawable.texturedVertices.length < texturedVerticesSize || perDrawable.texturedVertices.length > texturedVerticesSize * 2))
									{
										perDrawable.texturedVertices = [device newBufferWithLength:texturedVerticesSize * 3 / 2 options:MTLResourceCPUCacheModeWriteCombined];
									}

									if (skyboxVerticesSize > 0 && (perDrawable.skyboxVertices == nil || perDrawable.skyboxVertices.length < skyboxVerticesSize || perDrawable.skyboxVertices.length > skyboxVerticesSize * 2))
									{
										perDrawable.skyboxVertices = [device newBufferWithLength:skyboxVerticesSize * 3 / 2 options:MTLResourceCPUCacheModeWriteCombined];
									}

									if (surfacesVerticesSize > 0 && (perDrawable.surfaceVertices == nil || perDrawable.surfaceVertices.length < surfacesVerticesSize || perDrawable.surfaceVertices.length > surfacesVerticesSize * 2))
									{
										perDrawable.surfaceVertices = [device newBufferWithLength:surfacesVerticesSize * 3 / 2 options:MTLResourceCPUCacheModeWriteCombined];
									}

									if (aliasVerticesSize > 0 && (perDrawable.aliasVertices == nil || perDrawable.aliasVertices.length < aliasVerticesSize || perDrawable.aliasVertices.length > aliasVerticesSize * 2))
									{
										perDrawable.aliasVertices = [device newBufferWithLength:aliasVerticesSize * 3 / 2 options:MTLResourceCPUCacheModeWriteCombined];
									}
									
									if (particlesVerticesSize > 0 && (perDrawable.particleVertices == nil || perDrawable.particleVertices.length < particlesVerticesSize || perDrawable.particleVertices.length > particlesVerticesSize * 2))
									{
										perDrawable.particleVertices = [device newBufferWithLength:particlesVerticesSize * 3 / 2 options:MTLResourceCPUCacheModeWriteCombined];
									}

									if (indicesSize > 0)
									{
										auto indicesTarget = (uint32_t*)perDrawable.indices.contents;
										
										float* texturedVerticesTarget = nullptr;
										uint32_t texturedIndexBase = 0;
										
										if (texturedVerticesSize > 0)
										{
											texturedVerticesTarget = (float*)perDrawable.texturedVertices.contents;
											
											Sky::Fill(sky, texturedVerticesTarget, indicesTarget, texturedIndexBase, device, perDrawable, skyBuffer, textureIndex, textureCache);
										}
										
										if (skyboxVerticesSize > 0)
										{
											auto verticesTarget = (float*)perDrawable.skyboxVertices.contents;
											
											Skybox::Fill(skybox, verticesTarget, indicesTarget, device, perDrawable, textureIndex, textureCache);
										}
										
										if (surfacesVerticesSize > 0)
										{
											auto verticesTarget = (float*)perDrawable.surfaceVertices.contents;
											
											uint32_t indexBase = 0;
											
											NSUInteger lightmapBufferSize = 0;
											
											Surfaces::Fill(sortedSurfaces, verticesTarget, indicesTarget, indexBase, lightmapIndex, lightmapBufferSize, lightmapCopying, device, perDrawable, textureIndex, textureCache);
											
											size_t rotationSize = (d_lists.last_surface_rotated + 1 + d_lists.last_fence_rotated + 1) * 8;
											
											if (rotationSize > rotation->size() || rotationSize < rotation->size() / 2)
											{
												rotation->resize(rotationSize * 3 / 2);
											}
											
											auto rotationData = rotation->data();
											
											SurfacesRotated::Fill(sortedSurfacesRotated, verticesTarget, indicesTarget, rotationData, indexBase, lightmapIndex, lightmapBufferSize, lightmapCopying, device, perDrawable, textureIndex, textureCache);
											
											Fences::Fill(sortedFences, verticesTarget, indicesTarget, indexBase, lightmapIndex, lightmapBufferSize, lightmapCopying, device, perDrawable, textureIndex, textureCache);
											
											FencesRotated::Fill(sortedFencesRotated, verticesTarget, indicesTarget, rotationData, indexBase, lightmapIndex, lightmapBufferSize, lightmapCopying, device, perDrawable, textureIndex, textureCache);
											
											TurbulentLit::Fill(sortedTurbulentLit, verticesTarget, indicesTarget, indexBase, lightmapIndex, lightmapBufferSize, lightmapCopying, device, perDrawable, textureIndex, textureCache);

											Turbulent::Fill(sortedTurbulent, verticesTarget, indicesTarget, indexBase, device, perDrawable, textureIndex, textureCache);
											
											if (lightmapBufferSize > 0 && (perDrawable.lightmapBuffer == nil || perDrawable.lightmapBuffer.length < lightmapBufferSize || perDrawable.lightmapBuffer.length > lightmapBufferSize * 2))
											{
												perDrawable.lightmapBuffer = [device newBufferWithLength:lightmapBufferSize * 3 / 2 options:MTLResourceCPUCacheModeWriteCombined];
											}
										}
										
										if (texturedVerticesSize > 0)
										{
											Sprites::Fill(sortedSprites, texturedVerticesTarget, indicesTarget, texturedIndexBase, device, perDrawable, textureIndex, textureCache);
										}
										
										if (particlesVerticesSize > 0)
										{
											auto verticesTarget = (float*)perDrawable.particleVertices.contents;
											
											Particles::Fill(particles, verticesTarget, indicesTarget, device, perDrawable);
										}
									}

									if (aliasVerticesSize > 0)
									{
										auto target = (float*)perDrawable.aliasVertices.contents;
										
										Alias::Fill(sortedAlias, target, device, perDrawable, textureIndex, textureCache, aliasIndicesIndex, aliasIndicesCache, colormapIndex, colormapCache);
										
										Viewmodel::Fill(sortedViewmodel, target, device, perDrawable, textureIndex, textureCache, aliasIndicesIndex, aliasIndicesCache, colormapIndex, colormapCache);
									}
								}

								id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
								
								if (lightmapCopying.size() > 0)
								{
									auto target = (unsigned char*)perDrawable.lightmapBuffer.contents;
									for (auto& entry : lightmapCopying)
									{
										memcpy(target, entry.data, entry.size);

										target += entry.size;
									}

									id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
									
									auto origin = MTLOriginMake(0, 0, 0);

									NSUInteger offset = 0;
									for (auto& entry : lightmapCopying)
									{
										auto size = MTLSizeMake(entry.width, entry.height, 1);
										
										[commandEncoder copyFromBuffer:perDrawable.lightmapBuffer sourceOffset:offset sourceBytesPerRow:entry.bytesPerRow sourceBytesPerImage:0 sourceSize:size toTexture:perDrawable.lightmapCache[entry.lightmap].texture destinationSlice:0 destinationLevel:0 destinationOrigin:origin];
										
										offset += entry.size;
									}
									
									[commandEncoder endEncoding];
								}

								for (NSUInteger v = 0; v < viewCount; v++)
								{
									auto perView = [perDrawable.views objectForKey:@(v)];

									if (perView == nil)
									{
										perView = [PerView new];
										[perDrawable.views setObject:perView forKey:@(v)];

										perView.renderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
										perView.renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
										perView.renderPassDescriptor.colorAttachments[0].texture = cp_drawable_get_color_texture(drawable, v);
										perView.renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
										perView.renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
										perView.renderPassDescriptor.depthAttachment.texture = cp_drawable_get_depth_texture(drawable, v);
										perView.renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
										perView.renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
										perView.renderPassDescriptor.depthAttachment.clearDepth = 0;
										perView.renderPassDescriptor.renderTargetArrayLength = viewCount;
										perView.renderPassDescriptor.rasterizationRateMap = cp_drawable_get_rasterization_rate_map(drawable, v);
									}
									
									cp_view_t view = cp_drawable_get_view(drawable, v);
									simd_float4 tangents = cp_view_get_tangents(view);
									simd_float2 depth_range = cp_drawable_get_depth_range(drawable);
									auto projectiveTransform = SPProjectiveTransform3DMakeFromTangents(tangents[0], tangents[1], tangents[2], tangents[3], depth_range[1], depth_range[0], true);
									auto& projectiveMatrix = projectiveTransform.matrix;
									auto projectionMatrix = simd_matrix(
										simd_make_float4(projectiveMatrix.columns[0][0],
														 projectiveMatrix.columns[0][1],
														 projectiveMatrix.columns[0][2],
														 projectiveMatrix.columns[0][3]),
										simd_make_float4(projectiveMatrix.columns[1][0],
														 projectiveMatrix.columns[1][1],
														 projectiveMatrix.columns[1][2],
														 projectiveMatrix.columns[1][3]),
										simd_make_float4(projectiveMatrix.columns[2][0],
														 projectiveMatrix.columns[2][1],
														 projectiveMatrix.columns[2][2],
														 projectiveMatrix.columns[2][3]),
										simd_make_float4(projectiveMatrix.columns[3][0],
														 projectiveMatrix.columns[3][1],
														 projectiveMatrix.columns[3][2],
														 projectiveMatrix.columns[3][3]));

									auto cameraMatrix = simd_mul(head_pose, cp_view_get_transform(view));
									auto viewMatrix = simd_transpose(cameraMatrix);
									
									if (mode == AppWorldMode)
									{
										id<MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:perView.renderPassDescriptor];
										
										Sky::Render(sky, commandEncoder, pipelines.sky, consoleDepthStencilState, projectionMatrix, perDrawable, head_pose, drawable, v, planarSamplerState, textureCache, textureSamplerState);

										Skybox::Render(skybox, commandEncoder, pipelines.skybox, consoleDepthStencilState, projectionMatrix, perDrawable, head_pose, drawable, planarSamplerState, textureCache, textureSamplerState);
										
										Surfaces::Render(sortedSurfaces, commandEncoder, pipelines.surface, surfaceDepthStencilState, vertexTransformMatrix, viewMatrix, projectionMatrix, perDrawable, surfacesIndexBase, planarSamplerState, lightmapSamplerState, textureCache, textureSamplerState);

										auto rotationData = rotation->data();
										
										SurfacesRotated::Render(sortedSurfacesRotated, commandEncoder, pipelines.surfaceRotated, surfaceDepthStencilState, vertexTransformMatrix, viewMatrix, projectionMatrix, perDrawable, surfacesRotatedIndexBase, rotationData, planarSamplerState, lightmapSamplerState, textureCache, textureSamplerState);

										Fences::Render(sortedFences, commandEncoder, pipelines.fence, surfaceDepthStencilState, vertexTransformMatrix, viewMatrix, projectionMatrix, perDrawable, fencesIndexBase, planarSamplerState, lightmapSamplerState, textureCache, textureSamplerState);

										FencesRotated::Render(sortedFencesRotated, commandEncoder, pipelines.fenceRotated, surfaceDepthStencilState, vertexTransformMatrix, viewMatrix, projectionMatrix, perDrawable, fencesRotatedIndexBase, rotationData, planarSamplerState, lightmapSamplerState, textureCache, textureSamplerState);

										TurbulentLit::Render(sortedTurbulentLit, commandEncoder, pipelines.turbulentLit, surfaceDepthStencilState, vertexTransformMatrix, viewMatrix, projectionMatrix, perDrawable, turbulentLitIndexBase, planarSamplerState, lightmapSamplerState, textureCache, textureSamplerState);

										Turbulent::Render(sortedTurbulent, commandEncoder, pipelines.turbulent, surfaceDepthStencilState, vertexTransformMatrix, viewMatrix, projectionMatrix, perDrawable, turbulentIndexBase, planarSamplerState, textureCache, textureSamplerState);

										NSUInteger aliasVertexBase = 0;

										Alias::Render(sortedAlias, commandEncoder, pipelines.alias, surfaceDepthStencilState, vertexTransformMatrix, viewMatrix, projectionMatrix, perDrawable, aliasVertexBase, planarSamplerState, textureCache, textureSamplerState, aliasIndicesCache, colormapCache);
										
										Viewmodel::Render(sortedViewmodel, commandEncoder, pipelines.viewmodel, surfaceDepthStencilState, vertexTransformMatrix, viewMatrix, projectionMatrix, perDrawable, aliasVertexBase, planarSamplerState, textureCache, textureSamplerState, aliasIndicesCache, colormapCache);
										
										Sprites::Render(sortedSprites, commandEncoder, pipelines.sprite, surfaceDepthStencilState, vertexTransformMatrix, viewMatrix, projectionMatrix, perDrawable, spritesIndexBase, planarSamplerState, textureCache, textureSamplerState);
										
										Particles::Render(particles, commandEncoder, pipelines.particle, surfaceDepthStencilState, vertexTransformMatrix, viewMatrix, projectionMatrix, perDrawable, particlesIndexBase, drawable, planarSamplerState);

										cameraMatrix = simd_mul(locked_head_pose, cp_view_get_transform(view));
										viewMatrix = simd_transpose(cameraMatrix);

										[commandEncoder setRenderPipelineState:pipelines.console];
										[commandEncoder setDepthStencilState:consoleDepthStencilState];
										[commandEncoder setVertexBytes:&viewMatrix length:sizeof(viewMatrix) atIndex:1];
										[commandEncoder setVertexBytes:&projectionMatrix length:sizeof(projectionMatrix) atIndex:2];
										[commandEncoder setFragmentTexture:perDrawable.console.texture atIndex:0];
										[commandEncoder setFragmentTexture:perDrawable.palette.texture atIndex:1];
										[commandEncoder setFragmentSamplerState:planarSamplerState atIndex:0];
										[commandEncoder setFragmentSamplerState:planarSamplerState atIndex:1];

										[commandEncoder setVertexBuffer:consoleTopPlane offset:0 atIndex:0];
										[commandEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];

										[commandEncoder setVertexBuffer:consoleBottomPlane offset:0 atIndex:0];
										[commandEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];

										[commandEncoder endEncoding];
									}
									else if (mode == AppScreenMode)
									{
										{
											std::lock_guard<std::mutex> lock(Locks::RenderMutex);
											
											[perDrawable.palette.texture replaceRegion:perDrawable.palette.region mipmapLevel:0 withBytes:d_8to24table bytesPerRow:perDrawable.palette.region.size.width * 4];
											[perDrawable.screen.texture replaceRegion:perDrawable.screen.region mipmapLevel:0 withBytes:vid_buffer.data() bytesPerRow:perDrawable.screen.region.size.width];
											[perDrawable.console.texture replaceRegion:perDrawable.console.region mipmapLevel:0 withBytes:con_buffer.data() bytesPerRow:perDrawable.console.region.size.width];
										}
										
										{
											std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
											
											for (auto& directRect : DirectRect::directRects)
											{
												auto x = directRect.x * (perDrawable.console.region.size.width - directRect.width) / (directRect.vid_width - directRect.width);
												auto y = directRect.y * (perDrawable.console.region.size.height - directRect.width) / (directRect.vid_height - directRect.height);
												
												MTLRegion directRegion = MTLRegionMake2D(x, y, directRect.width, directRect.height);
												[perDrawable.console.texture replaceRegion:directRegion mipmapLevel:0 withBytes:directRect.data bytesPerRow:directRect.width];
											}
										}
										
										id<MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:perView.renderPassDescriptor];
										[commandEncoder setRenderPipelineState:pipelines.planar];
										[commandEncoder setVertexBuffer:screenPlane offset:0 atIndex:0];
										[commandEncoder setVertexBytes:&viewMatrix length:sizeof(viewMatrix) atIndex:1];
										[commandEncoder setVertexBytes:&projectionMatrix length:sizeof(projectionMatrix) atIndex:2];
										[commandEncoder setFragmentTexture:perDrawable.screen.texture atIndex:0];
										[commandEncoder setFragmentTexture:perDrawable.console.texture atIndex:1];
										[commandEncoder setFragmentTexture:perDrawable.palette.texture atIndex:2];
										[commandEncoder setFragmentSamplerState:planarSamplerState atIndex:0];
										[commandEncoder setFragmentSamplerState:planarSamplerState atIndex:1];
										[commandEncoder setFragmentSamplerState:planarSamplerState atIndex:2];
										[commandEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
										[commandEncoder endEncoding];
									}
								}
								
								cp_drawable_encode_present(drawable, commandBuffer);
								
								[commandBuffer commit];
							}
							
							cp_frame_end_submission(frame);
						}
					}
				}
				
				[NSThread sleepForTimeInterval:0];
			}
				break;
				
			case cp_layer_renderer_state_invalidated:
				engineStop.stopEngine = true;
				break;
		}
	}
}

+(void)renderFrame:(CGContextRef)context size:(CGSize)size date:(NSDate*)date
{
	{
		std::lock_guard<std::mutex> lock(Locks::ModeChangeMutex);
		
		appState.Mode = AppScreenMode;
	}

	{
		std::lock_guard<std::mutex> lock(Locks::RenderMutex);
		
		size_t vid_length = vid_width * vid_height;
		size_t vid_converted_length = vid_length * 4;

		if (vid_converted_length == 0) {
			return;
		}

		if (vid_converted.size() != vid_converted_length)
		{
			vid_converted.resize(vid_converted_length);
			vid_provider = CGDataProviderCreateWithData(NULL, vid_converted.data(), vid_converted.size(), NULL);
		}

		auto source = vid_buffer.data();
		auto target = vid_converted.data();
		
		while (vid_length > 0)
		{
			auto entry = d_8to24table[*source++];

			*target++ = entry;

			vid_length--;
		}

		size_t con_length = con_width * con_height;
		size_t con_converted_length = con_length * 4;

		if (con_converted.size() != con_converted_length)
		{
			con_converted.resize(con_converted_length);
			con_provider = CGDataProviderCreateWithData(NULL, con_converted.data(), con_converted.size(), NULL);
		}

		source = con_buffer.data();
		target = con_converted.data();
		
		while (con_length > 0)
		{
			auto color = *source++;
			if (color == 0 || color == 255)
			{
				*target++ = 0;
			}
			else
			{
				auto entry = d_8to24table[color];
				*target++ = entry;
			}
			
			con_length--;
		}
	}

	{
		std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
		
		for (auto& directRect : DirectRect::directRects)
		{
			auto x = directRect.x * (con_width - directRect.width) / (directRect.vid_width - directRect.width);
			auto y = directRect.y * (con_height - directRect.height) / (directRect.vid_height - directRect.height);

			auto source = directRect.data;
			auto target = con_converted.data() + y * con_width + x;
			
			for (auto v = 0; v < directRect.height; v++)
			{
				for (auto h = 0; h < directRect.width; h++)
				{
					auto entry = d_8to24table[*source++];

					*target++ = entry;
				}
				target += con_width;
				target -= directRect.width;
			}
		}
	}

	auto colorspace = CGColorSpaceCreateDeviceRGB();

	auto vid_image = CGImageCreate(vid_width, vid_height, 8, 32, vid_width * 4, colorspace,
				  (int)kCGBitmapByteOrderDefault | (int)kCGImageAlphaLast,
				  vid_provider, NULL, false, kCGRenderingIntentDefault);
	
	auto con_image = CGImageCreate(con_width, con_height, 8, 32, con_width * 4, colorspace,
				  (int)kCGBitmapByteOrderDefault | (int)kCGImageAlphaLast,
				  con_provider, NULL, false, kCGRenderingIntentDefault);

	CGContextSaveGState(context);
	CGContextSetInterpolationQuality(context, kCGInterpolationNone);
	CGContextTranslateCTM(context, 0, size.height);
	CGContextScaleCTM(context, 1.0, -1.0);
	CGContextDrawImage(context, CGRectMake(0, 0, size.width, size.height), vid_image);
	CGContextDrawImage(context, CGRectMake(0, 0, size.width, size.height), con_image);
	CGContextRestoreGState(context);

	CGImageRelease(con_image);

	CGImageRelease(vid_image);
	
	CGColorSpaceRelease(colorspace);
}

@end

