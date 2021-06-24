#pragma once

#include <vector>

struct StagingBuffer
{
	Buffer* buffer;
	VkDeviceSize offset;
	VkCommandBuffer commandBuffer;
	std::vector<VkImageMemoryBarrier> barriers;
	int lastBarrier;
};
