#pragma once

#include <openxr/openxr.h>
#include <vulkan/vulkan.h>

struct Controller
{
	XrSpaceLocation SpaceLocation;
	bool PoseIsValid;

	VkDeviceSize VerticesSize() const;
	VkDeviceSize AttributesSize() const;
	VkDeviceSize IndicesSize() const;
	float* WriteVertices(float* vertices);
	float* WriteAttributes(float* attributes);
	static unsigned char* WriteIndices8(unsigned char* indices, unsigned char offset);
	static uint16_t* WriteIndices16(uint16_t* indices, uint16_t offset);
};
