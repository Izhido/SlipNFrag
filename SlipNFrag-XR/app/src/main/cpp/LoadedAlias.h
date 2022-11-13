#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"
#include "LoadedIndexBuffer.h"
#include "LoadedTexture.h"

struct LoadedAlias
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryTexCoordsBuffer texCoords;
	LoadedIndexBuffer indices;
	LoadedSharedMemoryTexture texture;
	LoadedTexture colormap;
	int firstAttribute;
	bool isHostColormap;
	uint32_t count;
	float transform[3][4];
};
