#pragma once

#include "SortedSurfaceLightmap.h"

struct SortedSurfaceTextureWithLightmaps
{
	VkDescriptorSet texture;
	int count;
	std::vector<SortedSurfaceLightmap> lightmaps;
};
