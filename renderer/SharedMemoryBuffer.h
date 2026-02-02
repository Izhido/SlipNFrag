#pragma once

#include <vulkan/vulkan.h>
#include "SharedMemory.h"

struct AppState;

struct SharedMemoryBuffer
{
	SharedMemoryBuffer* next;
	int unusedCount;
	VkDeviceSize size;
	VkBuffer buffer;
	SharedMemory* sharedMemory;
	VkMemoryPropertyFlags properties;

	void Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void CreateStorageBuffer(AppState& appState, VkDeviceSize size);
	void Delete(AppState& appState) const;
};
