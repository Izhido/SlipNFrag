#pragma once

#include "TextureFromAllocation.h"

struct LoadedTextureFromAllocation
{
	TextureFromAllocation* texture;
	VkDeviceSize size;
	float* source;
	LoadedTextureFromAllocation* next;
};
