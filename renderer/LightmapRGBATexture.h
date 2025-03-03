#pragma once

#include <vector>
#include <SharedMemoryBuffer.h>

struct LightmapRGBATexture
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
	LightmapRGBATexture* previous;
	LightmapRGBATexture* next;

	void Initialize();
};
