#pragma once

#include "LoadedTurbulent.h"

struct LoadedTurbulentRotated : LoadedTurbulent
{
	float originX;
	float originY;
	float originZ;
	float yaw;
	float pitch;
	float roll;
};
