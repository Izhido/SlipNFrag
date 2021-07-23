#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"
#include "LoadedColormappedTexture.h"
#include "LoadedSharedMemoryAliasIndexBuffer.h"

struct LoadedAlias
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryTexCoordsBuffer texCoords;
	LoadedSharedMemoryAliasIndexBuffer indices;
	LoadedColormappedTexture colormapped;
	int firstAttribute;
	bool isHostColormap;
	uint32_t count;
	float transform[3][4];
};
