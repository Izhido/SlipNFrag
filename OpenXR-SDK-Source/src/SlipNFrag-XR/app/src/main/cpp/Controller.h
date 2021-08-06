#pragma once

#include <openxr/openxr.h>

struct Controller
{
	uint32_t PreviousButtons;
	uint32_t Buttons;
	XrSpaceLocation SpaceLocation;
	bool PoseIsValid;
	XrVector2f PreviousJoystick;
	XrVector2f Joystick;

	float* WriteVertices(float* vertices);
	float* WriteAttributes(float* attributes);
	uint16_t* WriteIndices(uint16_t* indices, uint16_t offset);
};
