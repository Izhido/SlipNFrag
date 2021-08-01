#include "EngineThread.h"
#include "AppState.h"
#include "Time.h"
#include "sys_ovr.h"
#include "Input.h"
#include "r_local.h"
//#include <android/log.h>

void runEngine(AppState* appState, struct android_app* app)
{
	while (!appState->EngineThreadStopped)
	{
		if (!host_initialized)
		{
			std::this_thread::yield();
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
			float positionX;
			float positionY;
			float positionZ;
			float scale;
			{
				std::lock_guard<std::mutex> lock(appState->RenderInputMutex);
				cl.viewangles[YAW] = appState->Yaw * 180 / M_PI + 90;
				cl.viewangles[PITCH] = -appState->Pitch * 180 / M_PI;
				cl.viewangles[ROLL] = -appState->Roll * 180 / M_PI;
				positionX = appState->PositionX;
				positionY = appState->PositionY;
				positionZ = appState->PositionZ;
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
			//auto startTime = Sys_FloatTime();
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
				r_modelorg_delta[0] = positionX / scale;
				r_modelorg_delta[1] = -positionZ / scale;
				r_modelorg_delta[2] = positionY / scale;
				auto distanceSquared = r_modelorg_delta[0] * r_modelorg_delta[0] + r_modelorg_delta[1] * r_modelorg_delta[1] + r_modelorg_delta[2] * r_modelorg_delta[2];
				appState->NearViewModel = (distanceSquared < 12 * 12);
				d_awayfromviewmodel = !appState->NearViewModel;
				auto nodrift = cl.nodrift;
				cl.nodrift = true;
				Host_FrameRender();
				cl.nodrift = nodrift;
			}
			Host_FrameFinish(updated);
			//auto endTime = Sys_FloatTime();
			//auto elapsed = endTime - startTime;
			//__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "EngineThread::runEngine(): %.6f s.", elapsed);
		}
		std::this_thread::yield();
	}
}
