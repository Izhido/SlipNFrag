#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexturePositionsBuffer.h"
#include "LoadedSharedMemoryIndexBuffer.h"
#include "LoadedSharedMemoryTexture.h"

struct LoadedTurbulent
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryTexturePositionsBuffer texturePositions;
	LoadedSharedMemoryIndexBuffer indices;
	LoadedSharedMemoryTexture texture;
	uint32_t count;
};
