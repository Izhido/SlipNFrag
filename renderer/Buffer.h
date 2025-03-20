#pragma once

#include <vulkan/vulkan.h>

struct AppState;

struct Buffer
{
	int unusedCount;
	VkDeviceSize size;
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkMemoryPropertyFlags properties;
	void* mapped;

	void Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void CreateStagingBuffer(AppState& appState, VkDeviceSize size);
	void CreateHostVisibleVertexBuffer(AppState& appState, VkDeviceSize size);
	void CreateHostVisibleStorageBuffer(AppState& appState, VkDeviceSize size);
	void CreateHostVisibleIndexBuffer(AppState& appState, VkDeviceSize size);
	void CreateHostVisibleUniformBuffer(AppState& appState, VkDeviceSize size);
	void CreateSourceBuffer(AppState& appState, VkDeviceSize size);
	void Delete(AppState& appState) const;
};
