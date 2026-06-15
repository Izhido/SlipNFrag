#pragma once

#include "SortedAliasColormap.h"

struct SortedAliasColormaps
{
    int count;
    std::vector<SortedAliasColormap> colormaps;
    std::unordered_map<VkDescriptorSet, int> added;
};
