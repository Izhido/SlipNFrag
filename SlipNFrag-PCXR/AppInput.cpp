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
			actionGetInfo.action = appState.Actions.Play1;
			CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
			if (booleanActionState.isActive && booleanActionState.currentState)
			{
				count++;
			}
			actionGetInfo.action = appState.Actions.Play2;
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
				actionGetInfo.action = appState.Actions.EscapeNonY;
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
				actionGetInfo.action = appState.Actions.Quit;
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
				actionGetInfo.action = appState.Actions.EscapeY;
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
			actionGetInfo.action = appState.Actions.Menu;
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
				actionGetInfo.action = appState.Actions.MenuLeftHanded;
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
				actionGetInfo.action = appState.Actions.MenuRightHanded;
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
			actionGetInfo.action = appState.Actions.MoveX;
			CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
			if (floatActionState.isActive && floatActionState.changedSinceLastSync)
			{
				thumbstick.x = -floatActionState.currentState;
			}
			actionGetInfo.action = appState.Actions.MoveY;
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
				actionGetInfo.action = appState.Actions.EnterTrigger;
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
				actionGetInfo.action = appState.Actions.EnterNonTriggerLeftHanded;
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
				actionGetInfo.action = appState.Actions.EnterNonTriggerRightHanded;
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
				actionGetInfo.action = appState.Actions.Fire;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync && !booleanActionState.currentState)
				{
					AddCommandInput("-attack");
				}
			}
			actionGetInfo.action = appState.Actions.Run;
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
					actionGetInfo.action = appState.Actions.Quit;
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
					actionGetInfo.action = appState.Actions.JumpLeftHanded;
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
					actionGetInfo.action = appState.Actions.JumpRightHanded;
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
				actionGetInfo.action = appState.Actions.Menu;
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
					actionGetInfo.action = appState.Actions.MenuLeftHanded;
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
					actionGetInfo.action = appState.Actions.MenuRightHanded;
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
					actionGetInfo.action = appState.Actions.MoveX;
					CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
					if (floatActionState.isActive && floatActionState.changedSinceLastSync)
					{
						pdwRawValue[JOY_AXIS_X] = -floatActionState.currentState;
					}
					actionGetInfo.action = appState.Actions.MoveY;
					CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
					if (floatActionState.isActive && floatActionState.changedSinceLastSync)
					{
						pdwRawValue[JOY_AXIS_Y] = -floatActionState.currentState;
					}
				}
				if (!keyPressHandled)
				{
					actionGetInfo.action = appState.Actions.Fire;
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
				actionGetInfo.action = appState.Actions.Run;
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
				actionGetInfo.action = appState.Actions.SwitchWeapon;
				CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
				if (booleanActionState.isActive && booleanActionState.changedSinceLastSync && booleanActionState.currentState)
				{
					AddCommandInput("impulse 10");
				}
				if (isLeftHanded)
				{
					actionGetInfo.action = appState.Actions.SwimDownLeftHanded;
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
					actionGetInfo.action = appState.Actions.SwimDownRightHanded;
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
					actionGetInfo.action = appState.Actions.EscapeNonY;
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
					actionGetInfo.action = appState.Actions.Quit;
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
					actionGetInfo.action = appState.Actions.EscapeY;
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
				actionGetInfo.action = appState.Actions.Menu;
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
					actionGetInfo.action = appState.Actions.MenuLeftHanded;
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
					actionGetInfo.action = appState.Actions.MenuRightHanded;
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
				actionGetInfo.action = appState.Actions.MoveX;
				CHECK_XRCMD(xrGetActionStateFloat(appState.Session, &actionGetInfo, &floatActionState));
				if (floatActionState.isActive && floatActionState.changedSinceLastSync)
				{
					thumbstick.x = -floatActionState.currentState;
				}
				actionGetInfo.action = appState.Actions.MoveY;
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
					actionGetInfo.action = appState.Actions.EnterTrigger;
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
					actionGetInfo.action = appState.Actions.EnterNonTriggerLeftHanded;
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
					actionGetInfo.action = appState.Actions.EnterNonTriggerRightHanded;
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
