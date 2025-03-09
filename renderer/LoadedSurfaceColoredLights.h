#pragma once

#include "LoadedTurbulent.h"
#include "LoadedLightmapRGB.h"

struct LoadedSurfaceColoredLights : LoadedTurbulent
{
	LoadedLightmapRGB lightmap;
};
