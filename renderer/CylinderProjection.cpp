#include "CylinderProjection.h"
#include "AppState.h"

const float CylinderProjection::radius = 1.0f;
const float CylinderProjection::horizontalAngle = 60 * M_PI / 180;
const float CylinderProjection::verticalAngle = horizontalAngle / 1.6;
const float CylinderProjection::screenLowerLimit = CylinderProjection::radius * tan(CylinderProjection::verticalAngle / 2);
const float CylinderProjection::keyboardLowerLimit = CylinderProjection::screenLowerLimit / 2;
const float CylinderProjection::epsilon = 1e-5;

bool CylinderProjection::HitPoint(AppState& appState, Controller& controller, float& x, float& y)
{
	if (!appState.CameraLocationIsValid)
	{
		return false;
	}
	float positionX = appState.CameraLocation.pose.position.x;
	float positionY = appState.CameraLocation.pose.position.y;
	float positionZ = appState.CameraLocation.pose.position.z;
	XrMatrix4x4f controllerTransform;
	XrMatrix4x4f_CreateFromQuaternion(&controllerTransform, &controller.SpaceLocation.pose.orientation);
	XrVector4f untransformedControllerDirection { 0, 0, -1, 0 };
	XrVector4f controllerDirection;
	XrMatrix4x4f_TransformVector4f(&controllerDirection, &controllerTransform, &untransformedControllerDirection);
	XrMatrix4x4f rotation;
	XrMatrix4x4f_CreateFromQuaternion(&rotation, &appState.CameraLocation.pose.orientation);
	XrMatrix4x4f translation;
	XrMatrix4x4f_CreateTranslation(&translation, positionX, positionY, positionZ);
	XrMatrix4x4f inverseView;
	XrMatrix4x4f_Multiply(&inverseView, &translation, &rotation);
	XrMatrix4x4f transform;
	XrMatrix4x4f_Invert(&transform, &inverseView);
	XrVector3f origin3d { positionX, positionY, positionZ };
	auto& controllerOrigin3d = controller.SpaceLocation.pose.position;
	XrVector4f untransformedControllerOrigin { controllerOrigin3d.x - origin3d.x, controllerOrigin3d.y - origin3d.y, controllerOrigin3d.z - origin3d.z, 0 };
	XrVector4f controllerOrigin;
	XrMatrix4x4f_TransformVector4f(&controllerOrigin, &transform, &untransformedControllerOrigin);
	XrVector4f untransformedControllerTip { controllerOrigin3d.x - origin3d.x + controllerDirection.x, controllerOrigin3d.y - origin3d.y + controllerDirection.y, controllerOrigin3d.z - origin3d.z + controllerDirection.z, 0 };
	XrVector4f controllerTip;
	XrMatrix4x4f_TransformVector4f(&controllerTip, &transform, &untransformedControllerTip);
	controllerDirection.x = controllerTip.x - controllerOrigin.x;
	controllerDirection.y = controllerTip.y - controllerOrigin.y;
	controllerDirection.z = controllerTip.z - controllerOrigin.z;
	auto lengthSquared2d = controllerDirection.x * controllerDirection.x + controllerDirection.z * controllerDirection.z;
	if (lengthSquared2d < epsilon)
	{
		return false;
	}
	auto length2d = sqrt(lengthSquared2d);
	XrVector2f controllerDirection2d { controllerDirection.x / length2d, controllerDirection.z / length2d };
	auto projection2d = -controllerOrigin.x * controllerDirection2d.x - controllerOrigin.z * controllerDirection2d.y;
	XrVector2f projected2d { controllerOrigin.x + controllerDirection2d.x * projection2d, controllerOrigin.z + controllerDirection2d.y * projection2d };
	auto rejection2d = sqrt(projected2d.x * projected2d.x + projected2d.y * projected2d.y);
	if (rejection2d >= radius)
	{
		return false;
	}
	auto distanceToHitPoint = sqrt(radius * radius - rejection2d * rejection2d);
	XrVector2f hitPoint2d { projected2d.x + controllerDirection2d.x * distanceToHitPoint, projected2d.y + controllerDirection2d.y * distanceToHitPoint };
	auto angle = atan2(hitPoint2d.y, hitPoint2d.x);
	if (angle < -M_PI / 2 - horizontalAngle / 2 || angle >= -M_PI / 2 + horizontalAngle / 2)
	{
		return false;
	}
	auto vertical = controllerOrigin.y - appState.KeyboardHitOffsetY + controllerDirection.y * (projection2d + distanceToHitPoint) / length2d;
	x = (angle + M_PI / 2 + horizontalAngle / 2) / horizontalAngle;
	y = (radius * verticalAngle / 2 - vertical) / (radius * verticalAngle);
	return true;
}

bool CylinderProjection::HitPointForScreenMode(AppState& appState, Controller& controller, float& x, float& y)
{
	XrMatrix4x4f controllerTransform;
	XrMatrix4x4f_CreateFromQuaternion(&controllerTransform, &controller.SpaceLocation.pose.orientation);
	XrVector4f untransformedControllerDirection { 0, 0, -1, 0 };
	XrVector4f controllerDirection;
	XrMatrix4x4f_TransformVector4f(&controllerDirection, &controllerTransform, &untransformedControllerDirection);
	auto& controllerOrigin3d = controller.SpaceLocation.pose.position;
	XrVector4f controllerOrigin { controllerOrigin3d.x, controllerOrigin3d.y, controllerOrigin3d.z, 0 };
	XrVector4f controllerTip { controllerOrigin3d.x + controllerDirection.x, controllerOrigin3d.y + controllerDirection.y, controllerOrigin3d.z + controllerDirection.z, 0 };
	controllerDirection.x = controllerTip.x - controllerOrigin.x;
	controllerDirection.y = controllerTip.y - controllerOrigin.y;
	controllerDirection.z = controllerTip.z - controllerOrigin.z;
	auto lengthSquared2d = controllerDirection.x * controllerDirection.x + controllerDirection.z * controllerDirection.z;
	if (lengthSquared2d < epsilon)
	{
		return false;
	}
	auto length2d = sqrt(lengthSquared2d);
	XrVector2f controllerDirection2d { controllerDirection.x / length2d, controllerDirection.z / length2d };
	auto projection2d = -controllerOrigin.x * controllerDirection2d.x - controllerOrigin.z * controllerDirection2d.y;
	XrVector2f projected2d { controllerOrigin.x + controllerDirection2d.x * projection2d, controllerOrigin.z + controllerDirection2d.y * projection2d };
	auto rejection2d = sqrt(projected2d.x * projected2d.x + projected2d.y * projected2d.y);
	if (rejection2d >= radius)
	{
		return false;
	}
	auto distanceToHitPoint = sqrt(radius * radius - rejection2d * rejection2d);
	XrVector2f hitPoint2d { projected2d.x + controllerDirection2d.x * distanceToHitPoint, projected2d.y + controllerDirection2d.y * distanceToHitPoint };
	auto angle = atan2(hitPoint2d.y, hitPoint2d.x);
	if (angle < -M_PI / 2 - horizontalAngle / 2 || angle >= -M_PI / 2 + horizontalAngle / 2)
	{
		return false;
	}
	auto vertical = controllerOrigin.y - appState.KeyboardHitOffsetY + controllerDirection.y * (projection2d + distanceToHitPoint) / length2d;
	x = (angle + M_PI / 2 + horizontalAngle / 2) / horizontalAngle;
	y = (radius * verticalAngle / 2 - vertical) / (radius * verticalAngle);
	return true;
}
