#pragma once

#include "LoadedAliasColoredLights.h"
#include "LoadedTexture.h"

struct LoadedAlias : LoadedAliasColoredLights
{
	LoadedTexture colormap;
	bool isHostColormap;
};
