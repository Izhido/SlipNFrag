#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexture.h"

struct LoadedTurbulent
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryBuffer texturePosition;
	LoadedSharedMemoryTexture texture;
	int firstIndex;
	int count;
	float originX;
	float originY;
	float originZ;
	float yaw;
	float pitch;
	float roll;
};
