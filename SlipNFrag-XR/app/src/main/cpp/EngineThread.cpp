#include "EngineThread.h"
#include "AppState.h"
#include "sys_oxr.h"
#include "Input.h"
#include "r_local.h"
#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>
#include "Utils.h"
#include <android_native_app_glue.h>
//#include <android/log.h>
#include "Locks.h"

void runEngine(AppState* appState, struct android_app* app)
{
	prctl(PR_SET_NAME, (long)"runEngine", 0, 0, 0);

	appState->EngineThreadId = gettid();
	CHECK_XRCMD(appState->xrSetAndroidApplicationThreadKHR(appState->Session, XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR, appState->EngineThreadId));

	while (!appState->EngineThreadStopped)
	{
		if (!host_initialized)
		{
			std::this_thread::yield();
			continue;
		}
		{
			std::lock_guard<std::mutex> lock(Locks::InputMutex);
			for (auto i = 0; i <= Input::lastInputQueueItem; i++)
			{
				auto& input = Input::inputQueue[i];
				if (input.key > 0)
				{
					Key_Event(input.key, input.down);
				}
				if (!input.command.empty())
				{
					Cmd_ExecuteString(input.command.c_str(), src_command);
				}
			}
			Input::lastInputQueueItem = -1;
		}
		AppMode mode;
		{
			std::lock_guard<std::mutex> lock(Locks::ModeChangeMutex);
			mode = appState->Mode;
		}
		if (mode == AppScreenMode)
		{
			if (appState->PreviousTime < 0)
			{
				appState->PreviousTime = GetTime();
			}
			else if (appState->CurrentTime < 0)
			{
				appState->CurrentTime = GetTime();
			}
			else
			{
				appState->PreviousTime = appState->CurrentTime;
				appState->CurrentTime = GetTime();
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
				appState->CallExitFunction = true;
				break;
			}
			if (updated)
			{
				std::lock_guard<std::mutex> lock(Locks::RenderMutex);
				if (appState->Scene.hostClearCount != host_clearcount)
				{
					appState->Scene.Reset();
					appState->Scene.hostClearCount = host_clearcount;
				}
				r_modelorg_delta[0] = 0;
				r_modelorg_delta[1] = 0;
				r_modelorg_delta[2] = 0;
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
				std::lock_guard<std::mutex> lock(Locks::RenderInputMutex);
				cl.viewangles[YAW] = appState->Yaw * 180 / M_PI + 90;
				cl.viewangles[PITCH] = -appState->Pitch * 180 / M_PI;
				cl.viewangles[ROLL] = -appState->Roll * 180 / M_PI;
				positionX = appState->CameraLocation.pose.position.x;
				positionY = appState->CameraLocation.pose.position.y;
				positionZ = appState->CameraLocation.pose.position.z;
				scale = appState->Scale;
			}
			if (appState->PreviousTime < 0)
			{
				appState->PreviousTime = GetTime();
			}
			else if (appState->CurrentTime < 0)
			{
				appState->CurrentTime = GetTime();
			}
			else
			{
				appState->PreviousTime = appState->CurrentTime;
				appState->CurrentTime = GetTime();
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
			//auto updatestart = GetTime();
			auto updated = Host_FrameUpdate(frame_lapse);
			//auto updateend = GetTime();
			if (sys_quitcalled || sys_errormessage.length() > 0)
			{
				ANativeActivity_finish(app->activity);
				appState->CallExitFunction = true;
				break;
			}
			//double renderstart = 0;
			//double renderend = 0;
			if (updated)
			{
				std::lock_guard<std::mutex> lock(Locks::RenderMutex);
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
				appState->NearViewmodel = (distanceSquared < 12 * 12);
				d_awayfromviewmodel = !appState->NearViewmodel;
				auto nodrift = cl.nodrift;
				cl.nodrift = true;
				//renderstart = GetTime();
				Host_FrameRender();
				//renderend = GetTime();
				cl.nodrift = nodrift;
			}
			//auto finishstart = GetTime();
			Host_FrameFinish(updated);
			//auto finishend = GetTime();
			//auto elapsed = (updateend-updatestart) + (renderend-renderstart) + (finishend-finishstart);
			//__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "runEngine() elapsed: %.3f", elapsed);
		}
		std::this_thread::yield();
	}
}
