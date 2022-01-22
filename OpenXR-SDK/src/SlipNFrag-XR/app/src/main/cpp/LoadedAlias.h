#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"
#include "LoadedIndexBuffer.h"
#include "LoadedColormappedTexture.h"

struct LoadedAlias
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryTexCoordsBuffer texCoords;
	LoadedIndexBuffer indices;
	LoadedColormappedTexture colormapped;
	int firstAttribute;
	bool isHostColormap;
	uint32_t count;
	float transform[3][4];
};
