#pragma once

#include "LoadedSharedMemoryTexture.h"

struct LoadedTurbulent
{
	void* face;
	void* entity;
    uint32_t count;
    int numedges;
    float* vertices;
	LoadedSharedMemoryTexture texture;
};
