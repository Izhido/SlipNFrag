#pragma once

#include "SharedMemoryBuffer.h"

struct IndexBuffer
{
	SharedMemoryBuffer* buffer;
	VkDeviceSize offset;
	VkIndexType indexType;
	uint32_t firstIndex;
};
