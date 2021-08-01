#include "Input.h"
#include "AppState.h"
#include "vid_ovr.h"
#include "in_ovr.h"
#include "Utils.h"

extern m_state_t m_state;

std::vector<Input> Input::inputQueue(8);
int Input::lastInputQueueItem = -1;

void Input::AddKeyInput(int key, int down)
{
	lastInputQueueItem++;
	if (lastInputQueueItem >= inputQueue.size())
	{
		inputQueue.emplace_back();
	}
	auto& entry = inputQueue[lastInputQueueItem];
	entry.key = key;
	entry.down = down;
	entry.command.clear();
}

void Input::AddCommandInput(const char* command)
{
	lastInputQueueItem++;
	if (lastInputQueueItem >= inputQueue.size())
	{
		inputQueue.emplace_back();
	}
	auto& entry = inputQueue[lastInputQueueItem];
	entry.key = 0;
	entry.down = false;
	entry.command = command;
}

void Input::Handle(AppState& appState, bool triggerHandled)
{
	std::lock_guard<std::mutex> lock(appState.InputMutex);
	
	const XrActiveActionSet activeActionSet { appState.ActionSet, XR_NULL_PATH };
	XrActionsSyncInfo syncInfo { XR_TYPE_ACTIONS_SYNC_INFO };
	syncInfo.countActiveActionSets = 1;
	syncInfo.activeActionSets = &activeActionSet;
	CHECK_XRCMD(xrSyncActions(appState.Session, &syncInfo));

	XrActionStateGetInfo actionGetInfo { XR_TYPE_ACTION_STATE_GET_INFO };
	XrActionStateBoolean booleanActionState { XR_TYPE_ACTION_STATE_BOOLEAN };
	XrActionStateFloat floatActionState { XR_TYPE_ACTION_STATE_FLOAT };

	if (appState.Mode == AppStartupMode)
	{
		if (!appState.StartupButtonsPressed)
		{
			auto count = 0;
			actionGetInfo.action = appState.Play1Action;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.currentState)
			{
				count++;
			}
			actionGetInfo.action = appState.Play2Action;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.currentState)
			{
				count++;
			}
			if (count > 1)
			{
				appState.Mode = AppWorldMode;
				appState.StartupButtonsPressed = true;
			}
		}
	}
	else if (appState.Mode == AppScreenMode)
	{
		if (host_initialized)
		{
			/*if (LeftButtonIsDown(appState, ovrButton_Enter))
			{
				AddKeyInput(K_ESCAPE, true);
			}
			if (LeftButtonIsUp(appState, ovrButton_Enter))
			{
				AddKeyInput(K_ESCAPE, false);
			}
			if (key_dest == key_game)
			{
				if (joy_initialized)
				{
					joy_avail = true;
					pdwRawValue[JOY_AXIS_X] = -appState.LeftController.Joystick.x;
					pdwRawValue[JOY_AXIS_Y] = -appState.LeftController.Joystick.y;
					pdwRawValue[JOY_AXIS_Z] = appState.RightController.Joystick.x;
					pdwRawValue[JOY_AXIS_R] = appState.RightController.Joystick.y;
				}
				if (!triggerHandled)
				{
					if (LeftButtonIsDown(appState, ovrButton_Trigger) || RightButtonIsDown(appState, ovrButton_Trigger))
					{
						AddCommandInput("+attack");
					}
					if (LeftButtonIsUp(appState, ovrButton_Trigger) || RightButtonIsUp(appState, ovrButton_Trigger))
					{
						AddCommandInput("-attack");
					}
				}
				if (LeftButtonIsDown(appState, ovrButton_GripTrigger) || RightButtonIsDown(appState, ovrButton_GripTrigger))
				{
					AddCommandInput("+speed");
				}
				if (LeftButtonIsUp(appState, ovrButton_GripTrigger) || RightButtonIsUp(appState, ovrButton_GripTrigger))
				{
					AddCommandInput("-speed");
				}
				if (LeftButtonIsDown(appState, ovrButton_Joystick))
				{
					AddCommandInput("impulse 10");
				}
				if (m_state == m_quit)
				{
					if (RightButtonIsDown(appState, ovrButton_B))
					{
						AddCommandInput("+jump");
					}
					if (RightButtonIsUp(appState, ovrButton_B))
					{
						AddCommandInput("-jump");
					}
					if (LeftButtonIsDown(appState, ovrButton_Y))
					{
						AddKeyInput('y', true);
					}
					if (LeftButtonIsUp(appState, ovrButton_Y))
					{
						AddKeyInput('y', false);
					}
				}
				else
				{
					if (LeftButtonIsDown(appState, ovrButton_Y) || RightButtonIsDown(appState, ovrButton_B))
					{
						AddCommandInput("+jump");
					}
					if (LeftButtonIsUp(appState, ovrButton_Y) || RightButtonIsUp(appState, ovrButton_B))
					{
						AddCommandInput("-jump");
					}
				}
				if (LeftButtonIsDown(appState, ovrButton_X) || RightButtonIsDown(appState, ovrButton_A))
				{
					AddCommandInput("+movedown");
				}
				if (LeftButtonIsUp(appState, ovrButton_X) || RightButtonIsUp(appState, ovrButton_A))
				{
					AddCommandInput("-movedown");
				}
				if (RightButtonIsDown(appState, ovrButton_Joystick))
				{
					AddCommandInput("+mlook");
				}
				if (RightButtonIsUp(appState, ovrButton_Joystick))
				{
					AddCommandInput("-mlook");
				}
			}
			else
			{
				if ((appState.LeftController.Joystick.y > 0.5 && appState.LeftController.PreviousJoystick.y <= 0.5) || (appState.RightController.Joystick.y > 0.5 && appState.RightController.PreviousJoystick.y <= 0.5))
				{
					AddKeyInput(K_UPARROW, true);
				}
				if ((appState.LeftController.Joystick.y <= 0.5 && appState.LeftController.PreviousJoystick.y > 0.5) || (appState.RightController.Joystick.y <= 0.5 && appState.RightController.PreviousJoystick.y > 0.5))
				{
					AddKeyInput(K_UPARROW, false);
				}
				if ((appState.LeftController.Joystick.y < -0.5 && appState.LeftController.PreviousJoystick.y >= -0.5) || (appState.RightController.Joystick.y < -0.5 && appState.RightController.PreviousJoystick.y >= -0.5))
				{
					AddKeyInput(K_DOWNARROW, true);
				}
				if ((appState.LeftController.Joystick.y >= -0.5 && appState.LeftController.PreviousJoystick.y < -0.5) || (appState.RightController.Joystick.y >= -0.5 && appState.RightController.PreviousJoystick.y < -0.5))
				{
					AddKeyInput(K_DOWNARROW, false);
				}
				if ((appState.LeftController.Joystick.x > 0.5 && appState.LeftController.PreviousJoystick.x <= 0.5) || (appState.RightController.Joystick.x > 0.5 && appState.RightController.PreviousJoystick.x <= 0.5))
				{
					AddKeyInput(K_RIGHTARROW, true);
				}
				if ((appState.LeftController.Joystick.x <= 0.5 && appState.LeftController.PreviousJoystick.x > 0.5) || (appState.RightController.Joystick.x <= 0.5 && appState.RightController.PreviousJoystick.x > 0.5))
				{
					AddKeyInput(K_RIGHTARROW, false);
				}
				if ((appState.LeftController.Joystick.x < -0.5 && appState.LeftController.PreviousJoystick.x >= -0.5) || (appState.RightController.Joystick.x < -0.5 && appState.RightController.PreviousJoystick.x >= -0.5))
				{
					AddKeyInput(K_LEFTARROW, true);
				}
				if ((appState.LeftController.Joystick.x >= -0.5 && appState.LeftController.PreviousJoystick.x < -0.5) || (appState.RightController.Joystick.x >= -0.5 && appState.RightController.PreviousJoystick.x < -0.5))
				{
					AddKeyInput(K_LEFTARROW, false);
				}
				if ((LeftButtonIsDown(appState, ovrButton_Trigger) && !triggerHandled) || LeftButtonIsDown(appState, ovrButton_X) || (RightButtonIsDown(appState, ovrButton_Trigger) && !triggerHandled) || RightButtonIsDown(appState, ovrButton_A))
				{
					AddKeyInput(K_ENTER, true);
				}
				if ((LeftButtonIsUp(appState, ovrButton_Trigger) && !triggerHandled) || LeftButtonIsUp(appState, ovrButton_X) || (RightButtonIsUp(appState, ovrButton_Trigger) && !triggerHandled) || RightButtonIsUp(appState, ovrButton_A))
				{
					AddKeyInput(K_ENTER, false);
				}
				if (LeftButtonIsDown(appState, ovrButton_GripTrigger) || RightButtonIsDown(appState, ovrButton_GripTrigger) || RightButtonIsDown(appState, ovrButton_B))
				{
					AddKeyInput(K_ESCAPE, true);
				}
				if (LeftButtonIsUp(appState, ovrButton_GripTrigger) || RightButtonIsUp(appState, ovrButton_GripTrigger) || RightButtonIsUp(appState, ovrButton_B))
				{
					AddKeyInput(K_ESCAPE, false);
				}
				if (m_state == m_quit)
				{
					if (LeftButtonIsDown(appState, ovrButton_GripTrigger) || RightButtonIsDown(appState, ovrButton_GripTrigger) || RightButtonIsDown(appState, ovrButton_B))
					{
						AddKeyInput(K_ESCAPE, true);
					}
					if (LeftButtonIsUp(appState, ovrButton_GripTrigger) || RightButtonIsUp(appState, ovrButton_GripTrigger) || RightButtonIsUp(appState, ovrButton_B))
					{
						AddKeyInput(K_ESCAPE, false);
					}
					if (LeftButtonIsDown(appState, ovrButton_Y))
					{
						AddKeyInput('y', true);
					}
					if (LeftButtonIsUp(appState, ovrButton_Y))
					{
						AddKeyInput('y', false);
					}
				}
				else
				{
					if (LeftButtonIsDown(appState, ovrButton_GripTrigger) || LeftButtonIsDown(appState, ovrButton_Y) || RightButtonIsDown(appState, ovrButton_GripTrigger) || RightButtonIsDown(appState, ovrButton_B))
					{
						AddKeyInput(K_ESCAPE, true);
					}
					if (LeftButtonIsUp(appState, ovrButton_GripTrigger) || LeftButtonIsUp(appState, ovrButton_Y) || RightButtonIsUp(appState, ovrButton_GripTrigger) || RightButtonIsUp(appState, ovrButton_B))
					{
						AddKeyInput(K_ESCAPE, false);
					}
				}
			}*/
		}
	}
	else if (appState.Mode == AppWorldMode)
	{
		if (host_initialized)
		{
			actionGetInfo.action = appState.MenuAction;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
			{
				if (booleanActionState.currentState)
				{
					AddKeyInput(K_ESCAPE, true);
				}
				else
				{
					AddKeyInput(K_ESCAPE, false);
				}
			}
			if (key_dest == key_game)
			{
				if (joy_initialized)
				{
					joy_avail = true;
					actionGetInfo.action = appState.MoveXAction;
					CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
					if (floatActionState.isActive && floatActionState.changedSinceLastSync)
					{
						pdwRawValue[JOY_AXIS_X] = -floatActionState.currentState;
					}
					actionGetInfo.action = appState.MoveYAction;
					CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
					if (floatActionState.isActive && floatActionState.changedSinceLastSync)
					{
						pdwRawValue[JOY_AXIS_Y] = -floatActionState.currentState;
					}
				}
				if (!triggerHandled)
				{
					actionGetInfo.action = appState.FireAction;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							if (!appState.ControlsMessageClosed && appState.ControlsMessageDisplayed && (appState.NearViewModel || (d_lists.last_viewmodel16 < 0 && d_lists.last_viewmodel32 < 0)))
							{
								SCR_Interrupt();
								appState.ControlsMessageClosed = true;
							}
							else if (appState.NearViewModel || (d_lists.last_viewmodel16 < 0 && d_lists.last_viewmodel32 < 0))
							{
								AddCommandInput("+attack");
							}
						}
						else
						{
							AddCommandInput("-attack");
						}
					}
				}
				actionGetInfo.action = appState.RunAction;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						AddCommandInput("+speed");
					}
					else
					{
						AddCommandInput("-speed");
					}
				}
				actionGetInfo.action = appState.SwitchWeaponAction;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync && booleanActionState.currentState)
				{
					AddCommandInput("impulse 10");
				}
				if (m_state == m_quit)
				{
					actionGetInfo.action = appState.QuitAction;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							AddKeyInput('y', true);
						}
						else
						{
							AddKeyInput('y', false);
						}
					}
				}
				else
				{
					actionGetInfo.action = appState.JumpAction;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							AddCommandInput("+jump");
						}
						else
						{
							AddCommandInput("-jump");
						}
					}
				}
				actionGetInfo.action = appState.SwimDownAction;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						AddCommandInput("+movedown");
					}
					else
					{
						AddCommandInput("-movedown");
					}
				}
			}
			else
			{
				auto thumbstick = appState.PreviousThumbstick;
				actionGetInfo.action = appState.MoveXAction;
				CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
				if (floatActionState.isActive && floatActionState.changedSinceLastSync)
				{
					thumbstick.x = -floatActionState.currentState;
				}
				actionGetInfo.action = appState.MoveYAction;
				CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
				if (floatActionState.isActive && floatActionState.changedSinceLastSync)
				{
					thumbstick.y = -floatActionState.currentState;
				}
				if (thumbstick.y > 0.5 && appState.PreviousThumbstick.y <= 0.5)
				{
					AddKeyInput(K_UPARROW, true);
				}
				if (thumbstick.y <= 0.5 && appState.PreviousThumbstick.y > 0.5)
				{
					AddKeyInput(K_UPARROW, false);
				}
				if (thumbstick.y < -0.5 && appState.PreviousThumbstick.y >= -0.5)
				{
					AddKeyInput(K_DOWNARROW, true);
				}
				if (thumbstick.y >= -0.5 && appState.PreviousThumbstick.y < -0.5)
				{
					AddKeyInput(K_DOWNARROW, false);
				}
				if (thumbstick.x > 0.5 && appState.PreviousThumbstick.x <= 0.5)
				{
					AddKeyInput(K_RIGHTARROW, true);
				}
				if (thumbstick.x <= 0.5 && appState.PreviousThumbstick.x > 0.5)
				{
					AddKeyInput(K_RIGHTARROW, false);
				}
				if (thumbstick.x < -0.5 && appState.PreviousThumbstick.x >= -0.5)
				{
					AddKeyInput(K_LEFTARROW, true);
				}
				if (thumbstick.x >= -0.5 && appState.PreviousThumbstick.x < -0.5)
				{
					AddKeyInput(K_LEFTARROW, false);
				}
				appState.PreviousThumbstick = thumbstick;
				/*if ((LeftButtonIsDown(appState, ovrButton_Trigger) && !triggerHandled) || LeftButtonIsDown(appState, ovrButton_X) || (RightButtonIsDown(appState, ovrButton_Trigger) && !triggerHandled) || RightButtonIsDown(appState, ovrButton_A))
				{
					AddKeyInput(K_ENTER, true);
				}
				if ((LeftButtonIsUp(appState, ovrButton_Trigger) && !triggerHandled) || LeftButtonIsUp(appState, ovrButton_X) || (RightButtonIsUp(appState, ovrButton_Trigger) && !triggerHandled) || RightButtonIsUp(appState, ovrButton_A))
				{
					AddKeyInput(K_ENTER, false);
				}
				if (m_state == m_quit)
				{
					if (LeftButtonIsDown(appState, ovrButton_GripTrigger) || RightButtonIsDown(appState, ovrButton_GripTrigger) || RightButtonIsDown(appState, ovrButton_B))
					{
						AddKeyInput(K_ESCAPE, true);
					}
					if (LeftButtonIsUp(appState, ovrButton_GripTrigger) || RightButtonIsUp(appState, ovrButton_GripTrigger) || RightButtonIsUp(appState, ovrButton_B))
					{
						AddKeyInput(K_ESCAPE, false);
					}
					if (LeftButtonIsDown(appState, ovrButton_Y))
					{
						AddKeyInput('y', true);
					}
					if (LeftButtonIsUp(appState, ovrButton_Y))
					{
						AddKeyInput('y', false);
					}
				}
				else
				{
					if (LeftButtonIsDown(appState, ovrButton_GripTrigger) || LeftButtonIsDown(appState, ovrButton_Y) || RightButtonIsDown(appState, ovrButton_GripTrigger) || RightButtonIsDown(appState, ovrButton_B))
					{
						AddKeyInput(K_ESCAPE, true);
					}
					if (LeftButtonIsUp(appState, ovrButton_GripTrigger) || LeftButtonIsUp(appState, ovrButton_Y) || RightButtonIsUp(appState, ovrButton_GripTrigger) || RightButtonIsUp(appState, ovrButton_B))
					{
						AddKeyInput(K_ESCAPE, false);
					}
				}*/
			}
		}
	}
}
