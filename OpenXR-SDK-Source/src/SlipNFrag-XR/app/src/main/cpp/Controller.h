#pragma once

#include <openxr/openxr.h>

struct Controller
{
	XrSpaceLocation SpaceLocation;
	bool PoseIsValid;

	float* WriteVertices(float* vertices);
	float* WriteAttributes(float* attributes);
	uint16_t* WriteIndices(uint16_t* indices, uint16_t offset);
};
