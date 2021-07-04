#pragma once

#include "SharedMemoryBuffer.h"

struct LoadedSharedMemoryTexCoordsBuffer
{
	SharedMemoryBuffer* buffer;
	int width;
	int height;
	VkDeviceSize size;
	void* source;
	LoadedSharedMemoryTexCoordsBuffer* next;
};
