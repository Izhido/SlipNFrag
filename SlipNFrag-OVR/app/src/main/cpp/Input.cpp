#include "Input.h"
#include "AppState.h"
#include "VrApi_Input.h"
#include "in_ovr.h"

extern m_state_t m_state;

std::vector<Input> Input::inputQueue;
int Input::lastInputQueueItem = -1;

bool Input::LeftButtonIsDown(AppState& appState, uint32_t button)
{
	return ((appState.LeftButtons & button) != 0 && (appState.PreviousLeftButtons & button) == 0);
}

bool Input::LeftButtonIsUp(AppState& appState, uint32_t button)
{
	return ((appState.LeftButtons & button) == 0 && (appState.PreviousLeftButtons & button) != 0);
}

bool Input::RightButtonIsDown(AppState& appState, uint32_t button)
{
	return ((appState.RightButtons & button) != 0 && (appState.PreviousRightButtons & button) == 0);
}

bool Input::RightButtonIsUp(AppState& appState, uint32_t button)
{
	return ((appState.RightButtons & button) == 0 && (appState.PreviousRightButtons & button) != 0);
}

void Input::AddKeyInput(int key, int down)
{
	lastInputQueueItem++;
	if (lastInputQueueItem >= inputQueue.size())
	{
		inputQueue.push_back({ key, down });
	}
	else
	{
		inputQueue[lastInputQueueItem].key = key;
		inputQueue[lastInputQueueItem].down = down;
		inputQueue[lastInputQueueItem].command.clear();
	}
}

void Input::AddCommandInput(const char* command)
{
	lastInputQueueItem++;
	if (lastInputQueueItem >= inputQueue.size())
	{
		inputQueue.push_back({ 0, false, command });
	}
	else
	{
		inputQueue[lastInputQueueItem].key = 0;
		inputQueue[lastInputQueueItem].down = false;
		inputQueue[lastInputQueueItem].command = command;
	}
}

