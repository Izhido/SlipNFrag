#pragma once

#include "LoadedTurbulent.h"
#include "LoadedLightmapRGBA.h"

struct LoadedSurface2TexturesColoredLights : LoadedTurbulent
{
	LoadedSharedMemoryTexture glowTexture;
	LoadedLightmapRGBA lightmap;
};
