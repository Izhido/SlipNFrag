#pragma once

#include "LightmapRGBA.h"

struct LoadedLightmapRGBA
{
	LightmapRGBA* lightmap;
	VkDeviceSize size;
	void* source;
	LoadedLightmapRGBA* next;
};
