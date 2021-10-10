#pragma once

#include "SharedMemoryBuffer.h"

struct LoadedSharedMemoryTexturePositionBuffer
{
	SharedMemoryBuffer* buffer;
	VkDeviceSize size;
	VkDeviceSize offset;
	void* source;
	LoadedSharedMemoryTexturePositionBuffer* next;
};
