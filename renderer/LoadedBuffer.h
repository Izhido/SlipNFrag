#pragma once

#include "Buffer.h"

struct LoadedBuffer
{
	Buffer* buffer;
	VkDeviceSize size;
	void* source;
	LoadedBuffer* next;
};
