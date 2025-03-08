#pragma once

#include "Buffer.h"
#include "StagingBuffer.h"

struct Texture
{
	void* key;
	int unusedCount;
	int width;
	int height;
	int mipCount;
	int layerCount;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkMemoryPropertyFlags properties;
	bool filled;

	void Create(AppState& appState, uint32_t width, uint32_t height, VkFormat format, uint32_t mipCount, VkImageUsageFlags usage);
	void Fill(AppState& appState, Buffer* buffer, VkDeviceSize offset, VkCommandBuffer commandBuffer);
	void Fill(AppState& appState, StagingBuffer& buffer);
	void Delete(AppState& appState);
};
