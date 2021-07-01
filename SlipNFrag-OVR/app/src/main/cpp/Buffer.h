#pragma once

#include <vulkan/vulkan.h>

struct AppState;

struct Buffer
{
	Buffer* next = nullptr;
	int unusedCount = 0;
	size_t size = 0;
	VkMemoryPropertyFlags properties = 0;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	void* mapped = nullptr;

	void Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void CreateVertexBuffer(AppState& appState, VkDeviceSize size);
	void CreateIndexBuffer(AppState& appState, VkDeviceSize size);
	void CreateStagingBuffer(AppState& appState, VkDeviceSize size);
	void CreateStagingStorageBuffer(AppState& appState, VkDeviceSize size);
	void CreateUniformBuffer(AppState& appState, VkDeviceSize size);
	void Submit(AppState& appState, VkCommandBuffer commandBuffer, VkAccessFlags access);
	void SubmitVertexBuffer(AppState& appState, VkCommandBuffer commandBuffer);
	void SubmitIndexBuffer(AppState& appState, VkCommandBuffer commandBuffer);
	void Destroy(AppState& appState);
};
