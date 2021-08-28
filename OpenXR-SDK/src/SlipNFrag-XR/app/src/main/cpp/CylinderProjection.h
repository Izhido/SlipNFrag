#pragma once

#include "Controller.h"

struct AppState;

struct CylinderProjection
{
	static const float radius;
	static const float horizontalAngle;
	static const float verticalAngle;
	static const float screenLowerLimit;
	static const float keyboardLowerLimit;
	static const float epsilon;

	static bool HitPoint(AppState& appState, Controller& controller, float& x, float& y);
	static bool HitPointForScreenMode(AppState& appState, Controller& controller, float& x, float& y);
};
