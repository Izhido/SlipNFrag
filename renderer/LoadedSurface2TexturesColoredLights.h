#pragma once

#include "LoadedTurbulent.h"
#include "LoadedLightmapRGB.h"

struct LoadedSurface2TexturesColoredLights : LoadedTurbulent
{
	LoadedSharedMemoryTexture glowTexture;
	LoadedLightmapRGB lightmap;
};
