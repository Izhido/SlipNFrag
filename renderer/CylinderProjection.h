#pragma once

#include "Controller.h"

struct CylinderProjection
{
	static const float radius;
	static const float horizontalAngle;
	static const float verticalAngle;
	static const float screenLowerLimit;
	static const float keyboardLowerLimit;
	static const float epsilon;

	static bool HitPoint(struct AppState& appState, const XrVector4f& controllerOrigin, const XrVector4f& controllerDirection, float& x, float& y);
	static bool HitPoint(AppState& appState, const Controller& controller, float& x, float& y);
	static bool HitPointForScreenMode(AppState& appState, const Controller& controller, float& x, float& y);
};
