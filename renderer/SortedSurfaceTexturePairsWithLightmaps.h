#pragma once

#include "SortedSurfaceTexturePairWithLightmaps.h"
#include <unordered_map>

struct SortedSurfaceTexturePairsWithLightmaps
{
	int count;
	std::vector<SortedSurfaceTexturePairWithLightmaps> textures;
	std::unordered_map<VkDescriptorSet, int> added;
};
