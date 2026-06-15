#pragma once

#include "SortedAliasIndicesWithTextures.h"

struct SortedAliasIndices
{
    int count;
    std::vector<SortedAliasIndicesWithTextures> indices;
    std::unordered_map<VkBuffer, int> added;
};
