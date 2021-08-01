#pragma once

#include <vector>

struct StagingBuffer
{
	Buffer* buffer;
	VkDeviceSize offset;
	VkCommandBuffer commandBuffer;
	std::vector<VkBufferMemoryBarrier> bufferBarriers;
	std::vector<VkImageMemoryBarrier> imageBarriers;
	int lastBarrier;
};
