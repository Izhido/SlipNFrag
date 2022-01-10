#pragma once

#include "SharedMemoryIndexBuffer.h"

struct LoadedSharedMemoryIndexBuffer
{
	SharedMemoryIndexBuffer indices;
	VkDeviceSize size;
	void* firstSource;
	void* secondSource;
	LoadedSharedMemoryIndexBuffer* next;
};
