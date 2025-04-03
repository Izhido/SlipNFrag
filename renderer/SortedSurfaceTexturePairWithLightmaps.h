#pragma once

#include "SortedSurfaceLightmap.h"

struct SortedSurfaceTexturePairWithLightmaps
{
	VkDescriptorSet texture;
	VkDescriptorSet glowTexture;
	int count;
	std::vector<SortedSurfaceLightmap> lightmaps;
};
