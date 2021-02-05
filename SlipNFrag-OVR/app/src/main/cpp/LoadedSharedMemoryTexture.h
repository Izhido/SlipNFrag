#pragma once

#include "SharedMemoryTexture.h"

struct LoadedSharedMemoryTexture
{
	SharedMemoryTexture* texture;
	int offset;
};
