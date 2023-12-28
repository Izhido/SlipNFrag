#pragma once

#include <vulkan/vulkan.h>
#include <vector>

struct LightmapTexture
{
	int width;
	int height;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	std::vector<bool> allocated;
	int allocatedCount;
	int firstFreeCandidate;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	std::vector<VkBufferImageCopy> regions;
	uint32_t regionCount;
	LightmapTexture* previous;
	LightmapTexture* next;

	void Initialize();
};
