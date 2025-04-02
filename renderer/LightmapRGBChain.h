#pragma once

#include "LoadedLightmapRGB.h"

struct LightmapRGBChain
{
	LoadedLightmapRGB* first;
	LoadedLightmapRGB* current;
};
