#include "EngineThread.h"
#include "AppState_pcxr.h"
#include "sys_pcxr.h"
#include "vid_pcxr.h"
#include "AppInput.h"
#include "r_local.h"
#include "Utils.h"
#include "Locks.h"

void runEngine(AppState_pcxr* appState)
{
	while (!appState->EngineThreadStopped)
	{
		if (!host_initialized)
		{
			std::this_thread::yield();
			continue;
		}
		{
			std::lock_guard<std::mutex> lock(Locks::InputMutex);
			for (auto i = 0; i <= AppInput::lastInputQueueItem; i++)
			{
				auto& input = AppInput::inputQueue[i];
				if (input.key > 0)
				{
					Key_Event(input.key, input.down);
				}
				if (!input.command.empty())
				{
					Cmd_ExecuteString(input.command.c_str(), src_command);
				}
			}
			AppInput::lastInputQueueItem = -1;
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
			cl_allow_immersive = false;
			auto updated = Host_FrameUpdate(frame_lapse);
			if (sys_quitcalled || !sys_errormessage.empty())
			{
				appState->DestroyRequested = true;
				break;
			}
			if (updated)
			{
				std::lock_guard<std::mutex> lock(Locks::RenderMutex);
				if (appState->Scene.hostClearCount != host_clearcount)
				{
					appState->Scene.Reset(*appState);
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
			float yaw;
			float pitch;
			float roll;
			float originDeltaX;
			float originDeltaY;
			float originDeltaZ;
			bool handsAvailable;
			float leftHandYaw;
			float leftHandPitch;
			float leftHandRoll;
			float leftHandDeltaX;
			float leftHandDeltaY;
			float leftHandDeltaZ;
			float rightHandYaw;
			float rightHandPitch;
			float rightHandRoll;
			float rightHandDeltaX;
			float rightHandDeltaY;
			float rightHandDeltaZ;
			{
				std::lock_guard<std::mutex> lock(Locks::RenderInputMutex);
				yaw = appState->Yaw * 180 / M_PI + 90;
				pitch = -appState->Pitch * 180 / M_PI;
				roll = -appState->Roll * 180 / M_PI;
				auto scale = appState->Scale;
				originDeltaX = appState->CameraLocation.pose.position.x / scale;
				originDeltaY = -appState->CameraLocation.pose.position.z / scale;
				originDeltaZ = appState->CameraLocation.pose.position.y / scale;
				handsAvailable = ((Cvar_VariableValue("hands_enabled") != 0) && appState->LeftController.PoseIsValid && appState->RightController.PoseIsValid);
				if (handsAvailable)
				{
					AppState::AnglesFromQuaternion(appState->LeftController.SpaceLocation.pose.orientation, leftHandYaw, leftHandPitch, leftHandRoll);
					leftHandYaw = leftHandYaw * 180 / M_PI + 90;
					leftHandPitch = -leftHandPitch * 180 / M_PI;
					leftHandRoll = -leftHandRoll * 180 / M_PI;
					leftHandDeltaX = appState->LeftController.SpaceLocation.pose.position.x / scale;
					leftHandDeltaY = -appState->LeftController.SpaceLocation.pose.position.z / scale;
					leftHandDeltaZ = appState->LeftController.SpaceLocation.pose.position.y / scale;
					AppState::AnglesFromQuaternion(appState->RightController.SpaceLocation.pose.orientation, rightHandYaw, rightHandPitch, rightHandRoll);
					rightHandYaw = rightHandYaw * 180 / M_PI + 90;
					rightHandPitch = -rightHandPitch * 180 / M_PI;
					rightHandRoll = -rightHandRoll * 180 / M_PI;
					rightHandDeltaX = appState->RightController.SpaceLocation.pose.position.x / scale;
					rightHandDeltaY = -appState->RightController.SpaceLocation.pose.position.z / scale;
					rightHandDeltaZ = appState->RightController.SpaceLocation.pose.position.y / scale;
				}
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
					SCR_InterruptableCenterPrint("Controls:\n\nLeft or Right Joysticks:\nWalk Forward / Backpedal, \n   Step Left / Step Right \n\n[B] / [Y]: Jump     \n[A] / [X]: Swim down\nTriggers: Attack  \nGrip Triggers: Run          \nClick Joysticks: Change Weapon  \n\nFire weapon to close");
					appState->ControlsMessageDisplayed = true;
				}
			}
			if (r_cache_thrash)
			{
				VID_ReallocSurfCache();
			}
			cl.viewangles[YAW] = yaw;
			cl.viewangles[PITCH] = pitch;
			cl.viewangles[ROLL] = roll;
			cl_allow_immersive = (Cvar_VariableValue("immersive_enabled") != 0);
			if (cl_allow_immersive)
			{
				cl_immersive_origin_delta[0] = originDeltaX;
				cl_immersive_origin_delta[1] = originDeltaY;
				cl_immersive_origin_delta[2] = originDeltaZ;
				cl_immersive_hands_available = handsAvailable;
				if (cl_immersive_hands_available)
				{
					cl_immersive_left_hand_delta[0] = leftHandDeltaX;
					cl_immersive_left_hand_delta[1] = leftHandDeltaY;
					cl_immersive_left_hand_delta[2] = leftHandDeltaZ;
					cl_immersive_left_hand_angles[YAW] = leftHandYaw;
					cl_immersive_left_hand_angles[PITCH] = leftHandPitch;
					cl_immersive_left_hand_angles[ROLL] = leftHandRoll;
					cl_immersive_right_hand_delta[0] = rightHandDeltaX;
					cl_immersive_right_hand_delta[1] = rightHandDeltaY;
					cl_immersive_right_hand_delta[2] = rightHandDeltaZ;
					cl_immersive_right_hand_angles[YAW] = rightHandYaw;
					cl_immersive_right_hand_angles[PITCH] = rightHandPitch;
					cl_immersive_right_hand_angles[ROLL] = rightHandRoll;
				}
			}
			auto updated = Host_FrameUpdate(frame_lapse);
			// After Host_FrameUpdate() is called, view angles can change due to commands sent by the server:
			auto previousYaw = cl.viewangles[YAW];
			auto previousPitch = cl.viewangles[PITCH];
			auto previousRoll = cl.viewangles[ROLL];
			if (sys_quitcalled || !sys_errormessage.empty())
			{
				appState->DestroyRequested = true;
				break;
			}
			if (updated)
			{
				std::lock_guard<std::mutex> lock(Locks::RenderMutex);
				if (appState->Scene.hostClearCount != host_clearcount)
				{
					appState->Scene.Reset(*appState);
					appState->Scene.hostClearCount = host_clearcount;
				}
				r_modelorg_delta[0] = originDeltaX;
				r_modelorg_delta[1] = originDeltaY;
				r_modelorg_delta[2] = originDeltaZ;
				// Setting again view angles just in case the server changed them:
				cl.viewangles[YAW] = yaw;
				cl.viewangles[PITCH] = pitch;
				cl.viewangles[ROLL] = roll;
				auto nodrift = cl.nodrift;
				cl.nodrift = true;
				D_ResetLists();
				Host_FrameRender();
				cl.nodrift = nodrift;
				// Restoring potential changes performed by server in view angles,
				// to allow code outside Host_***() calls to act upon them:
				cl.viewangles[YAW] = previousYaw;
				cl.viewangles[PITCH] = previousPitch;
				cl.viewangles[ROLL] = previousRoll;
			}
			Host_FrameFinish(updated);
		}
		std::this_thread::yield();
	}
}
