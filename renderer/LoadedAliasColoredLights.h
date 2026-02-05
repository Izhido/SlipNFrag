#pragma once

#include "LoadedBuffer.h"
#include "LoadedTexCoordsBuffer.h"
#include "LoadedIndexBuffer.h"

struct LoadedAliasColoredLights
{
	LoadedBuffer vertices;
	LoadedTexCoordsBuffer texCoords;
	LoadedIndexBuffer indices;
	LoadedSharedMemoryTexture texture;
	int firstAttribute;
	uint32_t count;
	float transform[3][4];
};
