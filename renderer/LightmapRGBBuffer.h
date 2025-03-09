#pragma once

#include <vector>
#include <SharedMemoryBuffer.h>

struct LightmapRGBBuffer
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
	LightmapRGBBuffer* previous;
	LightmapRGBBuffer* next;
};
