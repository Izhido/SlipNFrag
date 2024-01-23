#pragma once

#include <vector>
#include <unordered_set>

struct StagingBuffer
{
	Buffer* buffer;
	VkDeviceSize offset;
	VkCommandBuffer commandBuffer;
	std::vector<VkBufferMemoryBarrier> vertexInputEndBarriers;
    std::vector<VkBufferMemoryBarrier> vertexShaderEndBarriers;
	std::vector<VkImageMemoryBarrier> imageStartBarriers;
	std::vector<VkImageMemoryBarrier> imageEndBarriers;
	std::unordered_set<VkDescriptorSet> descriptorSetsInUse;
	int lastStartBarrier;
	int lastEndBarrier;
    int lastVertexShaderEndBarrier;
};
