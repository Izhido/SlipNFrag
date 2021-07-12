#pragma once

#include "Buffer.h"
#include "SharedMemory.h"
#include "StagingBuffer.h"
#include "DescriptorSets.h"

struct SharedMemoryTexture
{
	SharedMemoryTexture* next = nullptr;
	int unusedCount = 0;
	int width = 0;
	int height = 0;
	int mipCount = 0;
	int layerCount = 0;
	VkImage image = VK_NULL_HANDLE;
	SharedMemory* sharedMemory = nullptr;
	VkImageView view = VK_NULL_HANDLE;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	DescriptorSets* descriptorSets = nullptr;
	bool filled;

	void Create(AppState& appState, uint32_t width, uint32_t height, VkFormat format, uint32_t mipCount, VkImageUsageFlags usage);
	void FillMipmapped(AppState& appState, StagingBuffer& buffer);
	void Delete(AppState& appState);
};
