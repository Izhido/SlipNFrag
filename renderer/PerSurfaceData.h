#pragma once

#include "Lightmap.h"
#include "LightmapRGB.h"
#include "SharedMemoryTexture.h"

struct PerSurfaceData
{
	float* vertices;
	SharedMemoryTexture* texture;
	int textureIndex;
	unsigned char* textureSource;
	SharedMemoryTexture* glowTexture;
	int glowTextureIndex;
	unsigned char* glowTextureSource;
	uint32_t* lightmapSource;
	bool dlight;
	int lightadj[MAXLIGHTMAPS];
	Lightmap* lightmap;
	LightmapRGB* lightmapRGB;
	int frameCount;
};
