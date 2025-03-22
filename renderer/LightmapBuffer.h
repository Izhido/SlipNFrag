#pragma once

#include <vector>
#include <SharedMemoryBuffer.h>

struct LightmapBuffer
{
	SharedMemoryBuffer buffer;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	VkDeviceSize used;
	VkDeviceSize size;
	int referenceCount;
	LightmapBuffer* previous;
	LightmapBuffer* next;
};
