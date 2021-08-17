#pragma once

#include <vector>
#include <unordered_set>

struct StagingBuffer
{
	Buffer* buffer;
	VkDeviceSize offset;
	VkCommandBuffer commandBuffer;
	std::vector<VkBufferMemoryBarrier> bufferBarriers;
	std::vector<VkImageMemoryBarrier> imageBarriers;
	std::unordered_set<VkDescriptorSet> descriptorSetsInUse;
	int lastBarrier;
};
