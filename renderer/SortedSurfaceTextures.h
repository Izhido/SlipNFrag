#pragma once

#include "SortedSurfaceTexture.h"

struct SortedSurfaceTextures
{
    int count;
    std::vector<SortedSurfaceTexture> textures;
    std::unordered_map<VkDescriptorSet, int> added;
};
