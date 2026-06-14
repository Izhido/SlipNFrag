#pragma once

#include "LoadedAliasColoredLights.h"

struct LoadedAlias : LoadedAliasColoredLights
{
    bool isHostColormap;
	LoadedSharedMemoryTexture colormap;
};
