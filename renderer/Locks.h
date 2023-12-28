#pragma once

#include <mutex>

struct Locks
{
	static std::mutex ModeChangeMutex;
	static std::mutex InputMutex;
	static std::mutex RenderInputMutex;
	static std::mutex RenderMutex;
	static std::mutex DirectRectMutex;
	static std::mutex SoundMutex;
};
