#pragma once

#include "Buffer.h"
#include <list>

struct CachedBuffers
{
	std::list<Buffer*> buffers;
	std::list<Buffer*> oldBuffers;

	Buffer* Get(VkDeviceSize size);
	static VkDeviceSize MinimumAllocationFor(VkDeviceSize size);
	Buffer* GetHostVisibleVertexBuffer(AppState& appState, VkDeviceSize size);
	Buffer* GetHostVisibleStorageBuffer(AppState& appState, VkDeviceSize size);
	void Reset(AppState& appState);
	void MoveToFront(Buffer* buffer);
	void Delete(AppState& appState);
};
