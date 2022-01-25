#pragma once

#include <openxr/openxr.h>

struct Controller
{
	XrSpaceLocation SpaceLocation;
	bool PoseIsValid;

	float* WriteVertices(float* vertices);
	static float* WriteAttributes(float* attributes);
	static unsigned char* WriteIndices8(unsigned char* indices, unsigned char offset);
	static uint16_t* WriteIndices16(uint16_t* indices, uint16_t offset);
};
