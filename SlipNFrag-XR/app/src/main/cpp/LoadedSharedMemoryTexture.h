#pragma once

#include "SharedMemoryTexture.h"

struct LoadedSharedMemoryTexture
{
	SharedMemoryTexture* texture;
	VkDeviceSize size;
	unsigned char* source;
	int mips;
	LoadedSharedMemoryTexture* next;
};
