#pragma once

#include <openxr/openxr.h>
#include <vulkan/vulkan.h>

enum TrackedHand
{
	LEFT_TRACKED_HAND,
	RIGHT_TRACKED_HAND
};

struct HandTracker
{
	XrHandTrackerEXT tracker;
	XrHandJointLocationsEXT locations;
	XrHandJointLocationEXT jointLocations[XR_HAND_JOINT_COUNT_EXT];

	bool validJoints[XR_HAND_JOINT_COUNT_EXT];
	XrVector3f jointRight[XR_HAND_JOINT_COUNT_EXT];
	XrVector3f jointUp[XR_HAND_JOINT_COUNT_EXT];
	int nearJoints[XR_HAND_JOINT_COUNT_EXT];
	int farJoints[XR_HAND_JOINT_COUNT_EXT];
	VkDeviceSize attributeCount;
	VkDeviceSize indexCount;
	size_t jointCount;

	VkDeviceSize VerticesSize();
	VkDeviceSize AttributesSize() const;
	VkDeviceSize IndicesSize() const;
	void WriteVertices(float* vertices);
	void WriteAttributes(float* attributes);
	void WriteIndices16(uint16_t* indices, uint16_t& offset) const;
};
