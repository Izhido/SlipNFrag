#pragma once

#include <vulkan/vulkan.h>
#include "SharedMemory.h"

struct AppState;

struct SharedMemoryBuffer
{
	SharedMemoryBuffer* next = nullptr;
	int unusedCount = 0;
	VkDeviceSize size = 0;
	VkBuffer buffer = VK_NULL_HANDLE;
	SharedMemory* sharedMemory = nullptr;

	void Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void CreateVertexBuffer(AppState& appState, VkDeviceSize size);
	void Delete(AppState& appState);
};
