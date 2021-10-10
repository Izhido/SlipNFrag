#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexturePositionBuffer.h"
#include "LoadedSharedMemoryIndexBuffer.h"
#include "LoadedLightmap.h"
#include "LoadedSharedMemoryTexture.h"

struct LoadedSurface
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryTexturePositionBuffer texturePositions;
	LoadedSharedMemoryIndexBuffer indices;
	LoadedLightmap lightmap;
	LoadedSharedMemoryTexture texture;
	uint32_t count;
	float originX;
	float originY;
	float originZ;
};
