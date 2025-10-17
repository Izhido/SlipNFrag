#include "HandTracker.h"
#include "common/xr_linear.h"
#include <vector>

VkDeviceSize HandTracker::VerticesSize()
{
	XrMatrix4x4f transform;

	XrVector3f untransformedRight { -1, 0, 0 };
	XrVector3f untransformedUp { 0, 1, 0 };

	for (size_t i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++)
	{
		validJoints[i] = (jointLocations[i].locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) == (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT);
		if (validJoints[i])
		{
			XrMatrix4x4f_CreateFromQuaternion(&transform, &jointLocations[i].pose.orientation);
			XrMatrix4x4f_TransformVector3f(&jointRight[i], &transform, &untransformedRight);
			XrMatrix4x4f_TransformVector3f(&jointUp[i], &transform, &untransformedUp);
		}
	}
	jointCount = 0;
	if (validJoints[XR_HAND_JOINT_INDEX_TIP_EXT] && validJoints[XR_HAND_JOINT_INDEX_DISTAL_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_INDEX_TIP_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_INDEX_DISTAL_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_INDEX_DISTAL_EXT] && validJoints[XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_INDEX_DISTAL_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT] && validJoints[XR_HAND_JOINT_INDEX_PROXIMAL_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_INDEX_PROXIMAL_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_MIDDLE_TIP_EXT] && validJoints[XR_HAND_JOINT_MIDDLE_DISTAL_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_MIDDLE_TIP_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_MIDDLE_DISTAL_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_MIDDLE_DISTAL_EXT] && validJoints[XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_MIDDLE_DISTAL_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT] && validJoints[XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_RING_TIP_EXT] && validJoints[XR_HAND_JOINT_RING_DISTAL_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_RING_TIP_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_RING_DISTAL_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_RING_DISTAL_EXT] && validJoints[XR_HAND_JOINT_RING_INTERMEDIATE_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_RING_DISTAL_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_RING_INTERMEDIATE_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_RING_INTERMEDIATE_EXT] && validJoints[XR_HAND_JOINT_RING_PROXIMAL_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_RING_INTERMEDIATE_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_RING_PROXIMAL_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_LITTLE_TIP_EXT] && validJoints[XR_HAND_JOINT_LITTLE_DISTAL_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_LITTLE_TIP_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_LITTLE_DISTAL_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_LITTLE_DISTAL_EXT] && validJoints[XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_LITTLE_DISTAL_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT] && validJoints[XR_HAND_JOINT_LITTLE_PROXIMAL_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_LITTLE_PROXIMAL_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_THUMB_TIP_EXT] && validJoints[XR_HAND_JOINT_THUMB_DISTAL_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_THUMB_TIP_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_THUMB_DISTAL_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_THUMB_DISTAL_EXT] && validJoints[XR_HAND_JOINT_THUMB_PROXIMAL_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_THUMB_DISTAL_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_THUMB_PROXIMAL_EXT;
		jointCount++;
	}
	auto vertexCount = 6 * 4 * jointCount;
	attributeCount = vertexCount;
	indexCount = 6 * 6 * jointCount;
	return vertexCount * 3 * sizeof(float);
}

VkDeviceSize HandTracker::AttributesSize() const
{
	return attributeCount * 2 * sizeof(float);
}

VkDeviceSize HandTracker::IndicesSize() const
{
	return indexCount;
}

void HandTracker::WriteVertices(float *vertices)
{
	for (size_t i = 0; i < jointCount; i++)
	{
		auto nearJoint = nearJoints[i];
		auto farJoint = farJoints[i];
		auto& near = jointLocations[nearJoint].pose.position;
		auto& far = jointLocations[farJoint].pose.position;
		auto& right = jointRight[farJoint];
		auto& up = jointUp[farJoint];
		auto radiusNear = jointLocations[nearJoint].radius;
		auto radiusFar = jointLocations[farJoint].radius;

		auto f0x = far.x - right.x * radiusFar - up.x * radiusFar;
		auto f0y = far.y - right.y * radiusFar - up.y * radiusFar;
		auto f0z = far.z - right.z * radiusFar - up.z * radiusFar;
		auto f1x = far.x + right.x * radiusFar - up.x * radiusFar;
		auto f1y = far.y + right.y * radiusFar - up.y * radiusFar;
		auto f1z = far.z + right.z * radiusFar - up.z * radiusFar;
		auto f2x = far.x + right.x * radiusFar + up.x * radiusFar;
		auto f2y = far.y + right.y * radiusFar + up.y * radiusFar;
		auto f2z = far.z + right.z * radiusFar + up.z * radiusFar;
		auto f3x = far.x - right.x * radiusFar + up.x * radiusFar;
		auto f3y = far.y - right.y * radiusFar + up.y * radiusFar;
		auto f3z = far.z - right.z * radiusFar + up.z * radiusFar;

		auto n0x = near.x - right.x * radiusNear - up.x * radiusNear;
		auto n0y = near.y - right.y * radiusNear - up.y * radiusNear;
		auto n0z = near.z - right.z * radiusNear - up.z * radiusNear;
		auto n1x = near.x + right.x * radiusNear - up.x * radiusNear;
		auto n1y = near.y + right.y * radiusNear - up.y * radiusNear;
		auto n1z = near.z + right.z * radiusNear - up.z * radiusNear;
		auto n2x = near.x + right.x * radiusNear + up.x * radiusNear;
		auto n2y = near.y + right.y * radiusNear + up.y * radiusNear;
		auto n2z = near.z + right.z * radiusNear + up.z * radiusNear;
		auto n3x = near.x - right.x * radiusNear + up.x * radiusNear;
		auto n3y = near.y - right.y * radiusNear + up.y * radiusNear;
		auto n3z = near.z - right.z * radiusNear + up.z * radiusNear;

		*vertices++ = f0x;
		*vertices++ = f0y;
		*vertices++ = f0z;
		*vertices++ = f1x;
		*vertices++ = f1y;
		*vertices++ = f1z;
		*vertices++ = f2x;
		*vertices++ = f2y;
		*vertices++ = f2z;
		*vertices++ = f3x;
		*vertices++ = f3y;
		*vertices++ = f3z;

		*vertices++ = f0x;
		*vertices++ = f0y;
		*vertices++ = f0z;
		*vertices++ = n0x;
		*vertices++ = n0y;
		*vertices++ = n0z;
		*vertices++ = n1x;
		*vertices++ = n1y;
		*vertices++ = n1z;
		*vertices++ = f1x;
		*vertices++ = f1y;
		*vertices++ = f1z;

		*vertices++ = f1x;
		*vertices++ = f1y;
		*vertices++ = f1z;
		*vertices++ = n1x;
		*vertices++ = n1y;
		*vertices++ = n1z;
		*vertices++ = n2x;
		*vertices++ = n2y;
		*vertices++ = n2z;
		*vertices++ = f2x;
		*vertices++ = f2y;
		*vertices++ = f2z;

		*vertices++ = f2x;
		*vertices++ = f2y;
		*vertices++ = f2z;
		*vertices++ = n2x;
		*vertices++ = n2y;
		*vertices++ = n2z;
		*vertices++ = n3x;
		*vertices++ = n3y;
		*vertices++ = n3z;
		*vertices++ = f3x;
		*vertices++ = f3y;
		*vertices++ = f3z;

		*vertices++ = f3x;
		*vertices++ = f3y;
		*vertices++ = f3z;
		*vertices++ = n3x;
		*vertices++ = n3y;
		*vertices++ = n3z;
		*vertices++ = n0x;
		*vertices++ = n0y;
		*vertices++ = n0z;
		*vertices++ = f0x;
		*vertices++ = f0y;
		*vertices++ = f0z;

		*vertices++ = n1x;
		*vertices++ = n1y;
		*vertices++ = n1z;
		*vertices++ = n0x;
		*vertices++ = n0y;
		*vertices++ = n0z;
		*vertices++ = n3x;
		*vertices++ = n3y;
		*vertices++ = n3z;
		*vertices++ = n2x;
		*vertices++ = n2y;
		*vertices++ = n2z;
	}
}

void HandTracker::WriteAttributes(float *attributes)
{
	for (size_t i = 0; i < jointCount; i++)
	{
		for (size_t j = 0; j < 6; j++)
		{
			*attributes++ = 0;
			*attributes++ = 0;
			*attributes++ = 1;
			*attributes++ = 0;
			*attributes++ = 1;
			*attributes++ = 1;
			*attributes++ = 0;
			*attributes++ = 1;
		}
	}
}

void HandTracker::WriteIndices16(uint16_t *indices, uint16_t& offset) const
{
	for (size_t i = 0; i < jointCount; i++)
	{
		for (size_t j = 0; j < 6; j++)
		{
			*indices++ = 0 + offset;
			*indices++ = 1 + offset;
			*indices++ = 2 + offset;
			*indices++ = 2 + offset;
			*indices++ = 3 + offset;
			*indices++ = 0 + offset;

			offset += 4;
		}
	}
}
