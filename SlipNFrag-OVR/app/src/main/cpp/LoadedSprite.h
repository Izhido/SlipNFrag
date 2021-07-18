#pragma once

#include "LoadedSharedMemoryTexture.h"

struct LoadedSprite
{
	LoadedSharedMemoryTexture texture;
	int firstVertex;
	int count;
};
