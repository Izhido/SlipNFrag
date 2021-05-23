#pragma once

#include "VrApi.h"

struct Controller
{
	uint32_t PreviousButtons;
	uint32_t Buttons;
	ovrTracking Tracking;
	ovrResult TrackingResult;
	ovrVector2f PreviousJoystick;
	ovrVector2f Joystick;

	float* WriteVertices(float* vertices);
	float* WriteAttributes(float* attributes);
	uint16_t* WriteIndices(uint16_t* indices, uint16_t offset);
};
