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
			if (i == XR_HAND_JOINT_PALM_EXT)
			{
				XrVector3f untransformedBack { 0, 0, 1 };
				XrMatrix4x4f_TransformVector3f(&palmBack, &transform, &untransformedBack);
			}
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
	if (validJoints[XR_HAND_JOINT_THUMB_PROXIMAL_EXT] && validJoints[XR_HAND_JOINT_THUMB_METACARPAL_EXT])
	{
		nearJoints[jointCount] = XR_HAND_JOINT_THUMB_PROXIMAL_EXT;
		farJoints[jointCount] = XR_HAND_JOINT_THUMB_METACARPAL_EXT;
		jointCount++;
	}
	if (validJoints[XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT] && validJoints[XR_HAND_JOINT_INDEX_PROXIMAL_EXT] && validJoints[XR_HAND_JOINT_INDEX_METACARPAL_EXT] &&
		validJoints[XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT] && validJoints[XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT] &&
		validJoints[XR_HAND_JOINT_RING_INTERMEDIATE_EXT] && validJoints[XR_HAND_JOINT_RING_PROXIMAL_EXT] &&
		validJoints[XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT] && validJoints[XR_HAND_JOINT_LITTLE_PROXIMAL_EXT] && validJoints[XR_HAND_JOINT_LITTLE_METACARPAL_EXT] &&
		validJoints[XR_HAND_JOINT_PALM_EXT] && validJoints[XR_HAND_JOINT_WRIST_EXT])
	{
		palmJoints[0] = XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT;
		palmJoints[1] = XR_HAND_JOINT_INDEX_PROXIMAL_EXT;
		palmJoints[2] = XR_HAND_JOINT_INDEX_METACARPAL_EXT;
		palmJoints[3] = XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT;
		palmJoints[4] = XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT;
		palmJoints[5] = XR_HAND_JOINT_RING_INTERMEDIATE_EXT;
		palmJoints[6] = XR_HAND_JOINT_RING_PROXIMAL_EXT;
		palmJoints[7] = XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT;
		palmJoints[8] = XR_HAND_JOINT_LITTLE_PROXIMAL_EXT;
		palmJoints[9] = XR_HAND_JOINT_LITTLE_METACARPAL_EXT;
		palmJoints[10] = XR_HAND_JOINT_PALM_EXT;
		palmJoints[11] = XR_HAND_JOINT_WRIST_EXT;
		palmAvailable = true;
	}
	auto vertexCount = 6 * 4 * jointCount;
	indexCount = 6 * 6 * jointCount;
	if (palmAvailable)
	{
		vertexCount += 6 * 4 * 4;
		indexCount += 6 * 6 * 4;
	}
	attributeCount = vertexCount;
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

