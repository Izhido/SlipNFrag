#pragma once

#include "IndexBuffer.h"

struct LoadedIndexBuffer
{
	IndexBuffer indices;
	VkDeviceSize size;
	void* firstSource;
	void* secondSource;
	LoadedIndexBuffer* next;
};
