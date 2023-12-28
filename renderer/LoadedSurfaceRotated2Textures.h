#pragma once

#include "LoadedSurface2Textures.h"

struct LoadedSurfaceRotated2Textures : LoadedSurface2Textures
{
	float originX;
	float originY;
	float originZ;
	float yaw;
	float pitch;
	float roll;
};