void Input::Handle(AppState& appState)
{
	std::lock_guard<std::mutex> lock(appState.InputMutex);
	if (appState.Mode == AppStartupMode)
	{
		if (appState.StartupButtonsPressed)
		{
			if ((appState.LeftButtons & ovrButton_X) == 0 && (appState.RightButtons & ovrButton_A) == 0)
			{
				appState.Mode = AppWorldMode;
				appState.StartupButtonsPressed = false;
			}
		}
		else if ((appState.LeftButtons & ovrButton_X) != 0 && (appState.RightButtons & ovrButton_A) != 0)
		{
			appState.StartupButtonsPressed = true;
		}
	}
	else if (appState.Mode == AppScreenMode)
	{
		if (host_initialized)
		{
			if (LeftButtonIsDown(appState, ovrButton_Enter))
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
					pdwRawValue[JOY_AXIS_X] = -appState.LeftJoystick.x;
					pdwRawValue[JOY_AXIS_Y] = -appState.LeftJoystick.y;
					pdwRawValue[JOY_AXIS_Z] = appState.RightJoystick.x;
					pdwRawValue[JOY_AXIS_R] = appState.RightJoystick.y;
				}
				if (LeftButtonIsDown(appState, ovrButton_Trigger) || RightButtonIsDown(appState, ovrButton_Trigger))
				{
					AddCommandInput("+attack");
				}
				if (LeftButtonIsUp(appState, ovrButton_Trigger) || RightButtonIsUp(appState, ovrButton_Trigger))
				{
					AddCommandInput("-attack");
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
				if ((appState.LeftJoystick.y > 0.5 && appState.PreviousLeftJoystick.y <= 0.5) || (appState.RightJoystick.y > 0.5 && appState.PreviousRightJoystick.y <= 0.5))
				{
					AddKeyInput(K_UPARROW, true);
				}
				if ((appState.LeftJoystick.y <= 0.5 && appState.PreviousLeftJoystick.y > 0.5) || (appState.RightJoystick.y <= 0.5 && appState.PreviousRightJoystick.y > 0.5))
				{
					AddKeyInput(K_UPARROW, false);
				}
				if ((appState.LeftJoystick.y < -0.5 && appState.PreviousLeftJoystick.y >= -0.5) || (appState.RightJoystick.y < -0.5 && appState.PreviousRightJoystick.y >= -0.5))
				{
					AddKeyInput(K_DOWNARROW, true);
				}
				if ((appState.LeftJoystick.y >= -0.5 && appState.PreviousLeftJoystick.y < -0.5) || (appState.RightJoystick.y >= -0.5 && appState.PreviousRightJoystick.y < -0.5))
				{
					AddKeyInput(K_DOWNARROW, false);
				}
				if ((appState.LeftJoystick.x > 0.5 && appState.PreviousLeftJoystick.x <= 0.5) || (appState.RightJoystick.x > 0.5 && appState.PreviousRightJoystick.x <= 0.5))
				{
					AddKeyInput(K_RIGHTARROW, true);
				}
				if ((appState.LeftJoystick.x <= 0.5 && appState.PreviousLeftJoystick.x > 0.5) || (appState.RightJoystick.x <= 0.5 && appState.PreviousRightJoystick.x > 0.5))
				{
					AddKeyInput(K_RIGHTARROW, false);
				}
				if ((appState.LeftJoystick.x < -0.5 && appState.PreviousLeftJoystick.x >= -0.5) || (appState.RightJoystick.x < -0.5 && appState.PreviousRightJoystick.x >= -0.5))
				{
					AddKeyInput(K_LEFTARROW, true);
				}
				if ((appState.LeftJoystick.x >= -0.5 && appState.PreviousLeftJoystick.x < -0.5) || (appState.RightJoystick.x >= -0.5 && appState.PreviousRightJoystick.x < -0.5))
				{
					AddKeyInput(K_LEFTARROW, false);
				}
				if (LeftButtonIsDown(appState, ovrButton_Trigger) || LeftButtonIsDown(appState, ovrButton_X) || RightButtonIsDown(appState, ovrButton_Trigger) || RightButtonIsDown(appState, ovrButton_A))
				{
					AddKeyInput(K_ENTER, true);
				}
				if (LeftButtonIsUp(appState, ovrButton_Trigger) || LeftButtonIsUp(appState, ovrButton_X) || RightButtonIsUp(appState, ovrButton_Trigger) || RightButtonIsUp(appState, ovrButton_A))
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
			}
		}
	}
	else if (appState.Mode == AppWorldMode)
	{
		if (host_initialized)
		{
			if (LeftButtonIsDown(appState, ovrButton_Enter))
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
					pdwRawValue[JOY_AXIS_X] = -appState.LeftJoystick.x - appState.RightJoystick.x;
					pdwRawValue[JOY_AXIS_Y] = -appState.LeftJoystick.y - appState.RightJoystick.y;
				}
				if (LeftButtonIsDown(appState, ovrButton_Trigger) || RightButtonIsDown(appState, ovrButton_Trigger))
				{
					if (!appState.ControlsMessageClosed && appState.ControlsMessageDisplayed && (appState.NearViewModel || d_lists.last_viewmodel < 0))
					{
						SCR_Interrupt();
						appState.ControlsMessageClosed = true;
					}
					else if (appState.NearViewModel || d_lists.last_viewmodel < 0)
					{
						AddCommandInput("+attack");
					}
				}
				if (LeftButtonIsUp(appState, ovrButton_Trigger) || RightButtonIsUp(appState, ovrButton_Trigger))
				{
					AddCommandInput("-attack");
				}
				if (LeftButtonIsDown(appState, ovrButton_GripTrigger) || RightButtonIsDown(appState, ovrButton_GripTrigger))
				{
					AddCommandInput("+speed");
				}
				if (LeftButtonIsUp(appState, ovrButton_GripTrigger) || RightButtonIsUp(appState, ovrButton_GripTrigger))
				{
					AddCommandInput("-speed");
				}
				if (LeftButtonIsDown(appState, ovrButton_Joystick) || RightButtonIsDown(appState, ovrButton_Joystick))
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
			}
			else
			{
				if ((appState.LeftJoystick.y > 0.5 && appState.PreviousLeftJoystick.y <= 0.5) || (appState.RightJoystick.y > 0.5 && appState.PreviousRightJoystick.y <= 0.5))
				{
					AddKeyInput(K_UPARROW, true);
				}
				if ((appState.LeftJoystick.y <= 0.5 && appState.PreviousLeftJoystick.y > 0.5) || (appState.RightJoystick.y <= 0.5 && appState.PreviousRightJoystick.y > 0.5))
				{
					AddKeyInput(K_UPARROW, false);
				}
				if ((appState.LeftJoystick.y < -0.5 && appState.PreviousLeftJoystick.y >= -0.5) || (appState.RightJoystick.y < -0.5 && appState.PreviousRightJoystick.y >= -0.5))
				{
					AddKeyInput(K_DOWNARROW, true);
				}
				if ((appState.LeftJoystick.y >= -0.5 && appState.PreviousLeftJoystick.y < -0.5) || (appState.RightJoystick.y >= -0.5 && appState.PreviousRightJoystick.y < -0.5))
				{
					AddKeyInput(K_DOWNARROW, false);
				}
				if ((appState.LeftJoystick.x > 0.5 && appState.PreviousLeftJoystick.x <= 0.5) || (appState.RightJoystick.x > 0.5 && appState.PreviousRightJoystick.x <= 0.5))
				{
					AddKeyInput(K_RIGHTARROW, true);
				}
				if ((appState.LeftJoystick.x <= 0.5 && appState.PreviousLeftJoystick.x > 0.5) || (appState.RightJoystick.x <= 0.5 && appState.PreviousRightJoystick.x > 0.5))
				{
					AddKeyInput(K_RIGHTARROW, false);
				}
				if ((appState.LeftJoystick.x < -0.5 && appState.PreviousLeftJoystick.x >= -0.5) || (appState.RightJoystick.x < -0.5 && appState.PreviousRightJoystick.x >= -0.5))
				{
					AddKeyInput(K_LEFTARROW, true);
				}
				if ((appState.LeftJoystick.x >= -0.5 && appState.PreviousLeftJoystick.x < -0.5) || (appState.RightJoystick.x >= -0.5 && appState.PreviousRightJoystick.x < -0.5))
				{
					AddKeyInput(K_LEFTARROW, false);
				}
				if (LeftButtonIsDown(appState, ovrButton_Trigger) || LeftButtonIsDown(appState, ovrButton_X) || RightButtonIsDown(appState, ovrButton_Trigger) || RightButtonIsDown(appState, ovrButton_A))
				{
					AddKeyInput(K_ENTER, true);
				}
				if (LeftButtonIsUp(appState, ovrButton_Trigger) || LeftButtonIsUp(appState, ovrButton_X) || RightButtonIsUp(appState, ovrButton_Trigger) || RightButtonIsUp(appState, ovrButton_A))
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
				}
			}
		}
	}
}