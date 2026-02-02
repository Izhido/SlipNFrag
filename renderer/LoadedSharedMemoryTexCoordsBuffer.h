#pragma once

#include "SharedMemoryBuffer.h"

struct LoadedSharedMemoryTexCoordsBuffer
{
	Buffer* buffer;
	int width;
	int height;
	VkDeviceSize size;
	void* source;
	LoadedSharedMemoryTexCoordsBuffer* next;
};
