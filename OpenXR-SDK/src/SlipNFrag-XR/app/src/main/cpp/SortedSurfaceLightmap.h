#pragma once

#include "SortedSurfaceTexture.h"

struct SortedSurfaceLightmap
{
	VkDescriptorSet lightmap;
	std::list<SortedSurfaceTexture> textures;
};
