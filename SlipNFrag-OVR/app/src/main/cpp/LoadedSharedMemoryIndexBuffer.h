#pragma once

#include "SharedMemoryBuffer.h"

struct LoadedSharedMemoryIndexBuffer
{
	SharedMemoryBuffer* buffer;
	VkDeviceSize size;
	void* firstSource;
	void* secondSource;
	LoadedSharedMemoryIndexBuffer* next;
};
