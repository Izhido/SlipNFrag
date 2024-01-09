#pragma once

#include "LoadedSharedMemoryTexture.h"

struct LoadedTurbulent
{
	void* face;
	void* model;
	LoadedSharedMemoryTexture texture;
	uint32_t count;
};
