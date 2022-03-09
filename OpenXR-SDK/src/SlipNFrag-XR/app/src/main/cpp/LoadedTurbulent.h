#pragma once

#include "LoadedSharedMemoryTexture.h"

struct LoadedTurbulent
{
	void* vertexes;
	void* face;
	void* model;
	float* vertices;
	int vertexCount;
	LoadedSharedMemoryTexture texture;
	uint32_t count;
};
