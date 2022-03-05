#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedIndexBuffer.h"
#include "LoadedSharedMemoryTexture.h"

struct LoadedTurbulent
{
	void* vertexes;
	void* face;
	LoadedIndexBuffer indices;
	LoadedSharedMemoryTexture texture;
	uint32_t count;
};
