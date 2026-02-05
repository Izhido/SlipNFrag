#pragma once

#include <vector>
#include <Buffer.h>

struct LightmapRGBBuffer
{
	Buffer buffer;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	VkDeviceSize used;
	VkDeviceSize size;
	int referenceCount;
	LightmapRGBBuffer* previous;
	LightmapRGBBuffer* next;
};
