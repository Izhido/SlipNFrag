#pragma once

#include "SortedSurface2Textures.h"

struct SortedSurface2TexturesLightmap
{
	VkDescriptorSet lightmap;
    int count;
	std::vector<SortedSurface2Textures> textures;
};
