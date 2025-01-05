#pragma once

#include "SortedSurfaceLightmap.h"

struct SortedSurfaceLightmapsWithTextures
{
    int count;
    std::vector<SortedSurfaceLightmap> lightmaps;
    std::unordered_map<VkDescriptorSet, int> added;
};
