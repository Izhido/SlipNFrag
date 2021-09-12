#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryIndexBuffer.h"
#include "LoadedSharedMemoryTexture.h"

struct LoadedTurbulentRotated
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryBuffer texturePosition;
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
