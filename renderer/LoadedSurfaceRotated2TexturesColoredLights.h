#pragma once

#include "LoadedSurface2TexturesColoredLights.h"

struct LoadedSurfaceRotated2TexturesColoredLights : LoadedSurface2TexturesColoredLights
{
	float originX;
	float originY;
	float originZ;
	float yaw;
	float pitch;
	float roll;
};
