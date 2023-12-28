#pragma once

#include "Lightmap.h"

struct LoadedLightmap
{
	Lightmap* lightmap;
	VkDeviceSize size;
	void* source;
	LoadedLightmap* next;
};
