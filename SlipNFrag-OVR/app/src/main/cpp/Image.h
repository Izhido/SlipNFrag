#pragma once

struct Image
{
	int width;
	int height;
	int layerCount;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
};
