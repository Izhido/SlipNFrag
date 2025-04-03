#pragma once

#include "SortedSurfaceTexturePairWithLightmaps.h"

struct SortedSurfaceTexturePairsWithLightmaps
{
	int count;
	std::vector<SortedSurfaceTexturePairWithLightmaps> textures;
	std::unordered_map<VkDescriptorSet, int> added;
};
