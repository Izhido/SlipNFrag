#pragma once

#include "Lightmap.h"

struct LoadedLightmap
{
	Lightmap* lightmap;
	VkDeviceSize size;
	float* source;
	LoadedLightmap* next;
};
