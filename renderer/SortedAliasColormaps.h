#pragma once

#include "SortedAliasColormap.h"

struct SortedAliasColormaps
{
    int count;
    std::vector<SortedAliasColormap> colormaps;
    USE_CUSTOM_HASHMAP<VkDescriptorSet, int> added;
};