void HandTracker::WriteVertices(bool isRightHand, float *vertices)
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
	if (palmAvailable)
	{
		auto& indexIntermediate = jointLocations[palmJoints[0]].pose.position;
		auto& indexProximal = jointLocations[palmJoints[1]].pose.position;
		auto indexProximalRadius = jointLocations[palmJoints[1]].radius;
		auto& indexProximalRight = jointRight[palmJoints[1]];
		auto& indexMetacarpal = jointLocations[palmJoints[2]].pose.position;
		auto indexMetacarpalRadius = jointLocations[palmJoints[2]].radius;
		auto& middleIntermediate = jointLocations[palmJoints[3]].pose.position;
		auto& middleProximal = jointLocations[palmJoints[4]].pose.position;
		auto middleProximalRadius = jointLocations[palmJoints[4]].radius;
		auto& ringIntermediate = jointLocations[palmJoints[5]].pose.position;
		auto& ringProximal = jointLocations[palmJoints[6]].pose.position;
		auto ringProximalRadius = jointLocations[palmJoints[6]].radius;
		auto& littleIntermediate = jointLocations[palmJoints[7]].pose.position;
		auto& littleProximal = jointLocations[palmJoints[8]].pose.position;
		auto littleProximalRadius = jointLocations[palmJoints[8]].radius;
		auto& littleProximalRight = jointRight[palmJoints[8]];
		auto& littleMetacarpal = jointLocations[palmJoints[9]].pose.position;
		auto littleMetacarpalRadius = jointLocations[palmJoints[9]].radius;
		auto& palmRight = jointRight[palmJoints[10]];
		auto& palmUp = jointUp[palmJoints[10]];
		auto& wrist = jointLocations[palmJoints[11]].pose.position;
		auto wristRadius = jointLocations[palmJoints[11]].radius;

		auto deltaX = indexMetacarpal.x - littleMetacarpal.x;
		auto deltaY = indexMetacarpal.y - littleMetacarpal.y;
		auto deltaZ = indexMetacarpal.z - littleMetacarpal.z;

		auto palmFarWidth = sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
		palmFarWidth += (indexMetacarpalRadius + littleMetacarpalRadius);
		auto palmFarHalfWidth = palmFarWidth / 2;
		auto palmFarQuarterWidth = palmFarWidth / 4;

		auto indexMidX = (indexIntermediate.x + indexProximal.x) / 2;
		auto indexMidY = (indexIntermediate.y + indexProximal.y) / 2;
		auto indexMidZ = (indexIntermediate.z + indexProximal.z) / 2;
		auto middleMidX = (middleIntermediate.x + middleProximal.x) / 2;
		auto middleMidY = (middleIntermediate.y + middleProximal.y) / 2;
		auto middleMidZ = (middleIntermediate.z + middleProximal.z) / 2;
		auto ringMidX = (ringIntermediate.x + ringProximal.x) / 2;
		auto ringMidY = (ringIntermediate.y + ringProximal.y) / 2;
		auto ringMidZ = (ringIntermediate.z + ringProximal.z) / 2;
		auto littleMidX = (littleIntermediate.x + littleProximal.x) / 2;
		auto littleMidY = (littleIntermediate.y + littleProximal.y) / 2;
		auto littleMidZ = (littleIntermediate.z + littleProximal.z) / 2;

		auto indexToMiddleMidX = (indexMidX + middleMidX) / 2;
		auto indexToMiddleMidY = (indexMidY + middleMidY) / 2;
		auto indexToMiddleMidZ = (indexMidZ + middleMidZ) / 2;
		auto middleToRingMidX = (middleMidX + ringMidX) / 2;
		auto middleToRingMidY = (middleMidY + ringMidY) / 2;
		auto middleToRingMidZ = (middleMidZ + ringMidZ) / 2;
		auto ringToLittleMidX = (ringMidX + littleMidX) / 2;
		auto ringToLittleMidY = (ringMidY + littleMidY) / 2;
		auto ringToLittleMidZ = (ringMidZ + littleMidZ) / 2;

		auto indexToMiddleRadius = (indexProximalRadius + middleProximalRadius) / 2;
		auto middleToRingRadius = (middleProximalRadius + ringProximalRadius) / 2;
		auto ringToLittleRadius = (ringProximalRadius + littleProximalRadius) / 2;

		auto factor = (isRightHand ? -1.f : 1.f);

		auto palmBackX = wrist.x - palmBack.x * wristRadius;
		auto palmBackY = wrist.y - palmBack.y * wristRadius;
		auto palmBackZ = wrist.z - palmBack.z * wristRadius;

		auto f0x = palmBackX - palmRight.x * factor * palmFarHalfWidth - palmUp.x * wristRadius;
		auto f0y = palmBackY - palmRight.y * factor * palmFarHalfWidth - palmUp.y * wristRadius;
		auto f0z = palmBackZ - palmRight.z * factor * palmFarHalfWidth - palmUp.z * wristRadius;
		auto f1x = palmBackX - palmRight.x * factor * palmFarQuarterWidth - palmUp.x * wristRadius;
		auto f1y = palmBackY - palmRight.y * factor * palmFarQuarterWidth - palmUp.y * wristRadius;
		auto f1z = palmBackZ - palmRight.z * factor * palmFarQuarterWidth - palmUp.z * wristRadius;
		auto f2x = palmBackX - palmRight.x * factor * palmFarQuarterWidth + palmUp.x * wristRadius;
		auto f2y = palmBackY - palmRight.y * factor * palmFarQuarterWidth + palmUp.y * wristRadius;
		auto f2z = palmBackZ - palmRight.z * factor * palmFarQuarterWidth + palmUp.z * wristRadius;
		auto f3x = palmBackX - palmRight.x * factor * palmFarHalfWidth + palmUp.x * wristRadius;
		auto f3y = palmBackY - palmRight.y * factor * palmFarHalfWidth + palmUp.y * wristRadius;
		auto f3z = palmBackZ - palmRight.z * factor * palmFarHalfWidth + palmUp.z * wristRadius;

		auto n0x = indexMidX - indexProximalRight.x * factor * indexProximalRadius - palmUp.x * indexProximalRadius;
		auto n0y = indexMidY - indexProximalRight.y * factor * indexProximalRadius - palmUp.y * indexProximalRadius;
		auto n0z = indexMidZ - indexProximalRight.z * factor * indexProximalRadius - palmUp.z * indexProximalRadius;
		auto n1x = indexToMiddleMidX - palmUp.x * indexToMiddleRadius;
		auto n1y = indexToMiddleMidY - palmUp.y * indexToMiddleRadius;
		auto n1z = indexToMiddleMidZ - palmUp.z * indexToMiddleRadius;
		auto n2x = indexToMiddleMidX + palmUp.x * indexToMiddleRadius;
		auto n2y = indexToMiddleMidY + palmUp.y * indexToMiddleRadius;
		auto n2z = indexToMiddleMidZ + palmUp.z * indexToMiddleRadius;
		auto n3x = indexMidX - indexProximalRight.x * factor * indexProximalRadius + palmUp.x * indexProximalRadius;
		auto n3y = indexMidY - indexProximalRight.y * factor * indexProximalRadius + palmUp.y * indexProximalRadius;
		auto n3z = indexMidZ - indexProximalRight.z * factor * indexProximalRadius + palmUp.z * indexProximalRadius;

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

		f0x = palmBackX - palmRight.x * factor * palmFarQuarterWidth - palmUp.x * wristRadius;
		f0y = palmBackY - palmRight.y * factor * palmFarQuarterWidth - palmUp.y * wristRadius;
		f0z = palmBackZ - palmRight.z * factor * palmFarQuarterWidth - palmUp.z * wristRadius;
		f1x = palmBackX - palmUp.x * wristRadius;
		f1y = palmBackY - palmUp.y * wristRadius;
		f1z = palmBackZ - palmUp.z * wristRadius;
		f2x = palmBackX + palmUp.x * wristRadius;
		f2y = palmBackY + palmUp.y * wristRadius;
		f2z = palmBackZ + palmUp.z * wristRadius;
		f3x = palmBackX - palmRight.x * factor * palmFarQuarterWidth + palmUp.x * wristRadius;
		f3y = palmBackY - palmRight.y * factor * palmFarQuarterWidth + palmUp.y * wristRadius;
		f3z = palmBackZ - palmRight.z * factor * palmFarQuarterWidth + palmUp.z * wristRadius;

		n0x = indexToMiddleMidX - palmUp.x * indexToMiddleRadius;
		n0y = indexToMiddleMidY - palmUp.y * indexToMiddleRadius;
		n0z = indexToMiddleMidZ - palmUp.z * indexToMiddleRadius;
		n1x = middleToRingMidX - palmUp.x * middleToRingRadius;
		n1y = middleToRingMidY - palmUp.y * middleToRingRadius;
		n1z = middleToRingMidZ - palmUp.z * middleToRingRadius;
		n2x = middleToRingMidX + palmUp.x * middleToRingRadius;
		n2y = middleToRingMidY + palmUp.y * middleToRingRadius;
		n2z = middleToRingMidZ + palmUp.z * middleToRingRadius;
		n3x = indexToMiddleMidX + palmUp.x * indexToMiddleRadius;
		n3y = indexToMiddleMidY + palmUp.y * indexToMiddleRadius;
		n3z = indexToMiddleMidZ + palmUp.z * indexToMiddleRadius;

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

		f0x = palmBackX - palmUp.x * wristRadius;
		f0y = palmBackY - palmUp.y * wristRadius;
		f0z = palmBackZ - palmUp.z * wristRadius;
		f1x = palmBackX + palmRight.x * factor * palmFarQuarterWidth - palmUp.x * wristRadius;
		f1y = palmBackY + palmRight.y * factor * palmFarQuarterWidth - palmUp.y * wristRadius;
		f1z = palmBackZ + palmRight.z * factor * palmFarQuarterWidth - palmUp.z * wristRadius;
		f2x = palmBackX + palmRight.x * factor * palmFarQuarterWidth + palmUp.x * wristRadius;
		f2y = palmBackY + palmRight.y * factor * palmFarQuarterWidth + palmUp.y * wristRadius;
		f2z = palmBackZ + palmRight.z * factor * palmFarQuarterWidth + palmUp.z * wristRadius;
		f3x = palmBackX + palmUp.x * wristRadius;
		f3y = palmBackY + palmUp.y * wristRadius;
		f3z = palmBackZ + palmUp.z * wristRadius;

		n0x = middleToRingMidX - palmUp.x * middleToRingRadius;
		n0y = middleToRingMidY - palmUp.y * middleToRingRadius;
		n0z = middleToRingMidZ - palmUp.z * middleToRingRadius;
		n1x = ringToLittleMidX - palmUp.x * ringToLittleRadius;
		n1y = ringToLittleMidY - palmUp.y * ringToLittleRadius;
		n1z = ringToLittleMidZ - palmUp.z * ringToLittleRadius;
		n2x = ringToLittleMidX + palmUp.x * ringToLittleRadius;
		n2y = ringToLittleMidY + palmUp.y * ringToLittleRadius;
		n2z = ringToLittleMidZ + palmUp.z * ringToLittleRadius;
		n3x = middleToRingMidX + palmUp.x * middleToRingRadius;
		n3y = middleToRingMidY + palmUp.y * middleToRingRadius;
		n3z = middleToRingMidZ + palmUp.z * middleToRingRadius;

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

		f0x = palmBackX + palmRight.x * factor * palmFarQuarterWidth - palmUp.x * wristRadius;
		f0y = palmBackY + palmRight.y * factor * palmFarQuarterWidth - palmUp.y * wristRadius;
		f0z = palmBackZ + palmRight.z * factor * palmFarQuarterWidth - palmUp.z * wristRadius;
		f1x = palmBackX + palmRight.x * factor * palmFarHalfWidth - palmUp.x * wristRadius;
		f1y = palmBackY + palmRight.y * factor * palmFarHalfWidth - palmUp.y * wristRadius;
		f1z = palmBackZ + palmRight.z * factor * palmFarHalfWidth - palmUp.z * wristRadius;
		f2x = palmBackX + palmRight.x * factor * palmFarHalfWidth + palmUp.x * wristRadius;
		f2y = palmBackY + palmRight.y * factor * palmFarHalfWidth + palmUp.y * wristRadius;
		f2z = palmBackZ + palmRight.z * factor * palmFarHalfWidth + palmUp.z * wristRadius;
		f3x = palmBackX + palmRight.x * factor * palmFarQuarterWidth + palmUp.x * wristRadius;
		f3y = palmBackY + palmRight.y * factor * palmFarQuarterWidth + palmUp.y * wristRadius;
		f3z = palmBackZ + palmRight.z * factor * palmFarQuarterWidth + palmUp.z * wristRadius;

		n0x = ringToLittleMidX - palmUp.x * ringToLittleRadius;
		n0y = ringToLittleMidY - palmUp.y * ringToLittleRadius;
		n0z = ringToLittleMidZ - palmUp.z * ringToLittleRadius;
		n1x = littleMidX + littleProximalRight.x * factor * littleProximalRadius - palmUp.x * littleProximalRadius;
		n1y = littleMidY + littleProximalRight.y * factor * littleProximalRadius - palmUp.y * littleProximalRadius;
		n1z = littleMidZ + littleProximalRight.z * factor * littleProximalRadius - palmUp.z * littleProximalRadius;
		n2x = littleMidX + littleProximalRight.x * factor * littleProximalRadius + palmUp.x * littleProximalRadius;
		n2y = littleMidY + littleProximalRight.y * factor * littleProximalRadius + palmUp.y * littleProximalRadius;
		n2z = littleMidZ + littleProximalRight.z * factor * littleProximalRadius + palmUp.z * littleProximalRadius;
		n3x = ringToLittleMidX + palmUp.x * ringToLittleRadius;
		n3y = ringToLittleMidY + palmUp.y * ringToLittleRadius;
		n3z = ringToLittleMidZ + palmUp.z * ringToLittleRadius;

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

void HandTracker::WriteAttributes(float *attributes) const
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

	if (palmAvailable)
	{
		// Index to middle section:
		*attributes++ = 0;
		*attributes++ = 0;
		*attributes++ = 0.95;
		*attributes++ = 0;
		*attributes++ = 0.95;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;

		*attributes++ = 0;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 0.95;
		*attributes++ = 0;
		*attributes++ = 0.95;

		*attributes++ = 0;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;

		*attributes++ = 0;
		*attributes++ = 0.05;
		*attributes++ = 1;
		*attributes++ = 0.05;
		*attributes++ = 1;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;

		*attributes++ = 0;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;

		*attributes++ = 0.05;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 1;
		*attributes++ = 0.05;
		*attributes++ = 1;

		// Middle to ring, ring to little sections:
		for (size_t i = 1; i < 3; i++)
		{
			*attributes++ = 0.05;
			*attributes++ = 0;
			*attributes++ = 0.95;
			*attributes++ = 0;
			*attributes++ = 0.95;
			*attributes++ = 1;
			*attributes++ = 0.05;
			*attributes++ = 1;

			for (size_t j = 1; j < 5; j++)
			{
				*attributes++ = 0;
				*attributes++ = 0.05;
				*attributes++ = 1;
				*attributes++ = 0.05;
				*attributes++ = 1;
				*attributes++ = 0.95;
				*attributes++ = 0;
				*attributes++ = 0.95;
			}

			*attributes++ = 0.05;
			*attributes++ = 0;
			*attributes++ = 0.95;
			*attributes++ = 0;
			*attributes++ = 0.95;
			*attributes++ = 1;
			*attributes++ = 0.05;
			*attributes++ = 1;
		}

		// Ring to little section:
		*attributes++ = 0.05;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 1;
		*attributes++ = 0.05;
		*attributes++ = 1;

		*attributes++ = 0;
		*attributes++ = 0.05;
		*attributes++ = 1;
		*attributes++ = 0.05;
		*attributes++ = 1;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;

		*attributes++ = 0;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;

		*attributes++ = 0;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 0.95;
		*attributes++ = 0;
		*attributes++ = 0.95;

		*attributes++ = 0;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;

		*attributes++ = 0;
		*attributes++ = 0;
		*attributes++ = 0.95;
		*attributes++ = 0;
		*attributes++ = 0.95;
		*attributes++ = 1;
		*attributes++ = 0;
		*attributes++ = 1;
	}
}

void HandTracker::WriteIndices16(bool isRightHand, uint16_t *indices, uint16_t& offset) const
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

	if (palmAvailable)
	{
		for (size_t i = 0; i < 4; i++)
		{
			if (isRightHand)
			{
				for (size_t j = 0; j < 6; j++)
				{
					*indices++ = 0 + offset;
					*indices++ = 3 + offset;
					*indices++ = 2 + offset;
					*indices++ = 2 + offset;
					*indices++ = 1 + offset;
					*indices++ = 0 + offset;

					offset += 4;
				}
			}
			else
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
	}
}
