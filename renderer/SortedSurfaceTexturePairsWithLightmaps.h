#pragma once

#include "SortedSurfaceTexturePairWithLightmaps.h"

struct SortedSurfaceTexturePairsWithLightmaps
{
	int count;
	std::vector<SortedSurfaceTexturePairWithLightmaps> textures;
	USE_CUSTOM_HASHMAP<VkDescriptorSet, int> added;
};
