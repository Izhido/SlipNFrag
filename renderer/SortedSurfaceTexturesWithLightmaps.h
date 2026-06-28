#pragma once

#include "SortedSurfaceTextureWithLightmaps.h"
#include <unordered_map>

struct SortedSurfaceTexturesWithLightmaps
{
    int count;
    std::vector<SortedSurfaceTextureWithLightmaps> textures;
    std::unordered_map<VkDescriptorSet, int> added;
};
