#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedLightmap.h"
#include "LoadedSharedMemoryTexture.h"

struct LoadedSurface
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryBuffer texturePosition;
	LoadedLightmap lightmap;
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
