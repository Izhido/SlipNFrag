#pragma once

#include "SharedMemoryTexture.h"

struct LoadedSharedMemoryTexture
{
	SharedMemoryTexture* texture;
	int index;
	VkDeviceSize size;
	VkDeviceSize allocated;
	unsigned char* source;
	int mips;
	LoadedSharedMemoryTexture* next;
};
