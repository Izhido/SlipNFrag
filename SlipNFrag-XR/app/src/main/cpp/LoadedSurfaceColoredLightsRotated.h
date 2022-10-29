#pragma once

#include "LoadedSurfaceColoredLights.h"

struct LoadedSurfaceColoredLightsRotated : LoadedSurfaceColoredLights
{
	float originX;
	float originY;
	float originZ;
	float yaw;
	float pitch;
	float roll;
};
