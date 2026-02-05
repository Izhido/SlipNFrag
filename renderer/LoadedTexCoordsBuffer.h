#pragma once

#include "Buffer.h"

struct LoadedTexCoordsBuffer
{
	Buffer* buffer;
	int width;
	int height;
	VkDeviceSize size;
	void* source;
	LoadedTexCoordsBuffer* next;
};
