#pragma once

#include "Lightmap.h"

struct LoadedLightmap
{
	Lightmap* lightmap;
	VkDeviceSize size;
	unsigned* source;
	LoadedLightmap* next;
};
