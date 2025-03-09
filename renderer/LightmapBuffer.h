#pragma once

#include <vector>
#include <SharedMemoryBuffer.h>

struct LightmapBuffer
{
	int width;
	int height;
	SharedMemoryBuffer buffer;
	std::vector<bool> allocated;
	int allocatedCount;
	int firstFreeCandidate;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	VkDeviceSize size;
	LightmapBuffer* previous;
	LightmapBuffer* next;
};
