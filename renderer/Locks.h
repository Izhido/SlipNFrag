#pragma once

#include <mutex>

struct Locks
{
	static std::mutex ModeChangeMutex;
	static std::mutex InputMutex;
	static std::mutex RenderInputMutex;
	static std::mutex ClearMutex;
	static std::mutex ListsMutex;
	static std::mutex DirectRectMutex;
	static std::mutex SoundMutex;
    static std::mutex SysPrintMutex;
};
