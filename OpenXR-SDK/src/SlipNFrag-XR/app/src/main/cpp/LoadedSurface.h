#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexturePositionsBuffer.h"
#include "LoadedIndexBuffer.h"
#include "LoadedLightmap.h"
#include "LoadedSharedMemoryTexture.h"

struct LoadedSurface
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryTexturePositionsBuffer texturePositions;
	LoadedIndexBuffer indices;
	LoadedLightmap lightmap;
	LoadedSharedMemoryTexture texture;
	uint32_t count;
};
