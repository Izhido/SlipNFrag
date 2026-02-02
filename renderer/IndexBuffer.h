#pragma once

#include "Buffer.h"

struct IndexBuffer
{
	Buffer* buffer;
	VkDeviceSize offset;
	VkIndexType indexType;
	uint32_t firstIndex;
};
