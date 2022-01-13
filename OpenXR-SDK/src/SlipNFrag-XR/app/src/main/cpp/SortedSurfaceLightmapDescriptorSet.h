#pragma once

struct SortedSurfaceLightmapDescriptorSet
{
	VkDescriptorSet lightmapDescriptorSet;
	std::vector<int> entries;
};
