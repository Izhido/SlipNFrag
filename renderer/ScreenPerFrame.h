#pragma once

#include <vk_mem_alloc.h>

struct ScreenPerFrame
{
	VkImage image;
	Buffer stagingBuffer;
};
