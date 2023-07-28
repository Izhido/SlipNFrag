//
//  Engine.mm
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 23/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import "Engine.h"
#import <Spatial/Spatial.h>
#include "sys_visionos.h"
#include "vid_visionos.h"
#include "in_visionos.h"
#include <thread>
#include "AppState.h"
#import <GameController/GameController.h>

extern "C" double CACurrentMediaTime();

extern m_state_t m_state;

@interface Engine ()

@end

@implementation Engine
{
	GCController* joystick;
}

-(void)StartGame:(NSArray<NSString*>*)args layerRenderer:(CP_OBJECT_cp_layer_renderer*)layerRenderer
{
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
		_stopGameMessage = @"Rendering pipeline could not be created.";
		return;
	}

	float screenPlaneVertices[] = {
		-1.6,  1, -2, 1,  0, 0,
		 1.6,  1, -2, 1,  1, 0,
		-1.6, -1, -2, 1,  0, 1,
		 1.6, -1, -2, 1,  1, 1
	};

	id<MTLBuffer> screenPlane = [device newBufferWithBytes:screenPlaneVertices length:sizeof(screenPlaneVertices) options:0];

	vid_width = 960;
	vid_height = 600;
	con_width = 320;
	con_height = 200;

	NSString* version = NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"];

	sys_version = std::string("VisionOS ") + [version cStringUsingEncoding:NSString.defaultCStringEncoding];
	sys_argc = (int)args.count;
	sys_argv = new char*[sys_argc];
	for (auto i = 0; i < args.count; i++)
	{
		sys_argv[i] = new char[args[i].length + 1];
		strcpy(sys_argv[i], [args[i] cStringUsingEncoding:[NSString defaultCStringEncoding]]);
	}

	Sys_Init(sys_argc, sys_argv);
	
	if (sys_errormessage.length() > 0)
	{
		_stopGameMessage = [NSString stringWithCString:sys_errormessage.c_str() encoding:[NSString defaultCStringEncoding]];
		return;
	}

	if (joy_initialized)
	{
		for (GCController* controller in [GCController controllers])
		{
			if (controller.extendedGamepad != nil)
			{
				joystick = controller;
				joy_avail = YES;
				[self enableJoystick];
				break;
			}
		}
		[NSNotificationCenter.defaultCenter addObserver:self selector:@selector(controllerDidConnect:) name:GCControllerDidConnectNotification object:nil];
		[NSNotificationCenter.defaultCenter addObserver:self selector:@selector(controllerDidDisconnect:) name:GCControllerDidDisconnectNotification object:nil];
	}

	double previousTime = -1;
	double currentTime = -1;
	
	AppState appState { };
	
	while (!self.stopGame) @autoreleasepool
	{
		switch (cp_layer_renderer_get_state(layerRenderer))
		{
			case cp_layer_renderer_state_paused:
				std::this_thread::yield();
				break;

			case cp_layer_renderer_state_running:
			{
				if (previousTime < 0)
				{
					previousTime = CACurrentMediaTime();
				}
				else if (currentTime < 0)
				{
					currentTime = CACurrentMediaTime();
				}
				else
				{
					previousTime = currentTime;
					currentTime = CACurrentMediaTime();
					frame_lapse = currentTime - previousTime;
				}
				
				if (r_cache_thrash)
				{
					VID_ReallocSurfCache();
				}

				Sys_Frame(frame_lapse);
				
				if (sys_errormessage.length() > 0)
				{
					_stopGameMessage = [NSString stringWithCString:sys_errormessage.c_str() encoding:[NSString defaultCStringEncoding]];
					return;
				}
				
				auto frame = cp_layer_renderer_query_next_frame(layerRenderer);
				if (frame != nullptr)
				{
					auto timing = cp_frame_predict_timing(frame);
					if (timing != nullptr)
					{
						cp_frame_start_update(frame);
						
						//my_input_state input_state = my_engine_gather_inputs(engine, timing);
						//my_engine_update_frame(engine, timing, input_state);
						
						cp_frame_end_update(frame);
						
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

									[perDrawable->palette.texture replaceRegion:perDrawable->palette.region mipmapLevel:0 withBytes:d_8to24table bytesPerRow:perDrawable->palette.region.size.width * 4];
									[perDrawable->screen.texture replaceRegion:perDrawable->screen.region mipmapLevel:0 withBytes:vid_buffer.data() bytesPerRow:perDrawable->screen.region.size.width];
									[perDrawable->console.texture replaceRegion:perDrawable->console.region mipmapLevel:0 withBytes:con_buffer.data() bytesPerRow:perDrawable->console.region.size.width];
									
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

				std::this_thread::yield();
			}
			break;

			case cp_layer_renderer_state_invalidated:
				self.stopGame = true;
				break;
		}
	}
}

-(void)controllerDidConnect:(NSNotification*)notification
{
	if (joystick == nullptr)
	{
		auto controller = (GCController*)notification.object;
		if (controller.extendedGamepad != nil)
		{
			joystick = controller;
			joy_avail = YES;
			[self enableJoystick];
		}
	}
}

-(void)controllerDidDisconnect:(NSNotification*)notification
{
	if (joystick != nullptr)
	{
		auto controller = (GCController*)notification.object;
		if (joystick == controller)
		{
			[self disableJoystick];
			joy_avail = NO;
			joystick = nil;
		}
	}
}

-(void)enableJoystick
{
	joystick.playerIndex = GCControllerPlayerIndex1;
	[joystick.extendedGamepad.buttonMenu setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		Key_Event(K_ESCAPE, pressed);
	}];
	[joystick.extendedGamepad.buttonA setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Key_Event(K_ENTER, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Cmd_ExecuteString("+jump", src_command);
			}
			else
			{
				Cmd_ExecuteString("-jump", src_command);
			}
		}
	 }];
	[joystick.extendedGamepad.buttonB setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Key_Event(K_ESCAPE, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Cmd_ExecuteString("+movedown", src_command);
			}
			else
			{
				Cmd_ExecuteString("-movedown", src_command);
			}
		}
	 }];
	[joystick.extendedGamepad.buttonX setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Key_Event(K_ENTER, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Cmd_ExecuteString("+speed", src_command);
			}
			else
			{
				Cmd_ExecuteString("-speed", src_command);
			}
		}
	 }];
	[joystick.extendedGamepad.buttonY setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state == m_quit)
			{
				Key_Event('y', pressed);
			}
			else
			{
				Key_Event(K_ESCAPE, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Cmd_ExecuteString("+moveup", src_command);
			}
			else
			{
				Cmd_ExecuteString("-moveup", src_command);
			}
		}
	 }];
	[joystick.extendedGamepad.leftTrigger setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Key_Event(K_ENTER, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Cmd_ExecuteString("+attack", src_command);
			}
			else
			{
				Cmd_ExecuteString("-attack", src_command);
			}
		}
	 }];
	[joystick.extendedGamepad.leftShoulder setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Key_Event(K_ESCAPE, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Cmd_ExecuteString("impulse 10", src_command);
			}
		}
	 }];
	[joystick.extendedGamepad.rightTrigger setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Key_Event(K_ENTER, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Cmd_ExecuteString("+attack", src_command);
			}
			else
			{
				Cmd_ExecuteString("-attack", src_command);
			}
		}
	 }];
	[joystick.extendedGamepad.rightShoulder setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Key_Event(K_ESCAPE, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Cmd_ExecuteString("impulse 10", src_command);
			}
		}
	 }];
	[joystick.extendedGamepad.dpad.up setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		 Key_Event(128, pressed);
	 }];
	[joystick.extendedGamepad.dpad.left setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		 Key_Event(130, pressed);
	 }];
	[joystick.extendedGamepad.dpad.right setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		 Key_Event(131, pressed);
	 }];
	[joystick.extendedGamepad.dpad.down setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		 Key_Event(129, pressed);
	 }];
	[joystick.extendedGamepad.leftThumbstick setValueChangedHandler:^(GCControllerDirectionPad * _Nonnull dpad, float xValue, float yValue)
	 {
		 pdwRawValue[JOY_AXIS_X] = -xValue;
		 pdwRawValue[JOY_AXIS_Y] = -yValue;
	 }];
	[joystick.extendedGamepad.rightThumbstick setValueChangedHandler:^(GCControllerDirectionPad * _Nonnull dpad, float xValue, float yValue)
	 {
		 pdwRawValue[JOY_AXIS_Z] = xValue;
		 pdwRawValue[JOY_AXIS_R] = -yValue;
	 }];
	[joystick.extendedGamepad.leftThumbstickButton setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (pressed)
		{
			Cmd_ExecuteString("centerview", src_command);
		}
	 }];
	[joystick.extendedGamepad.rightThumbstickButton setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (pressed)
		{
			Cmd_ExecuteString("+mlook", src_command);
		}
		else
		{
			Cmd_ExecuteString("-mlook", src_command);
		}
	 }];

	Cvar_SetValue("joyadvanced", 1);
	Cvar_SetValue("joyadvaxisx", AxisSide);
	Cvar_SetValue("joyadvaxisy", AxisForward);
	Cvar_SetValue("joyadvaxisz", AxisTurn);
	Cvar_SetValue("joyadvaxisr", AxisLook);
	Joy_AdvancedUpdate_f();
}

-(void)disableJoystick
{
	in_forwardmove = 0.0;
	in_sidestepmove = 0.0;
	in_rollangle = 0.0;
	in_pitchangle = 0.0;
}

@end
