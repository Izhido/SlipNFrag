#pragma once

#include "SortedAliasIndicesWithTextures.h"

struct SortedAliasIndices
{
    int count;
    std::vector<SortedAliasIndicesWithTextures> indices;
    USE_CUSTOM_HASHMAP<VkBuffer, int> added;
};
