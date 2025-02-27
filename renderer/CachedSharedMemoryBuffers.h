#pragma once

#include "SharedMemoryBuffer.h"

struct CachedSharedMemoryBuffers
{
	SharedMemoryBuffer* buffers = nullptr;
	SharedMemoryBuffer* oldBuffers = nullptr;

	SharedMemoryBuffer* Get(AppState& appState, VkDeviceSize size);
	static VkDeviceSize MinimumAllocationFor(VkDeviceSize size);
	SharedMemoryBuffer* GetVertexBuffer(AppState& appState, VkDeviceSize size);
	SharedMemoryBuffer* GetIndexBuffer(AppState& appState, VkDeviceSize size);
	SharedMemoryBuffer* GetStorageBuffer(AppState& appState, VkDeviceSize size);
	void Reset(AppState& appState);
	void MoveToFront(SharedMemoryBuffer* buffer);
	void Delete(AppState& appState) const;
};
