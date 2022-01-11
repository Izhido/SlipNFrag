#pragma once

#include "SharedMemoryTexturePositionsBuffer.h"

struct LoadedSharedMemoryTexturePositionsBuffer
{
	SharedMemoryTexturePositionsBuffer texturePositions;
	VkDeviceSize size;
	void* source;
	LoadedSharedMemoryTexturePositionsBuffer* next;
};
