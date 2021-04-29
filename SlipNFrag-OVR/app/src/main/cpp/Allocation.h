#pragma once

#include <vulkan/vulkan.h>
#include <vector>

struct Allocation
{
	VkDeviceMemory memory = VK_NULL_HANDLE;
	std::vector<bool> allocated;
	int allocatedCount = 0;
	int firstFreeCandidate = 0;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> descriptorSets;
};
