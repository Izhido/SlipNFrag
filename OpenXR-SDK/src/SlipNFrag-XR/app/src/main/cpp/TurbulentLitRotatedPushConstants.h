#pragma once

#include "TurbulentLitPushConstants.h"

struct TurbulentLitRotatedPushConstants : TurbulentLitPushConstants
{
	float originX;
	float originY;
	float originZ;
	float yaw;
	float pitch;
	float roll;
};
