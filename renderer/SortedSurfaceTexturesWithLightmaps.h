#pragma once

#include "SortedSurfaceTextureWithLightmaps.h"
#include USE_CUSTOM_HASHMAP_INCLUDE

struct SortedSurfaceTexturesWithLightmaps
{
    int count;
    std::vector<SortedSurfaceTextureWithLightmaps> textures;
    USE_CUSTOM_HASHMAP<VkDescriptorSet, int> added;
};
