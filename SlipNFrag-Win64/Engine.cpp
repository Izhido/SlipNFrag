#include "Engine.h"
#include "sys_win64.h"
#include "vid_win64.h"
#include "Locks.h"
#include "Input.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void Engine::StartEngine(std::vector<std::string> arguments)
{
	sys_version = "Win64 1.0.22";
    sys_argc = (int)arguments.size();
    sys_argv = new char* [sys_argc];
    for (auto i = 0; i < arguments.size(); i++)
    {
        sys_argv[i] = new char[arguments[i].length() + 1];
        strcpy(sys_argv[i], arguments[i].c_str());
    }

    Sys_Init(sys_argc, sys_argv);

    if (sys_errormessage.length() > 0)
    {
        Locks::StopEngine = true;
        return;
    }

	double previousTime = -1;
	double currentTime = -1;

	while (!Locks::StopEngine)
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
				auto key = Input::inputQueueKeys[i];
				if (key > 0)
				{
					Key_Event(key, (qboolean)Input::inputQueuePressed[i]);
				}
				auto& command = Input::inputQueueCommands[i];
				if (!command.empty())
				{
					Cmd_ExecuteString(command.c_str(), src_command);
				}
			}
			Input::lastInputQueueItem = -1;
		}

		LARGE_INTEGER time;
		QueryPerformanceCounter(&time);
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		auto capturedTime = (double)time.QuadPart / (double)frequency.QuadPart;

		if (previousTime < 0)
		{
			previousTime = capturedTime;
		}
		else if (currentTime < 0)
		{
			currentTime = capturedTime;
		}
		else
		{
			previousTime = currentTime;
			currentTime = capturedTime;
			frame_lapse = currentTime - previousTime;
		}

		if (r_cache_thrash)
		{
			std::lock_guard<std::mutex> lock(Locks::RenderMutex);

			VID_ReallocSurfCache();
		}

		auto updated = Host_FrameUpdate(frame_lapse);

		if (sys_quitcalled || sys_errormessage.length() > 0)
		{
			Locks::StopEngine = true;
			return;
		}

		if (updated)
		{
			std::lock_guard<std::mutex> lock(Locks::RenderMutex);

			Host_FrameRender();
		}
		Host_FrameFinish(updated);

		std::this_thread::yield();
	}
}