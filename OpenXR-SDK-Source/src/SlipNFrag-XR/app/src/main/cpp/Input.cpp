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
				AddKeyInput(K_DOWNARROW, true);
			}
			if (thumbstick.y <= 0.5 && appState.PreviousThumbstick.y > 0.5)
			{
				AddKeyInput(K_DOWNARROW, false);
			}
			if (thumbstick.y < -0.5 && appState.PreviousThumbstick.y >= -0.5)
			{
				AddKeyInput(K_UPARROW, true);
			}
			if (thumbstick.y >= -0.5 && appState.PreviousThumbstick.y < -0.5)
			{
				AddKeyInput(K_UPARROW, false);
			}
			if (thumbstick.x > 0.5 && appState.PreviousThumbstick.x <= 0.5)
			{
				AddKeyInput(K_LEFTARROW, true);
			}
			if (thumbstick.x <= 0.5 && appState.PreviousThumbstick.x > 0.5)
			{
				AddKeyInput(K_LEFTARROW, false);
			}
			if (thumbstick.x < -0.5 && appState.PreviousThumbstick.x >= -0.5)
			{
				AddKeyInput(K_RIGHTARROW, true);
			}
			if (thumbstick.x >= -0.5 && appState.PreviousThumbstick.x < -0.5)
			{
				AddKeyInput(K_RIGHTARROW, false);
			}
			appState.PreviousThumbstick = thumbstick;
			auto enterDown = false;
			auto enterUp = false;
			actionGetInfo.action = appState.EnterTriggerAction;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.changedSinceLastSync && !triggerHandled)
			{
				if (booleanActionState.currentState)
				{
					enterDown = true;
				}
				else
				{
					enterUp = true;
				}
			}
			actionGetInfo.action = appState.EnterNonTriggerAction;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
			{
				if (booleanActionState.currentState)
				{
					enterDown = true;
				}
				else
				{
					enterUp = true;
				}
			}
			if (enterDown)
			{
				AddKeyInput(K_ENTER, true);
			}
			if (enterUp)
			{
				AddKeyInput(K_ENTER, false);
			}
			if (m_state == m_quit)
			{
				actionGetInfo.action = appState.EscapeNonYAction;
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
				actionGetInfo.action = appState.EscapeYAction;
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
			}
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
					AddKeyInput(K_DOWNARROW, true);
				}
				if (thumbstick.y <= 0.5 && appState.PreviousThumbstick.y > 0.5)
				{
					AddKeyInput(K_DOWNARROW, false);
				}
				if (thumbstick.y < -0.5 && appState.PreviousThumbstick.y >= -0.5)
				{
					AddKeyInput(K_UPARROW, true);
				}
				if (thumbstick.y >= -0.5 && appState.PreviousThumbstick.y < -0.5)
				{
					AddKeyInput(K_UPARROW, false);
				}
				if (thumbstick.x > 0.5 && appState.PreviousThumbstick.x <= 0.5)
				{
					AddKeyInput(K_LEFTARROW, true);
				}
				if (thumbstick.x <= 0.5 && appState.PreviousThumbstick.x > 0.5)
				{
					AddKeyInput(K_LEFTARROW, false);
				}
				if (thumbstick.x < -0.5 && appState.PreviousThumbstick.x >= -0.5)
				{
					AddKeyInput(K_RIGHTARROW, true);
				}
				if (thumbstick.x >= -0.5 && appState.PreviousThumbstick.x < -0.5)
				{
					AddKeyInput(K_RIGHTARROW, false);
				}
				appState.PreviousThumbstick = thumbstick;
				auto enterDown = false;
				auto enterUp = false;
				actionGetInfo.action = appState.EnterTriggerAction;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync && !triggerHandled)
				{
					if (booleanActionState.currentState)
					{
						enterDown = true;
					}
					else
					{
						enterUp = true;
					}
				}
				actionGetInfo.action = appState.EnterNonTriggerAction;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						enterDown = true;
					}
					else
					{
						enterUp = true;
					}
				}
				if (enterDown)
				{
					AddKeyInput(K_ENTER, true);
				}
				if (enterUp)
				{
					AddKeyInput(K_ENTER, false);
				}
				if (m_state == m_quit)
				{
					actionGetInfo.action = appState.EscapeNonYAction;
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
					actionGetInfo.action = appState.EscapeYAction;
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
				}
			}
		}
	}
}
