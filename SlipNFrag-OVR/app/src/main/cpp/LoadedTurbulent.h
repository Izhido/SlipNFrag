#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryIndexBuffer.h"
#include "LoadedSharedMemoryTexture.h"

struct LoadedTurbulent
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryBuffer texturePosition;
	LoadedSharedMemoryIndexBuffer indices;
	LoadedSharedMemoryTexture texture;
	int count;
	float originX;
	float originY;
	float originZ;
	float yaw;
	float pitch;
	float roll;
};
