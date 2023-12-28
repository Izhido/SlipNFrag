#pragma once

#include "SortedSurface2Textures.h"

struct SortedSurface2TexturesLightmap
{
	VkDescriptorSet lightmap;
	std::list<SortedSurface2Textures> textures;
};
