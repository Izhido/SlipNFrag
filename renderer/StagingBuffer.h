#pragma once

#include <vector>

struct StagingBuffer
{
	Buffer* buffer;
	VkDeviceSize offset;
	VkCommandBuffer commandBuffer;
	std::vector<VkBufferMemoryBarrier> vertexInputEndBarriers;
    std::vector<VkBufferMemoryBarrier> fragmentShaderEndBarriers;
	std::vector<VkImageMemoryBarrier> imageStartBarriers;
	std::vector<VkImageMemoryBarrier> imageEndBarriers;
	int lastStartBarrier;
	int lastEndBarrier;
    int lastFragmentShaderEndBarrier;
};
