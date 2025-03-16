#pragma once

#include "Lightmap.h"
#include "LightmapRGB.h"
#include "SharedMemoryTexture.h"

struct PerSurfaceData
{
	std::vector<float> vertices;
	SharedMemoryTexture* texture;
	int textureIndex;
	SharedMemoryTexture* glowTexture;
	int glowTextureIndex;
	Lightmap* lightmap;
	LightmapRGB* lightmapRGB;
};
