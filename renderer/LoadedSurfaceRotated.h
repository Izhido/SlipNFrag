#pragma once

#include "LoadedSurface.h"

struct LoadedSurfaceRotated : LoadedSurface
{
	float originX;
	float originY;
	float originZ;
	float yaw;
	float pitch;
	float roll;
};
