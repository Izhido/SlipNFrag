#pragma once

#include "SharedMemoryTexture.h"

struct LoadedSharedMemoryTexture
{
	SharedMemoryTexture* texture;
	int index;
	VkDeviceSize size;
	unsigned char* source;
	int mips;
	LoadedSharedMemoryTexture* next;
};
