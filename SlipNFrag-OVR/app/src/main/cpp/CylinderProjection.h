#pragma once

#include "VrApi_Helpers.h"
#include "Controller.h"

struct AppState;

struct CylinderProjection
{
	static const float density;
	static const float radius;
	static const float horizontalAngle;
	static const float verticalAngle;
	static const float epsilon;

	static void Setup(AppState& appState, ovrLayerCylinder2& layer);
	static void Setup(AppState& appState, ovrLayerCylinder2& layer, float yaw, ovrTextureSwapChain* swapChain);
	static void Setup(AppState& appState, ovrLayerCylinder2& layer, const ovrMatrix4f* transform, ovrTextureSwapChain* swapChain, int index);
	static bool HitPoint(AppState& appState, Controller& controller, float& x, float& y);
	static bool HitPointForScreenMode(AppState& appState, Controller& controller, float& x, float& y);
};
