#pragma once

struct SharedMemoryTexture
{
	SharedMemoryTexture* next;
	int unusedCount;
	int width;
	int height;
	int mipCount;
	int layerCount;
	VkImage image;
	SharedMemory* sharedMemory;
	VkImageView view;
};
