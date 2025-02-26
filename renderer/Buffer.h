#pragma once

#include <vulkan/vulkan.h>

struct AppState;

struct Buffer
{
	Buffer* next;
	int unusedCount;
	VkDeviceSize size;
	VkBuffer buffer;
	VkDeviceMemory memory;
	void* mapped;

	void Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void CreateHostVisibleVertexBuffer(AppState& appState, VkDeviceSize size);
	void CreateHostVisibleStorageBuffer(AppState& appState, VkDeviceSize size);
	void CreateSourceBuffer(AppState& appState, VkDeviceSize size);
	void Delete(AppState& appState) const;
};
