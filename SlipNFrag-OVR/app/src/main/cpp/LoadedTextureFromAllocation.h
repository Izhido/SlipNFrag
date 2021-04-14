#pragma once

#include "TextureFromAllocation.h"

struct LoadedTextureFromAllocation
{
	TextureFromAllocation* texture;
	VkDeviceSize size;
	unsigned char* source;
	LoadedTextureFromAllocation* next;
};
