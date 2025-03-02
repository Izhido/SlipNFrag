#pragma once

#include <vulkan/vulkan.h>
#include <vector>

struct LightmapRGBATexture
{
	int width;
	int height;
	VkBuffer buffer;
	VkDeviceMemory memory;
	std::vector<bool> allocated;
	int allocatedCount;
	int firstFreeCandidate;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	VkDeviceSize size;
	LightmapRGBATexture* previous;
	LightmapRGBATexture* next;

	void Initialize();
};
