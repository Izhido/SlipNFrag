#pragma once

#include "Buffer.h"

struct CachedBuffers
{
	static const int minimumAllocation = 4096;

	Buffer* buffers = nullptr;
	Buffer* oldBuffers = nullptr;

	Buffer* Get(AppState& appState, VkDeviceSize size);
	static VkDeviceSize MinimumAllocationFor(VkDeviceSize size);
	static VkDeviceSize MinimumStorageAllocationFor(VkDeviceSize size);
	Buffer* GetVertexBuffer(AppState& appState, VkDeviceSize size);
	Buffer* GetIndexBuffer(AppState& appState, VkDeviceSize size);
	Buffer* GetStorageBuffer(AppState& appState, VkDeviceSize size);
	void DeleteOld(AppState& appState);
	void DisposeFront();
	void Reset(AppState& appState);
	void MoveToFront(Buffer* buffer);
	void Delete(AppState& appState) const;
};
