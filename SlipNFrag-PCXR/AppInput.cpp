#include "AppInput.h"
#include "AppState_pcxr.h"
#include "in_pcxr.h"
#include "Utils.h"
#include "Locks.h"

extern m_state_t m_state;

std::vector<AppInput> AppInput::inputQueue(8);
int AppInput::lastInputQueueItem = -1;

void AppInput::AddKeyInput(int key, int down)
{
	std::lock_guard<std::mutex> lock(Locks::InputMutex);

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

void AppInput::AddCommandInput(const char* command)
{
	std::lock_guard<std::mutex> lock(Locks::InputMutex);

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

void AppInput::Handle(AppState_pcxr& appState, bool keyPressHandled)
{
	XrActionStateGetInfo actionGetInfo { XR_TYPE_ACTION_STATE_GET_INFO };
	XrActionStateBoolean booleanActionState { XR_TYPE_ACTION_STATE_BOOLEAN };
	XrActionStateFloat floatActionState { XR_TYPE_ACTION_STATE_FLOAT };

	auto hand = Cvar_VariableString ("dominant_hand");
	auto isLeftHanded = (Q_strncmp(hand, "left", 4) == 0);

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
			auto escapeDown = false;
			auto escapeUp = false;
			actionGetInfo.action = appState.MenuAction;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
			{
				if (booleanActionState.currentState)
				{
					escapeDown = true;
				}
				else
				{
					escapeUp = true;
				}
			}
			if (isLeftHanded)
			{
				actionGetInfo.action = appState.MenuLeftHandedAction;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						escapeDown = true;
					}
					else
					{
						escapeUp = true;
					}
				}
			}
			else
			{
				actionGetInfo.action = appState.MenuRightHandedAction;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						escapeDown = true;
					}
					else
					{
						escapeUp = true;
					}
				}
			}
			if (escapeDown)
			{
				AddKeyInput(K_ESCAPE, true);
			}
			if (escapeUp)
			{
				AddKeyInput(K_ESCAPE, false);
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
			if (!keyPressHandled)
			{
				actionGetInfo.action = appState.EnterTriggerAction;
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
			}
			if (isLeftHanded)
			{
				actionGetInfo.action = appState.EnterNonTriggerLeftHandedAction;
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
			}
			else
			{
				actionGetInfo.action = appState.EnterNonTriggerRightHandedAction;
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
			}
			if (enterDown)
			{
				AddKeyInput(K_ENTER, true);
			}
			if (enterUp)
			{
				AddKeyInput(K_ENTER, false);
			}
			// The following actions are performed only to reset any in-game actions
			// after a transition occurs - this is why only currentState == false is checked:
			if (!keyPressHandled)
			{
				actionGetInfo.action = appState.FireAction;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync && !booleanActionState.currentState)
				{
					AddCommandInput("-attack");
				}
			}
			actionGetInfo.action = appState.RunAction;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.changedSinceLastSync && !booleanActionState.currentState)
			{
				AddCommandInput("-speed");
			}
		}
	}
	else if (appState.Mode == AppWorldMode)
	{
		if (host_initialized)
		{
			if (key_dest == key_game)
			{
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
				else if (isLeftHanded)
				{
					actionGetInfo.action = appState.JumpLeftHandedAction;
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
				else
				{
					actionGetInfo.action = appState.JumpRightHandedAction;
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
				auto escapeDown = false;
				auto escapeUp = false;
				actionGetInfo.action = appState.MenuAction;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						escapeDown = true;
					}
					else
					{
						escapeUp = true;
					}
				}
				if (isLeftHanded)
				{
					actionGetInfo.action = appState.MenuLeftHandedAction;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							escapeDown = true;
						}
						else
						{
							escapeUp = true;
						}
					}
				}
				else
				{
					actionGetInfo.action = appState.MenuRightHandedAction;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							escapeDown = true;
						}
						else
						{
							escapeUp = true;
						}
					}
				}
				if (escapeDown)
				{
					AddKeyInput(K_ESCAPE, true);
				}
				if (escapeUp)
				{
					AddKeyInput(K_ESCAPE, false);
				}
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
				if (!keyPressHandled)
				{
					actionGetInfo.action = appState.FireAction;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							AddCommandInput("+attack");
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
				if (isLeftHanded)
				{
					actionGetInfo.action = appState.SwimDownLeftHandedAction;
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
					actionGetInfo.action = appState.SwimDownRightHandedAction;
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
			}
			else
			{
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
				auto escapeDown = false;
				auto escapeUp = false;
				actionGetInfo.action = appState.MenuAction;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
				{
					if (booleanActionState.currentState)
					{
						escapeDown = true;
					}
					else
					{
						escapeUp = true;
					}
				}
				if (isLeftHanded)
				{
					actionGetInfo.action = appState.MenuLeftHandedAction;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							escapeDown = true;
						}
						else
						{
							escapeUp = true;
						}
					}
				}
				else
				{
					actionGetInfo.action = appState.MenuRightHandedAction;
					CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
					if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
					{
						if (booleanActionState.currentState)
						{
							escapeDown = true;
						}
						else
						{
							escapeUp = true;
						}
					}
				}
				if (escapeDown)
				{
					AddKeyInput(K_ESCAPE, true);
				}
				if (escapeUp)
				{
					AddKeyInput(K_ESCAPE, false);
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
				if (!keyPressHandled)
				{
					actionGetInfo.action = appState.EnterTriggerAction;
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
				}
				if (isLeftHanded)
				{
					actionGetInfo.action = appState.EnterNonTriggerLeftHandedAction;
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
				}
				else
				{
					actionGetInfo.action = appState.EnterNonTriggerRightHandedAction;
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
				}
				if (enterDown)
				{
					AddKeyInput(K_ENTER, true);
				}
				if (enterUp)
				{
					AddKeyInput(K_ENTER, false);
				}
			}
		}
	}
}
