#pragma once

#include "LoadedTurbulent.h"
#include "LoadedLightmap.h"

struct LoadedTurbulentLit : LoadedTurbulent
{
	LoadedLightmap lightmap;
};
