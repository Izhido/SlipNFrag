#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"
#include "LoadedColormappedTexture.h"

struct LoadedAlias
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryTexCoordsBuffer texCoords;
	LoadedColormappedTexture colormapped;
	int firstAttribute;
	bool isHostColormap;
	uint32_t firstIndex;
	uint32_t count;
	float transform[3][4];
};
