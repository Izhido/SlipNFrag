#pragma once

#include <vector>
#include <SharedMemoryBuffer.h>

struct LightmapTexture
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
	LightmapTexture* previous;
	LightmapTexture* next;

	void Initialize();
};
