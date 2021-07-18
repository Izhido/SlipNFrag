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
	int firstIndex;
	int count;
	float transform[3][4];
};
