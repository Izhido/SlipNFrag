#pragma once

#include "SortedSurface2TexturesLightmap.h"

struct SortedSurfaceLightmapsWith2Textures
{
    int count;
    std::vector<SortedSurface2TexturesLightmap> lightmaps;
    std::unordered_map<VkDescriptorSet, int> added;
};
