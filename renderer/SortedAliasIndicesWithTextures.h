#pragma once

#include "SortedAliasTexture.h"

struct SortedAliasIndicesWithTextures
{
    VkBuffer indices;
    VkIndexType indexType;
    int count;
    std::vector<SortedAliasTexture> textures;
    USE_CUSTOM_HASHMAP<VkDescriptorSet, int> added;
};
