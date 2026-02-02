#pragma once

#include "Buffer.h"
#include <list>

struct CachedBuffers
{
	std::list<Buffer*> buffers;
	std::list<Buffer*> oldBuffers;

	Buffer* Get(VkDeviceSize size);
	static VkDeviceSize MinimumAllocationFor(VkDeviceSize size);
	Buffer* GetStagingBuffer(AppState& appState, VkDeviceSize size);
	Buffer* GetVertexBuffer(AppState& appState, VkDeviceSize size);
	Buffer* GetMappableVertexBuffer(AppState& appState, VkDeviceSize size);
	Buffer* GetIndexBuffer(AppState& appState, VkDeviceSize size);
	Buffer* GetMappableIndexBuffer(AppState& appState, VkDeviceSize size);
	Buffer* GetMappableStorageBuffer(AppState& appState, VkDeviceSize size);
	void Reset(AppState& appState);
	void MoveToFront(Buffer* buffer);
	void Delete(AppState& appState);
};
