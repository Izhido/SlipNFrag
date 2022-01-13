#pragma once

#include "SortedSurfaceLightmapDescriptorSet.h"

struct SortedSurfaceTexture
{
	VkDescriptorSet textureDescriptorSet;
	std::list<SortedSurfaceLightmapDescriptorSet> lightmapDescriptorSets;
};
