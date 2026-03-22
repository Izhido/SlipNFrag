#pragma once

#include "SortedSurfaceTexture.h"
#include <unordered_map>

struct SortedSurfaceTextures
{
    int count;
    std::vector<SortedSurfaceTexture> textures;
    std::unordered_map<VkDescriptorSet, int> added;
};
