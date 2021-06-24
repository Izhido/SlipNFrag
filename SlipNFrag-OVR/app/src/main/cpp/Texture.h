#pragma once

#include "Buffer.h"
#include "StagingBuffer.h"

struct Texture
{
	Texture* next = nullptr;
	void* key = nullptr;
	int unusedCount = 0;
	int width = 0;
	int height = 0;
	int mipCount = 0;
	int layerCount = 0;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;

	void Create(AppState& appState, VkCommandBuffer commandBuffer, uint32_t width, uint32_t height, VkFormat format, uint32_t mipCount, VkImageUsageFlags usage);
	void Fill(AppState& appState, Buffer* buffer, VkDeviceSize offset, VkCommandBuffer commandBuffer);
	void FillMipmapped(AppState& appState, StagingBuffer& buffer);
	void Delete(AppState& appState);
};
