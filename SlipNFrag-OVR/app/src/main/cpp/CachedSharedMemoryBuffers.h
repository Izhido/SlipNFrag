#pragma once

#include "SharedMemoryBuffer.h"

struct CachedSharedMemoryBuffers
{
	SharedMemoryBuffer* buffers = nullptr;
	SharedMemoryBuffer* oldBuffers = nullptr;

	void DisposeFront();
	void MoveToFront(SharedMemoryBuffer* buffer);
	void Delete(AppState& appState);
};
