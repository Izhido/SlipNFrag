#pragma once

#include "Controller.h"

struct AppState;

struct PlanarProjection
{
	static const float distance;
	static const float verticalAngle;
	static const float screenLowerLimit;
	static const float keyboardLowerLimit;
	static const float epsilon;

	static bool HitPoint(AppState& appState, const XrVector4f& controllerOrigin, const XrVector4f& controllerDirection, float& x, float& y);
	static bool HitPoint(AppState& appState, const Controller& controller, float& x, float& y);
	static bool HitPointForScreenMode(AppState& appState, const Controller& controller, float& x, float& y);
};
