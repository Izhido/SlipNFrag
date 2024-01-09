#pragma once

#include "LoadedSharedMemoryTexture.h"

struct LoadedTurbulent
{
	void* face;
	void* model;
    uint32_t count;
	LoadedSharedMemoryTexture texture;
};
