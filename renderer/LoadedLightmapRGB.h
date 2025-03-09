#pragma once

#include "LightmapRGB.h"

struct LoadedLightmapRGB
{
	LightmapRGB* lightmap;
	VkDeviceSize size;
	void* source;
	LoadedLightmapRGB* next;
};
