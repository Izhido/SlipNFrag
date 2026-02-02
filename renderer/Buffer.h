#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct AppState;

struct Buffer
{
	Buffer* next;
	int unusedCount;
	VkDeviceSize size;
	VkBuffer buffer;
	VmaAllocation allocation;
	void* mapped;

	void Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage, bool mappable);
	void CreateStagingBuffer(AppState& appState, VkDeviceSize size);
	void CreateVertexBuffer(AppState& appState, VkDeviceSize size);
	void CreateMappableVertexBuffer(AppState& appState, VkDeviceSize size);
	void CreateIndexBuffer(AppState& appState, VkDeviceSize size);
	void CreateMappableIndexBuffer(AppState& appState, VkDeviceSize size);
	void CreateMappableUniformBuffer(AppState& appState, VkDeviceSize size);
	void CreateMappableStorageBuffer(AppState& appState, VkDeviceSize size);
	void Map(AppState& appState);
	void UnmapAndFlush(AppState& appState) const;
	void Delete(AppState& appState) const;
};
