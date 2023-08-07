//
//  Engine.mm
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 23/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import "Engine.h"
#import <GameController/GameController.h>
#include "sys_macos.h"
#include "vid_macos.h"
#include "in_macos.h"
#include "Locks.h"
#include "Input.h"

extern "C" double CACurrentMediaTime();

extern m_state_t m_state;

@interface Engine ()

@end

@implementation Engine
{
	GCController* joystick;
}

-(void)StartEngine:(NSArray<NSString*>*)args engineStop:(EngineStop*)engineStop
{
	vid_width = 960;
	vid_height = 600;
	con_width = 320;
	con_height = 200;

	NSString* version = NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"];

	sys_version = std::string("MacOS ") + [version cStringUsingEncoding:NSString.defaultCStringEncoding];
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
		engineStop.stopEngine = true;
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
	
	while (!engineStop.stopEngine)
	{
		if (!host_initialized)
		{
			[NSThread sleepForTimeInterval:0];
			continue;
		}

		{
			std::lock_guard<std::mutex> lock(Locks::InputMutex);

			for (auto i = 0; i <= Input::lastInputQueueItem; i++)
			{
				auto key = Input::inputQueueKeys[i];
				if (key > 0)
				{
					Key_Event(key, (qboolean)Input::inputQueuePressed[i]);
				}
				auto& command = Input::inputQueueCommands[i];
				if (!command.empty())
				{
					Cmd_ExecuteString(command.c_str(), src_command);
				}
			}
			Input::lastInputQueueItem = -1;
		}

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
			std::lock_guard<std::mutex> lock(Locks::RenderMutex);

			VID_ReallocSurfCache();
		}

		auto updated = Host_FrameUpdate(frame_lapse);

		if (sys_quitcalled || sys_errormessage.length() > 0)
		{
			engineStop.stopEngine = true;
			return;
		}

		if (updated)
		{
			std::lock_guard<std::mutex> lock(Locks::RenderMutex);

			Host_FrameRender();
		}
		Host_FrameFinish(updated);
		
		[NSThread sleepForTimeInterval:0];
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
		Input::AddKeyInput(K_ENTER, pressed);
	}];
	[joystick.extendedGamepad.buttonA setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Input::AddKeyInput(K_ENTER, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Input::AddCommandInput("+jump");
			}
			else
			{
				Input::AddCommandInput("-jump");
			}
		}
	 }];
	[joystick.extendedGamepad.buttonB setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Input::AddKeyInput(K_ESCAPE, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Input::AddCommandInput("+movedown");
			}
			else
			{
				Input::AddCommandInput("-movedown");
			}
		}
	 }];
	[joystick.extendedGamepad.buttonX setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Input::AddKeyInput(K_ENTER, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Input::AddCommandInput("+speed");
			}
			else
			{
				Input::AddCommandInput("-speed");
			}
		}
	 }];
	[joystick.extendedGamepad.buttonY setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state == m_quit)
			{
				Input::AddKeyInput('y', pressed);
			}
			else
			{
				Input::AddKeyInput(K_ESCAPE, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Input::AddCommandInput("+moveup");
			}
			else
			{
				Input::AddCommandInput("-moveup");
			}
		}
	 }];
	[joystick.extendedGamepad.leftTrigger setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Input::AddKeyInput(K_ENTER, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Input::AddCommandInput("+attack");
			}
			else
			{
				Input::AddCommandInput("-attack");
			}
		}
	 }];
	[joystick.extendedGamepad.leftShoulder setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Input::AddKeyInput(K_ESCAPE, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Input::AddCommandInput("impulse 10");
			}
		}
	 }];
	[joystick.extendedGamepad.rightTrigger setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Input::AddKeyInput(K_ENTER, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Input::AddCommandInput("+attack");
			}
			else
			{
				Input::AddCommandInput("-attack");
			}
		}
	 }];
	[joystick.extendedGamepad.rightShoulder setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				Input::AddKeyInput(K_ESCAPE, pressed);
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				Input::AddCommandInput("impulse 10");
			}
		}
	 }];
	[joystick.extendedGamepad.dpad.up setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		Input::AddKeyInput(K_UPARROW, pressed);
	 }];
	[joystick.extendedGamepad.dpad.left setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		Input::AddKeyInput(K_LEFTARROW, pressed);
	 }];
	[joystick.extendedGamepad.dpad.right setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		Input::AddKeyInput(K_RIGHTARROW, pressed);
	 }];
	[joystick.extendedGamepad.dpad.down setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		Input::AddKeyInput(K_DOWNARROW, pressed);
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
			Input::AddCommandInput("centerview");
		}
	 }];
	[joystick.extendedGamepad.rightThumbstickButton setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (pressed)
		{
			Input::AddCommandInput("+mlook");
		}
		else
		{
			Input::AddCommandInput("-mlook");
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
