#pragma once

#include "Buffer.h"

struct BufferWithOffset
{
	VkDeviceSize offset;
	Buffer* buffer;
};
