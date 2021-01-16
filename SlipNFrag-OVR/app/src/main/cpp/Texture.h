#pragma once

struct Texture
{
	Texture* next;
	void* key;
	int unusedCount;
	int width;
	int height;
	int mipCount;
	int layerCount;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
};
