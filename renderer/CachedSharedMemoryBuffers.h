#pragma once

#include "SharedMemoryBuffer.h"
#include <list>

struct CachedSharedMemoryBuffers
{
	std::list<SharedMemoryBuffer*> buffers;
	std::list<SharedMemoryBuffer*> oldBuffers;

	SharedMemoryBuffer* Get(VkDeviceSize size);
	static VkDeviceSize MinimumAllocationFor(VkDeviceSize size);
	SharedMemoryBuffer* GetIndexBuffer(AppState& appState, VkDeviceSize size);
	void Reset(AppState& appState);
	void MoveToFront(SharedMemoryBuffer* buffer);
	void Delete(AppState& appState);
};
