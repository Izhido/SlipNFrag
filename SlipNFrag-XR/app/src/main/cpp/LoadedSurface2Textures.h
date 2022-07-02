#pragma once

#include "LoadedTurbulent.h"
#include "LoadedLightmap.h"

struct LoadedSurface2Textures : LoadedTurbulent
{
	LoadedSharedMemoryTexture glowTexture;
	LoadedLightmap lightmap;
};
