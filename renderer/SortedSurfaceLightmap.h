#pragma once

struct SortedSurfaceLightmap
{
	VkDescriptorSet lightmap;
	std::vector<int> entries;
	VkDeviceSize indexCount;
};
