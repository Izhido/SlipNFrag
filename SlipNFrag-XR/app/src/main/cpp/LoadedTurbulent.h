#pragma once

#include "LoadedSharedMemoryTexture.h"

struct LoadedTurbulent
{
	void* vertexes;
	void* face;
	void* model;
	LoadedSharedMemoryTexture texture;
	uint32_t count;
};
