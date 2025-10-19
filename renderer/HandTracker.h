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
	int palmJoints[12];
	bool palmAvailable;
	XrVector3f palmBack;
	VkDeviceSize attributeCount;
	VkDeviceSize indexCount;
	size_t jointCount;

	VkDeviceSize VerticesSize();
	VkDeviceSize AttributesSize() const;
	VkDeviceSize IndicesSize() const;
	void WriteVertices(bool isRightHand, float* vertices);
	void WriteAttributes(float* attributes) const;
	void WriteIndices16(bool isRightHand, uint16_t* indices, uint16_t& offset) const;
};
