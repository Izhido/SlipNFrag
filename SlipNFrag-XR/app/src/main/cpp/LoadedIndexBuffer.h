#pragma once

#include "IndexBuffer.h"

struct LoadedIndexBuffer
{
	IndexBuffer indices;
	VkDeviceSize size;
	void* source;
	LoadedIndexBuffer* next;
};
