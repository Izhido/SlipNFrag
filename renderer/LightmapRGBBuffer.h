#pragma once

#include <vector>
#include <SharedMemoryBuffer.h>

struct LightmapRGBBuffer
{
	SharedMemoryBuffer buffer;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	VkDeviceSize used;
	VkDeviceSize size;
	int referenceCount;
	LightmapRGBBuffer* previous;
	LightmapRGBBuffer* next;
};
