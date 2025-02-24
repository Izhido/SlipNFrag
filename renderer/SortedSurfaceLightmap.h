#pragma once

#include <vector>

struct SortedSurfaceLightmap
{
	VkDescriptorSet lightmap;
	std::vector<int> entries;
	VkDeviceSize indexCount;
};
