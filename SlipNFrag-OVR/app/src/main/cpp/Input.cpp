#include "Input.h"
#include "AppState.h"
#include "VrApi_Input.h"
#include "vid_ovr.h"
#include "in_ovr.h"
#include "d_lists.h"

extern m_state_t m_state;

bool leftButtonIsDown(AppState& appState, uint32_t button)
{
	return ((appState.LeftButtons & button) != 0 && (appState.PreviousLeftButtons & button) == 0);
}

bool leftButtonIsUp(AppState& appState, uint32_t button)
{
	return ((appState.LeftButtons & button) == 0 && (appState.PreviousLeftButtons & button) != 0);
}

bool rightButtonIsDown(AppState& appState, uint32_t button)
{
	return ((appState.RightButtons & button) != 0 && (appState.PreviousRightButtons & button) == 0);
}

bool rightButtonIsUp(AppState& appState, uint32_t button)
{
	return ((appState.RightButtons & button) == 0 && (appState.PreviousRightButtons & button) != 0);
}

void handleInput(AppState& appState)
{
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
			if (leftButtonIsDown(appState, ovrButton_Enter))
			{
				Key_Event(K_ESCAPE, true);
			}
			if (leftButtonIsUp(appState, ovrButton_Enter))
			{
				Key_Event(K_ESCAPE, false);
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
				if (leftButtonIsDown(appState, ovrButton_Trigger) || rightButtonIsDown(appState, ovrButton_Trigger))
				{
					Cmd_ExecuteString("+attack", src_command);
				}
				if (leftButtonIsUp(appState, ovrButton_Trigger) || rightButtonIsUp(appState, ovrButton_Trigger))
				{
					Cmd_ExecuteString("-attack", src_command);
				}
				if (leftButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_GripTrigger))
				{
					Cmd_ExecuteString("+speed", src_command);
				}
				if (leftButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_GripTrigger))
				{
					Cmd_ExecuteString("-speed", src_command);
				}
				if (leftButtonIsDown(appState, ovrButton_Joystick))
				{
					Cmd_ExecuteString("impulse 10", src_command);
				}
				if (m_state == m_quit)
				{
					if (rightButtonIsDown(appState, ovrButton_B))
					{
						Cmd_ExecuteString("+jump", src_command);
					}
					if (rightButtonIsUp(appState, ovrButton_B))
					{
						Cmd_ExecuteString("-jump", src_command);
					}
					if (leftButtonIsDown(appState, ovrButton_Y))
					{
						Key_Event('y', true);
					}
					if (leftButtonIsUp(appState, ovrButton_Y))
					{
						Key_Event('y', false);
					}
				}
				else
				{
					if (leftButtonIsDown(appState, ovrButton_Y) || rightButtonIsDown(appState, ovrButton_B))
					{
						Cmd_ExecuteString("+jump", src_command);
					}
					if (leftButtonIsUp(appState, ovrButton_Y) || rightButtonIsUp(appState, ovrButton_B))
					{
						Cmd_ExecuteString("-jump", src_command);
					}
				}
				if (leftButtonIsDown(appState, ovrButton_X) || rightButtonIsDown(appState, ovrButton_A))
				{
					Cmd_ExecuteString("+movedown", src_command);
				}
				if (leftButtonIsUp(appState, ovrButton_X) || rightButtonIsUp(appState, ovrButton_A))
				{
					Cmd_ExecuteString("-movedown", src_command);
				}
				if (rightButtonIsDown(appState, ovrButton_Joystick))
				{
					Cmd_ExecuteString("+mlook", src_command);
				}
				if (rightButtonIsUp(appState, ovrButton_Joystick))
				{
					Cmd_ExecuteString("-mlook", src_command);
				}
			}
			else
			{
				if ((appState.LeftJoystick.y > 0.5 && appState.PreviousLeftJoystick.y <= 0.5) || (appState.RightJoystick.y > 0.5 && appState.PreviousRightJoystick.y <= 0.5))
				{
					Key_Event(K_UPARROW, true);
				}
				if ((appState.LeftJoystick.y <= 0.5 && appState.PreviousLeftJoystick.y > 0.5) || (appState.RightJoystick.y <= 0.5 && appState.PreviousRightJoystick.y > 0.5))
				{
					Key_Event(K_UPARROW, false);
				}
				if ((appState.LeftJoystick.y < -0.5 && appState.PreviousLeftJoystick.y >= -0.5) || (appState.RightJoystick.y < -0.5 && appState.PreviousRightJoystick.y >= -0.5))
				{
					Key_Event(K_DOWNARROW, true);
				}
				if ((appState.LeftJoystick.y >= -0.5 && appState.PreviousLeftJoystick.y < -0.5) || (appState.RightJoystick.y >= -0.5 && appState.PreviousRightJoystick.y < -0.5))
				{
					Key_Event(K_DOWNARROW, false);
				}
				if ((appState.LeftJoystick.x > 0.5 && appState.PreviousLeftJoystick.x <= 0.5) || (appState.RightJoystick.x > 0.5 && appState.PreviousRightJoystick.x <= 0.5))
				{
					Key_Event(K_RIGHTARROW, true);
				}
				if ((appState.LeftJoystick.x <= 0.5 && appState.PreviousLeftJoystick.x > 0.5) || (appState.RightJoystick.x <= 0.5 && appState.PreviousRightJoystick.x > 0.5))
				{
					Key_Event(K_RIGHTARROW, false);
				}
				if ((appState.LeftJoystick.x < -0.5 && appState.PreviousLeftJoystick.x >= -0.5) || (appState.RightJoystick.x < -0.5 && appState.PreviousRightJoystick.x >= -0.5))
				{
					Key_Event(K_LEFTARROW, true);
				}
				if ((appState.LeftJoystick.x >= -0.5 && appState.PreviousLeftJoystick.x < -0.5) || (appState.RightJoystick.x >= -0.5 && appState.PreviousRightJoystick.x < -0.5))
				{
					Key_Event(K_LEFTARROW, false);
				}
				if (leftButtonIsDown(appState, ovrButton_Trigger) || leftButtonIsDown(appState, ovrButton_X) || rightButtonIsDown(appState, ovrButton_Trigger) || rightButtonIsDown(appState, ovrButton_A))
				{
					Key_Event(K_ENTER, true);
				}
				if (leftButtonIsUp(appState, ovrButton_Trigger) || leftButtonIsUp(appState, ovrButton_X) || rightButtonIsUp(appState, ovrButton_Trigger) || rightButtonIsUp(appState, ovrButton_A))
				{
					Key_Event(K_ENTER, false);
				}
				if (leftButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_B))
				{
					Key_Event(K_ESCAPE, true);
				}
				if (leftButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_B))
				{
					Key_Event(K_ESCAPE, false);
				}
				if (m_state == m_quit)
				{
					if (leftButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_B))
					{
						Key_Event(K_ESCAPE, true);
					}
					if (leftButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_B))
					{
						Key_Event(K_ESCAPE, false);
					}
					if (leftButtonIsDown(appState, ovrButton_Y))
					{
						Key_Event('y', true);
					}
					if (leftButtonIsUp(appState, ovrButton_Y))
					{
						Key_Event('y', false);
					}
				}
				else
				{
					if (leftButtonIsDown(appState, ovrButton_GripTrigger) || leftButtonIsDown(appState, ovrButton_Y) || rightButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_B))
					{
						Key_Event(K_ESCAPE, true);
					}
					if (leftButtonIsUp(appState, ovrButton_GripTrigger) || leftButtonIsUp(appState, ovrButton_Y) || rightButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_B))
					{
						Key_Event(K_ESCAPE, false);
					}
				}
			}
		}
	}
	else if (appState.Mode == AppWorldMode)
	{
		if (host_initialized)
		{
			if (leftButtonIsDown(appState, ovrButton_Enter))
			{
				Key_Event(K_ESCAPE, true);
			}
			if (leftButtonIsUp(appState, ovrButton_Enter))
			{
				Key_Event(K_ESCAPE, false);
			}
			if (key_dest == key_game)
			{
				if (joy_initialized)
				{
					joy_avail = true;
					pdwRawValue[JOY_AXIS_X] = -appState.LeftJoystick.x - appState.RightJoystick.x;
					pdwRawValue[JOY_AXIS_Y] = -appState.LeftJoystick.y - appState.RightJoystick.y;
				}
				if (leftButtonIsDown(appState, ovrButton_Trigger) || rightButtonIsDown(appState, ovrButton_Trigger))
				{
					if (!appState.ControlsMessageClosed && appState.ControlsMessageDisplayed && (appState.NearViewModel || d_lists.last_viewmodel < 0))
					{
						SCR_Interrupt();
						appState.ControlsMessageClosed = true;
					}
					else if (appState.NearViewModel || d_lists.last_viewmodel < 0)
					{
						Cmd_ExecuteString("+attack", src_command);
					}
				}
				if (leftButtonIsUp(appState, ovrButton_Trigger) || rightButtonIsUp(appState, ovrButton_Trigger))
				{
					Cmd_ExecuteString("-attack", src_command);
				}
				if (leftButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_GripTrigger))
				{
					Cmd_ExecuteString("+speed", src_command);
				}
				if (leftButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_GripTrigger))
				{
					Cmd_ExecuteString("-speed", src_command);
				}
				if (leftButtonIsDown(appState, ovrButton_Joystick) || rightButtonIsDown(appState, ovrButton_Joystick))
				{
					Cmd_ExecuteString("impulse 10", src_command);
				}
				if (m_state == m_quit)
				{
					if (rightButtonIsDown(appState, ovrButton_B))
					{
						Cmd_ExecuteString("+jump", src_command);
					}
					if (rightButtonIsUp(appState, ovrButton_B))
					{
						Cmd_ExecuteString("-jump", src_command);
					}
					if (leftButtonIsDown(appState, ovrButton_Y))
					{
						Key_Event('y', true);
					}
					if (leftButtonIsUp(appState, ovrButton_Y))
					{
						Key_Event('y', false);
					}
				}
				else
				{
					if (leftButtonIsDown(appState, ovrButton_Y) || rightButtonIsDown(appState, ovrButton_B))
					{
						Cmd_ExecuteString("+jump", src_command);
					}
					if (leftButtonIsUp(appState, ovrButton_Y) || rightButtonIsUp(appState, ovrButton_B))
					{
						Cmd_ExecuteString("-jump", src_command);
					}
				}
				if (leftButtonIsDown(appState, ovrButton_X) || rightButtonIsDown(appState, ovrButton_A))
				{
					Cmd_ExecuteString("+movedown", src_command);
				}
				if (leftButtonIsUp(appState, ovrButton_X) || rightButtonIsUp(appState, ovrButton_A))
				{
					Cmd_ExecuteString("-movedown", src_command);
				}
			}
			else
			{
				if ((appState.LeftJoystick.y > 0.5 && appState.PreviousLeftJoystick.y <= 0.5) || (appState.RightJoystick.y > 0.5 && appState.PreviousRightJoystick.y <= 0.5))
				{
					Key_Event(K_UPARROW, true);
				}
				if ((appState.LeftJoystick.y <= 0.5 && appState.PreviousLeftJoystick.y > 0.5) || (appState.RightJoystick.y <= 0.5 && appState.PreviousRightJoystick.y > 0.5))
				{
					Key_Event(K_UPARROW, false);
				}
				if ((appState.LeftJoystick.y < -0.5 && appState.PreviousLeftJoystick.y >= -0.5) || (appState.RightJoystick.y < -0.5 && appState.PreviousRightJoystick.y >= -0.5))
				{
					Key_Event(K_DOWNARROW, true);
				}
				if ((appState.LeftJoystick.y >= -0.5 && appState.PreviousLeftJoystick.y < -0.5) || (appState.RightJoystick.y >= -0.5 && appState.PreviousRightJoystick.y < -0.5))
				{
					Key_Event(K_DOWNARROW, false);
				}
				if ((appState.LeftJoystick.x > 0.5 && appState.PreviousLeftJoystick.x <= 0.5) || (appState.RightJoystick.x > 0.5 && appState.PreviousRightJoystick.x <= 0.5))
				{
					Key_Event(K_RIGHTARROW, true);
				}
				if ((appState.LeftJoystick.x <= 0.5 && appState.PreviousLeftJoystick.x > 0.5) || (appState.RightJoystick.x <= 0.5 && appState.PreviousRightJoystick.x > 0.5))
				{
					Key_Event(K_RIGHTARROW, false);
				}
				if ((appState.LeftJoystick.x < -0.5 && appState.PreviousLeftJoystick.x >= -0.5) || (appState.RightJoystick.x < -0.5 && appState.PreviousRightJoystick.x >= -0.5))
				{
					Key_Event(K_LEFTARROW, true);
				}
				if ((appState.LeftJoystick.x >= -0.5 && appState.PreviousLeftJoystick.x < -0.5) || (appState.RightJoystick.x >= -0.5 && appState.PreviousRightJoystick.x < -0.5))
				{
					Key_Event(K_LEFTARROW, false);
				}
				if (leftButtonIsDown(appState, ovrButton_Trigger) || leftButtonIsDown(appState, ovrButton_X) || rightButtonIsDown(appState, ovrButton_Trigger) || rightButtonIsDown(appState, ovrButton_A))
				{
					Key_Event(K_ENTER, true);
				}
				if (leftButtonIsUp(appState, ovrButton_Trigger) || leftButtonIsUp(appState, ovrButton_X) || rightButtonIsUp(appState, ovrButton_Trigger) || rightButtonIsUp(appState, ovrButton_A))
				{
					Key_Event(K_ENTER, false);
				}
				if (m_state == m_quit)
				{
					if (leftButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_B))
					{
						Key_Event(K_ESCAPE, true);
					}
					if (leftButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_B))
					{
						Key_Event(K_ESCAPE, false);
					}
					if (leftButtonIsDown(appState, ovrButton_Y))
					{
						Key_Event('y', true);
					}
					if (leftButtonIsUp(appState, ovrButton_Y))
					{
						Key_Event('y', false);
					}
				}
				else
				{
					if (leftButtonIsDown(appState, ovrButton_GripTrigger) || leftButtonIsDown(appState, ovrButton_Y) || rightButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_B))
					{
						Key_Event(K_ESCAPE, true);
					}
					if (leftButtonIsUp(appState, ovrButton_GripTrigger) || leftButtonIsUp(appState, ovrButton_Y) || rightButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_B))
					{
						Key_Event(K_ESCAPE, false);
					}
				}
			}
		}
	}
}