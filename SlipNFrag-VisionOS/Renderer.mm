//
//  Renderer.mm
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 30/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import "Renderer.h"
#import <Spatial/Spatial.h>
#include "sys_visionos.h"
#include "vid_visionos.h"
#include "AppState.h"
#include "Locks.h"
#include "DirectRect.h"

@interface Renderer ()

@end

@implementation Renderer

-(void)StartRenderer:(CP_OBJECT_cp_layer_renderer*)layerRenderer engineStop:(EngineStop*)engineStop
{
	while (!host_initialized && !engineStop.stopEngine)
	{
		[NSThread sleepForTimeInterval:0];
	}

	auto session = ar_session_create();

	ar_world_tracking_configuration_t world_tracking_config = ar_world_tracking_configuration_create();
	auto world_tracking = ar_world_tracking_provider_create(world_tracking_config);

	ar_data_providers_t providers = ar_data_providers_create();
	ar_data_providers_add_data_provider(providers, world_tracking);
	ar_session_run(session, providers);
	
	auto configuration = cp_layer_renderer_get_configuration(layerRenderer);
	auto colorPixelFormat = cp_layer_renderer_configuration_get_color_format(configuration);
	auto depthPixelFormat = cp_layer_renderer_configuration_get_depth_format(configuration);

	id<MTLDevice> device = MTLCreateSystemDefaultDevice();
	id<MTLCommandQueue> commandQueue = [device newCommandQueue];
	id<MTLLibrary> library = [device newDefaultLibrary];
	id<MTLFunction> vertexProgram = [library newFunctionWithName:@"vertexMain"];
	id<MTLFunction> fragmentProgram = [library newFunctionWithName:@"fragmentMain"];
	MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor new];
	vertexDescriptor.attributes[0].format = MTLVertexFormatFloat4;
	vertexDescriptor.attributes[0].offset = 0;
	vertexDescriptor.attributes[0].bufferIndex = 0;
	vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
	vertexDescriptor.attributes[1].offset = 16;
	vertexDescriptor.attributes[1].bufferIndex = 0;
	vertexDescriptor.layouts[0].stride = 24;
	MTLRenderPipelineDescriptor* pipelineStateDescriptor = [MTLRenderPipelineDescriptor new];
	pipelineStateDescriptor.vertexDescriptor = vertexDescriptor;
	pipelineStateDescriptor.vertexFunction = vertexProgram;
	pipelineStateDescriptor.fragmentFunction = fragmentProgram;
	pipelineStateDescriptor.colorAttachments[0].pixelFormat = colorPixelFormat;
	pipelineStateDescriptor.colorAttachments[0].blendingEnabled = YES;
	pipelineStateDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
	pipelineStateDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
	pipelineStateDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	pipelineStateDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	pipelineStateDescriptor.depthAttachmentPixelFormat = depthPixelFormat;
	pipelineStateDescriptor.rasterSampleCount = 1;
	NSError* error = nil;
	id<MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
	if (error != nil)
	{
		engineStop.stopEngineMessage = @"Rendering pipeline could not be created.";
		engineStop.stopEngine = true;
		return;
	}

	float screenPlaneVertices[] = {
		-1.6,  1, -2, 1,  0, 0,
		 1.6,  1, -2, 1,  1, 0,
		-1.6, -1, -2, 1,  0, 1,
		 1.6, -1, -2, 1,  1, 1
	};

	id<MTLBuffer> screenPlane = [device newBufferWithBytes:screenPlaneVertices length:sizeof(screenPlaneVertices) options:0];
	
	AppState appState { };
	
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
						cp_time_wait_until(cp_frame_timing_get_optimal_input_time(timing));

						cp_frame_start_submission(frame);
						auto drawable = cp_frame_query_drawable(frame);
						if (drawable != nullptr)
						{
							auto viewCount = cp_drawable_get_view_count(drawable);
							
							cp_frame_timing_t timing = cp_drawable_get_frame_timing(drawable);
							auto pose = ar_pose_create();

							auto p_time = cp_time_to_cf_time_interval(cp_frame_timing_get_presentation_time(timing));
							auto pose_status = ar_world_tracking_provider_query_pose_at_timestamp(world_tracking, p_time, pose);
							if (pose_status == ar_pose_status_success)
							{
								cp_drawable_set_ar_pose(drawable, pose);

								id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];

								for (auto v = 0; v < viewCount; v++)
								{
									PerDrawable* perDrawable = nullptr;

									for (size_t d = 0; d < appState.drawables.size(); d++)
									{
										if (appState.drawables[d].drawable == drawable)
										{
											perDrawable = &appState.drawables[d];
											break;
										}
									}
									
									if (perDrawable == nullptr)
									{
										appState.drawables.emplace_back();
										perDrawable = &appState.drawables.back();

										{
											std::lock_guard<std::mutex> lock(Locks::RenderMutex);
											
											perDrawable->palette.descriptor = [MTLTextureDescriptor new];
											perDrawable->palette.descriptor.textureType = MTLTextureType1D;
											perDrawable->palette.descriptor.pixelFormat = MTLPixelFormatRGBA8Unorm_sRGB;
											perDrawable->palette.descriptor.width = 256;
											perDrawable->palette.texture = [device newTextureWithDescriptor:perDrawable->palette.descriptor];
											perDrawable->palette.samplerDescriptor = [MTLSamplerDescriptor new];
											perDrawable->palette.samplerState = [device newSamplerStateWithDescriptor:perDrawable->palette.samplerDescriptor];
											perDrawable->palette.region = MTLRegionMake1D(0, 256);
											
											perDrawable->screen.descriptor = [MTLTextureDescriptor new];
											perDrawable->screen.descriptor.pixelFormat = MTLPixelFormatR8Unorm;
											perDrawable->screen.descriptor.width = vid_width;
											perDrawable->screen.descriptor.height = vid_height;
											perDrawable->screen.texture = [device newTextureWithDescriptor:perDrawable->screen.descriptor];
											perDrawable->screen.samplerDescriptor = [MTLSamplerDescriptor new];
											perDrawable->screen.samplerState = [device newSamplerStateWithDescriptor:perDrawable->screen.samplerDescriptor];
											perDrawable->screen.region = MTLRegionMake2D(0, 0, vid_width, vid_height);
											
											perDrawable->console.descriptor = [MTLTextureDescriptor new];
											perDrawable->console.descriptor.pixelFormat = MTLPixelFormatR8Unorm;
											perDrawable->console.descriptor.width = con_width;
											perDrawable->console.descriptor.height = con_height;
											perDrawable->console.texture = [device newTextureWithDescriptor:perDrawable->console.descriptor];
											perDrawable->console.samplerDescriptor = [MTLSamplerDescriptor new];
											perDrawable->console.samplerState = [device newSamplerStateWithDescriptor:perDrawable->console.samplerDescriptor];
											perDrawable->console.region = MTLRegionMake2D(0, 0, con_width, con_height);
										}
									}
									
									PerView* perView = nullptr;
									
									if (perDrawable->views.size() <= v)
									{
										perDrawable->views.emplace_back();
										perView = &perDrawable->views.back();
										
										perView->renderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
										perView->renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
										perView->renderPassDescriptor.colorAttachments[0].texture = cp_drawable_get_color_texture(drawable, v);
										perView->renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
										perView->renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
										perView->renderPassDescriptor.depthAttachment.texture = cp_drawable_get_depth_texture(drawable, v);
										perView->renderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
										perView->renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
										perView->renderPassDescriptor.renderTargetArrayLength = viewCount;
										perView->renderPassDescriptor.rasterizationRateMap = cp_drawable_get_rasterization_rate_map(drawable, v);
									}
									
									simd_float4x4 poseTransform = ar_pose_get_origin_from_device_transform(pose);

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

									auto cameraMatrix = simd_mul(poseTransform, cp_view_get_transform(view));
									auto viewMatrix = simd_inverse(cameraMatrix);

									{
										std::lock_guard<std::mutex> lock(Locks::RenderMutex);

										[perDrawable->palette.texture replaceRegion:perDrawable->palette.region mipmapLevel:0 withBytes:d_8to24table bytesPerRow:perDrawable->palette.region.size.width * 4];
										[perDrawable->screen.texture replaceRegion:perDrawable->screen.region mipmapLevel:0 withBytes:vid_buffer.data() bytesPerRow:perDrawable->screen.region.size.width];
										[perDrawable->console.texture replaceRegion:perDrawable->console.region mipmapLevel:0 withBytes:con_buffer.data() bytesPerRow:perDrawable->console.region.size.width];
									}
									
									{
										std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
										
										for (auto& directRect : DirectRect::directRects)
										{
											auto x = directRect.x * (perDrawable->console.region.size.width - directRect.width) / (directRect.vid_width - directRect.width);
											auto y = directRect.y * (perDrawable->console.region.size.height - directRect.width) / (directRect.vid_height - directRect.height);

											MTLRegion directRegion = MTLRegionMake2D(x, y, directRect.width, directRect.height);
											[perDrawable->console.texture replaceRegion:directRegion mipmapLevel:0 withBytes:directRect.data bytesPerRow:directRect.width];
										}
									}

									id<MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:perView->renderPassDescriptor];
									[commandEncoder setRenderPipelineState:pipelineState];
									[commandEncoder setVertexBuffer:screenPlane offset:0 atIndex:0];
									[commandEncoder setVertexBytes:&viewMatrix length:sizeof(viewMatrix) atIndex:1];
									[commandEncoder setVertexBytes:&projectionMatrix length:sizeof(projectionMatrix) atIndex:2];
									[commandEncoder setFragmentTexture:perDrawable->screen.texture atIndex:0];
									[commandEncoder setFragmentTexture:perDrawable->console.texture atIndex:1];
									[commandEncoder setFragmentTexture:perDrawable->palette.texture atIndex:2];
									[commandEncoder setFragmentSamplerState:perDrawable->screen.samplerState atIndex:0];
									[commandEncoder setFragmentSamplerState:perDrawable->console.samplerState atIndex:1];
									[commandEncoder setFragmentSamplerState:perDrawable->palette.samplerState atIndex:2];
									[commandEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
									[commandEncoder endEncoding];
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

@end

