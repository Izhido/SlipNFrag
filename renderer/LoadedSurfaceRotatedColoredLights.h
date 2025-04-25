#pragma once

#include "LoadedSurfaceColoredLights.h"

struct LoadedSurfaceRotatedColoredLights : LoadedSurfaceColoredLights
{
	float originX;
	float originY;
	float originZ;
	float yaw;
	float pitch;
	float roll;
	unsigned char alpha;
};
