#pragma once

#include "LoadedSharedMemoryTexture.h"

struct LoadedSprite
{
	LoadedSharedMemoryTexture texture;
	uint32_t firstVertex;
	uint32_t count;
};
