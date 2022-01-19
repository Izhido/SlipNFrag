#pragma once

#include "LoadedTurbulentRotated.h"
#include "LoadedLightmap.h"

struct LoadedTurbulentRotatedLit : LoadedTurbulentRotated
{
	LoadedLightmap lightmap;
};
