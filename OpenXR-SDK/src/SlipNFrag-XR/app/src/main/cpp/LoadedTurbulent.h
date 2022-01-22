#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexturePositionsBuffer.h"
#include "LoadedIndexBuffer.h"
#include "LoadedSharedMemoryTexture.h"

struct LoadedTurbulent
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryTexturePositionsBuffer texturePositions;
	LoadedIndexBuffer indices;
	LoadedSharedMemoryTexture texture;
	uint32_t count;
};
