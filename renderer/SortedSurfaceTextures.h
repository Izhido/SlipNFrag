#pragma once

#include "SortedSurfaceTexture.h"

struct SortedSurfaceTextures
{
    std::list<SortedSurfaceTexture> textures;
    std::unordered_map<VkDescriptorSet, std::list<SortedSurfaceTexture>::iterator> added;
};
