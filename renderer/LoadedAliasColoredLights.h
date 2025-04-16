#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"
#include "LoadedIndexBuffer.h"

struct LoadedAliasColoredLights
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryTexCoordsBuffer texCoords;
	LoadedIndexBuffer indices;
	LoadedSharedMemoryTexture texture;
	int firstAttribute;
	uint32_t count;
	float transform[3][4];
};
