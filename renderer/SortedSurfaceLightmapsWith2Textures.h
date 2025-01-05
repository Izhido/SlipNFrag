#pragma once

#include "SortedSurface2TexturesLightmap.h"

struct SortedSurfaceLightmapsWith2Textures
{
    std::list<SortedSurface2TexturesLightmap> lightmaps;
    std::unordered_map<VkDescriptorSet, std::list<SortedSurface2TexturesLightmap>::iterator> added;
};
