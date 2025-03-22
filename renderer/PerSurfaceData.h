#pragma once

#include "Lightmap.h"
#include "LightmapRGB.h"
#include "SharedMemoryTexture.h"

struct PerSurfaceData
{
	std::vector<float> vertices;
	SharedMemoryTexture* texture;
	int textureIndex;
	unsigned char* textureSource;
	SharedMemoryTexture* glowTexture;
	int glowTextureIndex;
	unsigned char* glowTextureSource;
	Lightmap* lightmap;
	LightmapRGB* lightmapRGB;
	int frameCount;
};
