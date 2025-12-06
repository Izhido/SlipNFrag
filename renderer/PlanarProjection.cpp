#include "PlanarProjection.h"
#include "AppState.h"

const float PlanarProjection::distance = 1.0f;
const float PlanarProjection::verticalAngle = (60 * M_PI / 180) / 1.6;
const float PlanarProjection::screenLowerLimit = PlanarProjection::distance * tan(PlanarProjection::verticalAngle / 2);
const float PlanarProjection::keyboardLowerLimit = PlanarProjection::screenLowerLimit / 2;
const float PlanarProjection::epsilon = 1e-5;

bool PlanarProjection::HitPoint(AppState& appState, const XrVector4f& controllerOrigin, const XrVector4f& controllerDirection, float& x, float& y)
{
	const XrVector3f planeNormal { 0, 0, -PlanarProjection::distance };
	XrVector3f controllerDirection3 { controllerDirection.x, controllerDirection.y, controllerDirection.z };
	auto dotNormal = XrVector3f_Dot(&planeNormal, &controllerDirection3);
	if (fabs(dotNormal) < epsilon)
	{
		return false;
	}
	XrVector3f planeOrigin { 0, 0, -PlanarProjection::distance };
	XrVector3f delta { planeOrigin.x - controllerOrigin.x, planeOrigin.y - controllerOrigin.y, planeOrigin.z - controllerOrigin.z };
	auto dotDelta = XrVector3f_Dot(&planeNormal, &delta);
	float t = dotDelta / dotNormal;
	if (t <= 0)
	{
		return false;
	}
	XrVector3f intersection { controllerOrigin.x + t * controllerDirection.x, controllerOrigin.y + t * controllerDirection.y, controllerOrigin.z + t * controllerDirection.z };
	if (intersection.x < -0.5 || intersection.x > 0.5)
	{
		return false;
	}
	x = intersection.x + 0.5f;
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
	if (rejection2d >= distance)
	{
		return false;
	}
	auto distanceToHitPoint = sqrt(distance * distance - rejection2d * rejection2d);
	auto vertical = controllerOrigin.y - appState.KeyboardHitOffsetY + controllerDirection.y * (projection2d + distanceToHitPoint) / length2d;
	y = (distance * verticalAngle / 2 - vertical) / (distance * verticalAngle);
    return true;
}

bool PlanarProjection::HitPoint(AppState& appState, const Controller& controller, float& x, float& y)
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
	return HitPoint(appState, controllerOrigin, controllerDirection, x, y);
}

bool PlanarProjection::HitPointForScreenMode(AppState& appState, const Controller& controller, float& x, float& y)
{
	XrMatrix4x4f controllerTransform;
	XrMatrix4x4f_CreateFromQuaternion(&controllerTransform, &controller.SpaceLocation.pose.orientation);
	XrVector4f untransformedControllerDirection { 0, 0, -1, 0 };
	XrVector4f controllerDirection;
	XrMatrix4x4f_TransformVector4f(&controllerDirection, &controllerTransform, &untransformedControllerDirection);
	auto& controllerOrigin3d = controller.SpaceLocation.pose.position;
	XrVector4f controllerOrigin { controllerOrigin3d.x, controllerOrigin3d.y, controllerOrigin3d.z, 0 };
	return HitPoint(appState, controllerOrigin, controllerDirection, x, y);
}
