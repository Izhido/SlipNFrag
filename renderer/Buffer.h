#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct AppState;

struct Buffer
{
	int unusedCount;
	VkDeviceSize size;
	VkBuffer buffer;
	VmaAllocation allocation;
	void* mapped;

	void Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage);
	void CreateStagingBuffer(AppState& appState, VkDeviceSize size);
	void CreateHostVisibleVertexBuffer(AppState& appState, VkDeviceSize size);
	void CreateHostVisibleStorageBuffer(AppState& appState, VkDeviceSize size);
	void CreateHostVisibleIndexBuffer(AppState& appState, VkDeviceSize size);
	void CreateHostVisibleUniformBuffer(AppState& appState, VkDeviceSize size);
	void Map(AppState& appState);
	void UnmapAndFlush(AppState& appState) const;
	void Delete(AppState& appState) const;
};
