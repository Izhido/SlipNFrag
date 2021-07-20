#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryIndexBuffer.h"
#include "LoadedLightmap.h"
#include "LoadedSharedMemoryTexture.h"

struct LoadedSurface
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryBuffer texturePosition;
	LoadedSharedMemoryIndexBuffer indices;
	LoadedLightmap lightmap;
	LoadedSharedMemoryTexture texture;
	int count;
	float originX;
	float originY;
	float originZ;
	float yaw;
	float pitch;
	float roll;
};
