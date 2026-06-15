#pragma once

#include "SortedAliasTexture.h"

struct SortedAliasIndicesWithTextures
{
    VkBuffer indices;
    VkIndexType indexType;
    int count;
    std::vector<SortedAliasTexture> textures;
    std::unordered_map<VkDescriptorSet, int> added;
};
