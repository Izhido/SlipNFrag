#pragma once

#include <vulkan/vulkan.h>
#include <vector>

struct LightmapTexture
{
	int width = 0;
	int height = 0;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	std::vector<bool> allocated;
	int allocatedCount = 0;
	int firstFreeCandidate = 0;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	std::vector<VkBufferImageCopy> regions;
	uint32_t regionCount = 0;
	LightmapTexture* previous = nullptr;
	LightmapTexture* next = nullptr;

	void Initialize();
};
