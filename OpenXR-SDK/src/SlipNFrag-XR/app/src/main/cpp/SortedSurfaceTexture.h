#pragma once

#include <vector>

struct SortedSurfaceTexture
{
	VkDescriptorSet texture;
	std::vector<int> entries;
	VkDeviceSize indexCount;
};
