#pragma once

#include "SortedSurfaceTexture.h"

struct SortedSurfaceTextures
{
    int count;
    std::vector<SortedSurfaceTexture> textures;
    USE_CUSTOM_HASHMAP<VkDescriptorSet, int> added;
};
