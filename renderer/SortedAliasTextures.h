#pragma once

#include "SortedAliasTexture.h"

struct SortedAliasTextures
{
    int count;
    std::vector<SortedAliasTexture> textures;
    std::unordered_map<VkDescriptorSet, int> added;
};
