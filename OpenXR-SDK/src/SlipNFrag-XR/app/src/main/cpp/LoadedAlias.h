#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"
#include "LoadedSharedMemoryWithOffsetBuffer.h"
#include "LoadedColormappedTexture.h"

struct LoadedAlias
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryTexCoordsBuffer texCoords;
	LoadedSharedMemoryWithOffsetBuffer indices;
	LoadedColormappedTexture colormapped;
	int firstAttribute;
	bool isHostColormap;
	uint32_t count;
	float transform[3][4];
};
