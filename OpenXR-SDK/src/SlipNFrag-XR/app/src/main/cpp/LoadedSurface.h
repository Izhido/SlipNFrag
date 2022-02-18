#pragma once

#include "LoadedTurbulent.h"
#include "LoadedLightmap.h"

struct LoadedSurface : LoadedTurbulent
{
	LoadedLightmap lightmap;
};
