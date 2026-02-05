#pragma once

#include <vector>
#include <Buffer.h>

struct LightmapBuffer
{
	Buffer buffer;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	VkDeviceSize used;
	VkDeviceSize size;
	int referenceCount;
	LightmapBuffer* previous;
	LightmapBuffer* next;
};
