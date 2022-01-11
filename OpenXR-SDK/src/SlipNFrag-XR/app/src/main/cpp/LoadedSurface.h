#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexturePositionsBuffer.h"
#include "LoadedSharedMemoryIndexBuffer.h"
#include "LoadedLightmap.h"
#include "LoadedSharedMemoryTexture.h"

struct LoadedSurface
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryTexturePositionsBuffer texturePositions;
	LoadedSharedMemoryIndexBuffer indices;
	LoadedLightmap lightmap;
	LoadedSharedMemoryTexture texture;
	uint32_t count;
};
