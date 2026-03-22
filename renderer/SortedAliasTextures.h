#pragma once

#include "SortedAliasTexture.h"
#include <unordered_map>

struct SortedAliasTextures
{
    int count;
    std::vector<SortedAliasTexture> textures;
    std::unordered_map<VkDescriptorSet, int> added;
};
