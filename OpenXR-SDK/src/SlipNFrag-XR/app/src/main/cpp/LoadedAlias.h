#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"
#include "LoadedSharedMemoryIndexBuffer.h"
#include "LoadedColormappedTexture.h"

struct LoadedAlias
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryTexCoordsBuffer texCoords;
	LoadedSharedMemoryIndexBuffer indices;
	LoadedColormappedTexture colormapped;
	int firstAttribute;
	bool isHostColormap;
	uint32_t count;
	float transform[3][4];
};
