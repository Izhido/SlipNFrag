#pragma once

#include <vector>

struct StagingBuffer
{
	Buffer* buffer;
	VkDeviceSize offset;
	VkCommandBuffer commandBuffer;
	std::vector<VkBufferMemoryBarrier> vertexInputEndBarriers;
    std::vector<VkBufferMemoryBarrier> vertexShaderEndBarriers;
	std::vector<VkImageMemoryBarrier> imageStartBarriers;
	std::vector<VkImageMemoryBarrier> imageEndBarriers;
	int lastStartBarrier;
	int lastEndBarrier;
    int lastVertexShaderEndBarrier;
};
