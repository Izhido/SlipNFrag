#pragma once

#include "LoadedTurbulent.h"
#include "LoadedLightmapRGBA.h"

struct LoadedSurfaceColoredLights : LoadedTurbulent
{
	LoadedLightmapRGBA lightmap;
};
