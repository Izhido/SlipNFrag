#pragma once

#include "LoadedLightmap.h"

struct LightmapChain
{
	LoadedLightmap* first;
	LoadedLightmap* current;
};
