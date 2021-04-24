#include "EngineThread.h"
#include "vid_ovr.h"
#include "AppState.h"
#include "Time.h"
#include "sys_ovr.h"
#include "Input.h"
#include "r_local.h"

void runEngine(AppState* appState, struct android_app* app)
{
	while (!appState->EngineThreadStopped)
	{
		if (!host_initialized)
		{
			continue;
		}
		{
			std::lock_guard<std::mutex> lock(appState->InputMutex);
			for (auto i = 0; i <= Input::lastInputQueueItem; i++)
			{
				auto& input = Input::inputQueue[i];
				if (input.key > 0)
				{
					Key_Event(input.key, input.down);
				}
				if (input.command.size() > 0)
				{
					Cmd_ExecuteString(input.command.c_str(), src_command);
				}
			}
			Input::lastInputQueueItem = -1;
		}
		AppMode mode;
		{
			std::lock_guard<std::mutex> lock(appState->ModeChangeMutex);
			mode = appState->Mode;
		}
		if (mode == AppScreenMode)
		{
			if (appState->PreviousTime < 0)
			{
				appState->PreviousTime = getTime();
			}
			else if (appState->CurrentTime < 0)
			{
				appState->CurrentTime = getTime();
			}
			else
			{
				appState->PreviousTime = appState->CurrentTime;
				appState->CurrentTime = getTime();
				frame_lapse = (float) (appState->CurrentTime - appState->PreviousTime);
			}
			if (r_cache_thrash)
			{
				VID_ReallocSurfCache();
			}
			auto updated = Host_FrameUpdate(frame_lapse);
			if (sys_quitcalled || sys_errormessage.length() > 0)
			{
				ANativeActivity_finish(app->activity);
				break;
			}
			if (updated)
			{
				std::lock_guard<std::mutex> lock(appState->RenderMutex);
				if (appState->Scene.hostClearCount != host_clearcount)
				{
					appState->Scene.Reset();
					appState->Scene.hostClearCount = host_clearcount;
				}
				Host_FrameRender();
			}
			Host_FrameFinish(updated);
		}
		else if (mode == AppWorldMode)
		{
			ovrTracking2 tracking;
			float scale;
			{
				std::lock_guard<std::mutex> lock(appState->RenderInputMutex);
				cl.viewangles[YAW] = appState->Yaw * 180 / M_PI + 90;
				cl.viewangles[PITCH] = -appState->Pitch * 180 / M_PI;
				cl.viewangles[ROLL] = -appState->Roll * 180 / M_PI;
				tracking = appState->Tracking;
				scale = appState->Scale;
			}
			if (appState->PreviousTime < 0)
			{
				appState->PreviousTime = getTime();
			}
			else if (appState->CurrentTime < 0)
			{
				appState->CurrentTime = getTime();
			}
			else
			{
				appState->PreviousTime = appState->CurrentTime;
				appState->CurrentTime = getTime();
				frame_lapse = (float) (appState->CurrentTime - appState->PreviousTime);
				appState->TimeInWorldMode += frame_lapse;
				if (!appState->ControlsMessageDisplayed && appState->TimeInWorldMode > 4)
				{
					SCR_InterruptableCenterPrint("Controls:\n\nLeft or Right Joysticks:\nWalk Forward / Backpedal, \n   Step Left / Step Right \n\n[B] / [Y]: Jump     \n[A] / [X]: Swim down\nTriggers: Attack  \nGrip Triggers: Run          \nClick Joysticks: Change Weapon  \n\nApproach and fire weapon to close");
					appState->ControlsMessageDisplayed = true;
				}
			}
			if (r_cache_thrash)
			{
				VID_ReallocSurfCache();
			}
			auto updated = Host_FrameUpdate(frame_lapse);
			if (sys_quitcalled || sys_errormessage.length() > 0)
			{
				ANativeActivity_finish(app->activity);
				break;
			}
			if (updated)
			{
				std::lock_guard<std::mutex> lock(appState->RenderMutex);
				if (appState->Scene.hostClearCount != host_clearcount)
				{
					appState->Scene.Reset();
					appState->Scene.hostClearCount = host_clearcount;
				}
				else
				{
					D_ResetLists();
				}
				r_modelorg_delta[0] = tracking.HeadPose.Pose.Position.x / scale;
				r_modelorg_delta[1] = -tracking.HeadPose.Pose.Position.z / scale;
				r_modelorg_delta[2] = tracking.HeadPose.Pose.Position.y / scale;
				auto distanceSquared = r_modelorg_delta[0] * r_modelorg_delta[0] + r_modelorg_delta[1] * r_modelorg_delta[1] + r_modelorg_delta[2] * r_modelorg_delta[2];
				appState->NearViewModel = (distanceSquared < 12 * 12);
				d_awayfromviewmodel = !appState->NearViewModel;
				auto nodrift = cl.nodrift;
				cl.nodrift = true;
				Host_FrameRender();
				cl.nodrift = nodrift;
			}
			Host_FrameFinish(updated);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
