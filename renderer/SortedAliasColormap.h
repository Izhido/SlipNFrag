#pragma once

#include "SortedAliasIndicesWithTextures.h"

struct SortedAliasColormap
{
    VkDescriptorSet colormap;
    int count;
    std::vector<SortedAliasIndicesWithTextures> indices;
    std::unordered_map<VkBuffer, int> added;
};
