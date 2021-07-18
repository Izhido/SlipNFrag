#pragma once

#include "LoadedSharedMemoryTexture.h"
#include "LoadedTexture.h"

struct LoadedColormappedTexture
{
	LoadedSharedMemoryTexture texture;
	LoadedTexture colormap;
};
