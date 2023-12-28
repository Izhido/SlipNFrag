#pragma once

#include "Texture.h"

struct LoadedTexture
{
	Texture* texture;
	VkDeviceSize size;
};
