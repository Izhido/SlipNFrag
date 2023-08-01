//
//  Engine.mm
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 31/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#import "Engine.h"
#include "sys_macos.h"
#include "vid_macos.h"
#include "in_macos.h"
#import <GameController/GameController.h>

extern "C" double CACurrentMediaTime();

extern m_state_t m_state;

@interface Engine ()

@end

@implementation Engine
{
	Locks* locks;

	GCController* joystick;

	std::vector<int> inputQueueKeys;
	std::vector<bool> inputQueuePressed;
	std::vector<std::string> inputQueueCommands;

	int lastInputQueueItem;
}

-(void)StartEngine:(NSArray<NSString*>*)args locks:(Locks*)locks
{
	self->locks = locks;

	self->lastInputQueueItem = -1;
	
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
		locks.stopEngine = true;
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

	locks.engineStarted = true;
	
	double previousTime = -1;
	double currentTime = -1;
	
	while (!locks.stopEngine)
	{
		if (!host_initialized)
		{
			[NSThread sleepForTimeInterval:0];
			continue;
		}

		@synchronized(locks.inputLock)
		{
			for (auto i = 0; i <= lastInputQueueItem; i++)
			{
				auto key = inputQueueKeys[i];
				if (key > 0)
				{
					Key_Event(key, (qboolean)inputQueuePressed[i]);
				}
				auto& command = inputQueueCommands[i];
				if (!command.empty())
				{
					Cmd_ExecuteString(command.c_str(), src_command);
				}
			}
			lastInputQueueItem = -1;
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
			@synchronized(locks.renderLock)
			{
				VID_ReallocSurfCache();
			}
		}

		auto updated = Host_FrameUpdate(frame_lapse);

		if (sys_quitcalled || sys_errormessage.length() > 0)
		{
			locks.stopEngine = true;
			return;
		}

		if (updated)
		{
			@synchronized(locks.renderLock)
			{
				Host_FrameRender();
			}
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

-(void)addKeyInput:(int)key pressed:(bool)pressed
{
	@synchronized(locks.inputLock)
	{
		lastInputQueueItem++;
		if (lastInputQueueItem >= inputQueueKeys.size())
		{
			inputQueueKeys.emplace_back();
			inputQueuePressed.emplace_back();
			inputQueueCommands.emplace_back();
		}
		inputQueueKeys[lastInputQueueItem] = key;
		inputQueuePressed[lastInputQueueItem] = pressed;
		inputQueueCommands[lastInputQueueItem].clear();
	}
}

-(void)addCommandInput:(const char*)command
{
	@synchronized(locks.inputLock)
	{
		lastInputQueueItem++;
		if (lastInputQueueItem >= inputQueueKeys.size())
		{
			inputQueueKeys.emplace_back();
			inputQueuePressed.emplace_back();
			inputQueueCommands.emplace_back();
		}
		inputQueueKeys[lastInputQueueItem] = 0;
		inputQueuePressed[lastInputQueueItem] = false;
		inputQueueCommands[lastInputQueueItem] = command;
	}
}

-(void)enableJoystick
{
	joystick.playerIndex = GCControllerPlayerIndex1;
	if (@available(macOS 10.15, *)) {
		[joystick.extendedGamepad.buttonMenu setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
		 {
			[self addKeyInput:K_ESCAPE pressed:pressed];
		}];
	} else {
		joystick.controllerPausedHandler = ^(GCController * _Nonnull controller)
		{
			[self addKeyInput:K_ESCAPE pressed:true];
			[self addKeyInput:K_ESCAPE pressed:false];
		};
	}
	[joystick.extendedGamepad.buttonA setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				[self addKeyInput:K_ENTER pressed:pressed];
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				[self addCommandInput:"+jump"];
			}
			else
			{
				[self addCommandInput:"-jump"];
			}
		}
	 }];
	[joystick.extendedGamepad.buttonB setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				[self addKeyInput:K_ESCAPE pressed:pressed];
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				[self addCommandInput:"+movedown"];
			}
			else
			{
				[self addCommandInput:"-movedown"];
			}
		}
	 }];
	[joystick.extendedGamepad.buttonX setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				[self addKeyInput:K_ENTER pressed:pressed];
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				[self addCommandInput:"+speed"];
			}
			else
			{
				[self addCommandInput:"-speed"];
			}
		}
	 }];
	[joystick.extendedGamepad.buttonY setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state == m_quit)
			{
				[self addKeyInput:'y' pressed:pressed];
			}
			else
			{
				[self addKeyInput:K_ESCAPE pressed:pressed];
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				[self addCommandInput:"+moveup"];
			}
			else
			{
				[self addCommandInput:"-moveup"];
			}
		}
	 }];
	[joystick.extendedGamepad.leftTrigger setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				[self addKeyInput:K_ENTER pressed:pressed];
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				[self addCommandInput:"+attack"];
			}
			else
			{
				[self addCommandInput:"-attack"];
			}
		}
	 }];
	[joystick.extendedGamepad.leftShoulder setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				[self addKeyInput:K_ESCAPE pressed:pressed];
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				[self addCommandInput:"impulse 10"];
			}
		}
	 }];
	[joystick.extendedGamepad.rightTrigger setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				[self addKeyInput:K_ENTER pressed:pressed];
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				[self addCommandInput:"+attack"];
			}
			else
			{
				[self addCommandInput:"-attack"];
			}
		}
	 }];
	[joystick.extendedGamepad.rightShoulder setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (key_dest == key_menu)
		{
			if (m_state != m_quit)
			{
				[self addKeyInput:K_ESCAPE pressed:pressed];
			}
		}
		else if (key_dest == key_game)
		{
			if (pressed)
			{
				[self addCommandInput:"impulse 10"];
			}
		}
	 }];
	[joystick.extendedGamepad.dpad.up setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		 [self addKeyInput:K_UPARROW pressed:pressed];
	 }];
	[joystick.extendedGamepad.dpad.left setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		 [self addKeyInput:K_LEFTARROW pressed:pressed];
	 }];
	[joystick.extendedGamepad.dpad.right setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		 [self addKeyInput:K_RIGHTARROW pressed:pressed];
	 }];
	[joystick.extendedGamepad.dpad.down setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		 [self addKeyInput:K_DOWNARROW pressed:pressed];
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
			[self addCommandInput:"centerview"];
		}
	 }];
	[joystick.extendedGamepad.rightThumbstickButton setPressedChangedHandler:^(GCControllerButtonInput * _Nonnull button, float value, BOOL pressed)
	 {
		if (pressed)
		{
			[self addCommandInput:"+mlook"];
		}
		else
		{
			[self addCommandInput:"-mlook"];
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
