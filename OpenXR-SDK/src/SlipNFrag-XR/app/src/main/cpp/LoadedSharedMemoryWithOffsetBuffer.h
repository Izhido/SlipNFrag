#pragma once

#include "SharedMemoryBuffer.h"

struct LoadedSharedMemoryWithOffsetBuffer
{
	SharedMemoryBuffer* buffer;
	VkDeviceSize size;
	VkDeviceSize offset;
	void* source;
	LoadedSharedMemoryWithOffsetBuffer* next;
};
