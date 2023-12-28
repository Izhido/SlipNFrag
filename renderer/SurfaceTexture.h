#pragma once

#include "SharedMemoryTexture.h"

struct SurfaceTexture
{
	SharedMemoryTexture* texture;
	int index;
};
