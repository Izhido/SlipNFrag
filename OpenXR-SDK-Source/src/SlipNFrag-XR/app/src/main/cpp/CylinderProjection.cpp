#include "CylinderProjection.h"
#include "AppState.h"

const float CylinderProjection::radius = 1.0f;
const float CylinderProjection::horizontalAngle = 76.75 * M_PI / 180;
const float CylinderProjection::verticalAngle = horizontalAngle / 1.6;
const float CylinderProjection::screenLowerLimit = 
		(CylinderProjection::radius * CylinderProjection::radius + 
		CylinderProjection::radius * CylinderProjection::radius - 
		2 * CylinderProjection::radius * CylinderProjection::radius * cos(CylinderProjection::horizontalAngle)) / (1.6 * 2);
const float CylinderProjection::keyboardLowerLimit = CylinderProjection::screenLowerLimit / 2;

bool CylinderProjection::HitPoint(AppState& appState, Controller& controller, float& x, float& y)
{
	/*XrMatrix4x4f controllerTransform;
	XrMatrix4x4f_CreateFromQuaternion(&controllerTransform, &controller.SpaceLocation.pose.orientation);
	XrVector4f untransformedControllerDirection { 0, 0, -1, 0 };
	XrVector4f controllerDirection;
	XrMatrix4x4f_TransformVector4f(&controllerDirection, &controllerTransform, &untransformedControllerDirection);
	auto transform = vrapi_GetViewMatrixFromPose(&appState.Tracking.HeadPose.Pose);
	auto& origin3d = appState.Tracking.HeadPose.Pose.Position;
	auto& controllerOrigin3d = controller.Tracking.HeadPose.Pose.Position;
	ovrVector4f controllerOrigin { controllerOrigin3d.x - origin3d.x, controllerOrigin3d.y - origin3d.y, controllerOrigin3d.z - origin3d.z, 0 };
	controllerOrigin = ovrVector4f_MultiplyMatrix4f(&transform, &controllerOrigin);
	ovrVector4f controllerTip { controllerOrigin3d.x - origin3d.x + controllerDirection.x, controllerOrigin3d.y - origin3d.y + controllerDirection.y, controllerOrigin3d.z - origin3d.z + controllerDirection.z, 0 };
	controllerTip = ovrVector4f_MultiplyMatrix4f(&transform, &controllerTip);
	controllerDirection.x = controllerTip.x - controllerOrigin.x;
	controllerDirection.y = controllerTip.y - controllerOrigin.y;
	controllerDirection.z = controllerTip.z - controllerOrigin.z;
	auto lengthSquared2d = controllerDirection.x * controllerDirection.x + controllerDirection.z * controllerDirection.z;
	if (lengthSquared2d < epsilon)
	{
		return false;
	}
	auto length2d = sqrt(lengthSquared2d);
	ovrVector2f controllerDirection2d { controllerDirection.x / length2d, controllerDirection.z / length2d };
	auto projection2d = -controllerOrigin.x * controllerDirection2d.x - controllerOrigin.z * controllerDirection2d.y;
	ovrVector2f projected2d { controllerOrigin.x + controllerDirection2d.x * projection2d, controllerOrigin.z + controllerDirection2d.y * projection2d };
	auto rejection2d = sqrt(projected2d.x * projected2d.x + projected2d.y * projected2d.y);
	if (rejection2d >= radius)
	{
		return false;
	}
	auto distanceToHitPoint = sqrt(radius * radius - rejection2d * rejection2d);
	ovrVector2f hitPoint2d { projected2d.x + controllerDirection2d.x * distanceToHitPoint, projected2d.y + controllerDirection2d.y * distanceToHitPoint };
	auto angle = atan2(hitPoint2d.y, hitPoint2d.x);
	if (angle < -M_PI / 2 - horizontalAngle / 2 || angle >= -M_PI / 2 + horizontalAngle / 2)
	{
		return false;
	}
	auto vertical = controllerOrigin.y - appState.KeyboardHitOffsetY + controllerDirection.y * (projection2d + distanceToHitPoint) / length2d;
	x = (angle + M_PI / 2 + horizontalAngle / 2) / horizontalAngle;
	y = (radius * verticalAngle / 2 - vertical) / (radius * verticalAngle);
	return true;*/return false;
}

bool CylinderProjection::HitPointForScreenMode(AppState& appState, Controller& controller, float& x, float& y)
{
	/*auto controllerTransform = ovrMatrix4f_CreateFromQuaternion(&controller.Tracking.HeadPose.Pose.Orientation);
	ovrVector4f controllerDirection { 0, 0, -1, 0 };
	controllerDirection = ovrVector4f_MultiplyMatrix4f(&controllerTransform, &controllerDirection);
	auto& controllerOrigin3d = controller.Tracking.HeadPose.Pose.Position;
	ovrVector4f controllerOrigin { controllerOrigin3d.x, controllerOrigin3d.y, controllerOrigin3d.z, 0 };
	ovrVector4f controllerTip { controllerOrigin3d.x + controllerDirection.x, controllerOrigin3d.y + controllerDirection.y, controllerOrigin3d.z + controllerDirection.z, 0 };
	controllerDirection.x = controllerTip.x - controllerOrigin.x;
	controllerDirection.y = controllerTip.y - controllerOrigin.y;
	controllerDirection.z = controllerTip.z - controllerOrigin.z;
	auto lengthSquared2d = controllerDirection.x * controllerDirection.x + controllerDirection.z * controllerDirection.z;
	if (lengthSquared2d < epsilon)
	{
		return false;
	}
	auto length2d = sqrt(lengthSquared2d);
	ovrVector2f controllerDirection2d { controllerDirection.x / length2d, controllerDirection.z / length2d };
	auto projection2d = -controllerOrigin.x * controllerDirection2d.x - controllerOrigin.z * controllerDirection2d.y;
	ovrVector2f projected2d { controllerOrigin.x + controllerDirection2d.x * projection2d, controllerOrigin.z + controllerDirection2d.y * projection2d };
	auto rejection2d = sqrt(projected2d.x * projected2d.x + projected2d.y * projected2d.y);
	if (rejection2d >= radius)
	{
		return false;
	}
	auto distanceToHitPoint = sqrt(radius * radius - rejection2d * rejection2d);
	ovrVector2f hitPoint2d { projected2d.x + controllerDirection2d.x * distanceToHitPoint, projected2d.y + controllerDirection2d.y * distanceToHitPoint };
	auto angle = atan2(hitPoint2d.y, hitPoint2d.x);
	if (angle < -M_PI / 2 - horizontalAngle / 2 || angle >= -M_PI / 2 + horizontalAngle / 2)
	{
		return false;
	}
	auto vertical = controllerOrigin.y - appState.KeyboardHitOffsetY + controllerDirection.y * (projection2d + distanceToHitPoint) / length2d;
	x = (angle + M_PI / 2 + horizontalAngle / 2) / horizontalAngle;
	y = (radius * verticalAngle / 2 - vertical) / (radius * verticalAngle);
	return true;*/return false;
}
