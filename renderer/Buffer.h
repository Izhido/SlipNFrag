#pragma once

#include <vulkan/vulkan.h>

struct AppState;

struct Buffer
{
	Buffer* next = nullptr;
	int unusedCount = 0;
	VkDeviceSize size = 0;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	void* mapped = nullptr;

	void Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void CreateHostVisibleVertexBuffer(AppState& appState, VkDeviceSize size);
	void CreateHostVisibleStorageBuffer(AppState& appState, VkDeviceSize size);
	void CreateSourceBuffer(AppState& appState, VkDeviceSize size);
	void Delete(AppState& appState) const;
};
