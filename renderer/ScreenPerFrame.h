#pragma once

#include <vk_mem_alloc.h>

struct ScreenPerFrame
{
	VkImage image;
	VkBuffer buffer;
	VmaAllocation allocation;
	VkDeviceSize size;
};
