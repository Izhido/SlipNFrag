#pragma once

#include "Buffer.h"

struct LoadedSharedMemoryBuffer
{
	Buffer* buffer;
	VkDeviceSize size;
	void* source;
	LoadedSharedMemoryBuffer* next;
};
