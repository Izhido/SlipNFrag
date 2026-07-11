#pragma once

#include "SortedAliasIndicesWithTextures.h"

struct SortedAliasColormap
{
    VkDescriptorSet colormap;
    int count;
    std::vector<SortedAliasIndicesWithTextures> indices;
    USE_CUSTOM_HASHMAP<VkBuffer, int> added;
};
