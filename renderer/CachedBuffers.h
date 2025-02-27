#pragma once

#include "Buffer.h"

struct CachedBuffers
{
	Buffer* buffers = nullptr;
	Buffer* oldBuffers = nullptr;

	Buffer* Get(AppState& appState, VkDeviceSize size);
	static VkDeviceSize MinimumAllocationFor(VkDeviceSize size);
	Buffer* GetHostVisibleVertexBuffer(AppState& appState, VkDeviceSize size);
	Buffer* GetHostVisibleStorageBuffer(AppState& appState, VkDeviceSize size);
	void Reset(AppState& appState);
	void MoveToFront(Buffer* buffer);
	void Delete(AppState& appState) const;
};
