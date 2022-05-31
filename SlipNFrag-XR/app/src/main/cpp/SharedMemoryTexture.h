#pragma once

#include "Buffer.h"
#include "SharedMemory.h"
#include "StagingBuffer.h"
#include "DescriptorSets.h"

struct SharedMemoryTexture
{
	SharedMemoryTexture* next;
	int unusedCount;
	int width;
	int height;
	int mipCount;
	int layerCount;
	VkImage image;
	SharedMemory* sharedMemory;
	VkImageView view;
	VkDescriptorSet descriptorSet;
	DescriptorSets* descriptorSets;

	void Create(AppState& appState, uint32_t width, uint32_t height, VkFormat format, uint32_t mipCount, uint32_t layerCount, VkImageUsageFlags usage);
	void FillMipmapped(AppState& appState, StagingBuffer& buffer, int mips, int layer) const;
	void Delete(AppState& appState) const;
};
