#pragma once

#include "SortedSurfaceTextureWithLightmaps.h"

struct SortedSurfaceTexturesWithLightmaps
{
    int count;
    std::vector<SortedSurfaceTextureWithLightmaps> textures;
    std::unordered_map<VkDescriptorSet, int> added;
};
