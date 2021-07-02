#pragma once

#include "SharedMemoryBuffer.h"

struct LoadedSharedMemoryBuffer
{
	SharedMemoryBuffer* buffer;
	VkDeviceSize size;
	void* source;
	LoadedSharedMemoryBuffer* next;
};
