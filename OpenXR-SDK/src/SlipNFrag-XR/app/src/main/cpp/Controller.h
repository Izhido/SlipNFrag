#pragma once

#include <openxr/openxr.h>

struct Controller
{
	XrSpaceLocation SpaceLocation;
	bool PoseIsValid;

	float* WriteVertices(float* vertices);
	static float* WriteAttributes(float* attributes);
	static uint16_t* WriteIndices(uint16_t* indices, uint16_t offset);
};
