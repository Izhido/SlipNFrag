#pragma once

#include "SortedSurfaceTexture.h"

struct SortedSurfaceLightmap
{
	VkDescriptorSet lightmap;
    int count;
	std::vector<SortedSurfaceTexture> textures;
};
