#pragma once

#include <mutex>

struct Locks
{
	static bool StopEngine;

	static std::mutex InputMutex;
	static std::mutex RenderMutex;
	static std::mutex DirectRectMutex;
	static std::mutex SoundMutex;
};
