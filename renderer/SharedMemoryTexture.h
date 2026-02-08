#pragma once

#include "Buffer.h"
#include "StagingBuffer.h"
#include "DescriptorSets.h"

struct SharedMemoryTexture
{
	SharedMemoryTexture* next;
	int unusedCount;
	int width;
	int height;
	VkFormat format;
	int mipCount;
	int layerCount;
	VmaAllocation allocation;
	VkImage image;
	VkImageView view;
	VkDescriptorSet descriptorSet;
	DescriptorSets* descriptorSets;
	VkMemoryPropertyFlags properties;
	bool filled;

	void Create(AppState& appState, uint32_t width, uint32_t height, VkFormat format, uint32_t mipCount, uint32_t layerCount, VkImageUsageFlags usage);
	void FillMipmapped(AppState& appState, StagingBuffer& buffer, int mips, int layer);
	void FillMipmapped(AppState& appState, StagingBuffer& buffer, int layer);
	void Delete(AppState& appState) const;
};
