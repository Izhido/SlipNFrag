#pragma once

#include "Buffer.h"

struct CachedBuffers
{
	Buffer* buffers = nullptr;
	Buffer* oldBuffers = nullptr;

	Buffer* Get(AppState& appState, VkDeviceSize size);
	static VkDeviceSize MinimumAllocationFor(VkDeviceSize size);
	Buffer* GetVertexBuffer(AppState& appState, VkDeviceSize size);
	Buffer* GetHostVisibleVertexBuffer(AppState& appState, VkDeviceSize size);
	Buffer* GetIndexBuffer(AppState& appState, VkDeviceSize size);
	Buffer* GetHostVisibleStorageBuffer(AppState& appState, VkDeviceSize size);
    Buffer* GetStorageBuffer(AppState& appState, VkDeviceSize size);
	void DeleteOld(AppState& appState);
	void DisposeFront();
	void Reset(AppState& appState);
	void MoveToFront(Buffer* buffer);
	void Delete(AppState& appState) const;
};
