#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryWithOffsetBuffer.h"
#include "LoadedSharedMemoryIndexBuffer.h"
#include "LoadedLightmap.h"
#include "LoadedSharedMemoryTexture.h"

struct LoadedSurfaceRotated
{
	LoadedSharedMemoryBuffer vertices;
	LoadedSharedMemoryWithOffsetBuffer texturePositions;
	LoadedSharedMemoryIndexBuffer indices;
	LoadedLightmap lightmap;
	LoadedSharedMemoryTexture texture;
	uint32_t count;
	float originX;
	float originY;
	float originZ;
	float yaw;
	float pitch;
	float roll;
};
