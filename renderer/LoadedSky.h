#pragma once

struct LoadedSky
{
	int width;
	int height;
	VkDeviceSize size;
	unsigned char* data;
	int firstVertex;
	uint32_t count;
};
