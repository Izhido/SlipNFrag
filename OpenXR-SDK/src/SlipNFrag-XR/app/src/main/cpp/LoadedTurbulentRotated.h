#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexturePositionBuffer.h"
#include "LoadedSharedMemoryIndexBuffer.h"
#include "LoadedSharedMemoryTexture.h"

struct LoadedTurbulentRotated
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryTexturePositionBuffer texturePositions;
	LoadedSharedMemoryIndexBuffer indices;
	LoadedSharedMemoryTexture texture;
	uint32_t count;
	float originX;
	float originY;
	float originZ;
	float yaw;
	float pitch;
	float roll;
};
