#pragma once

#include "AppState.h"
#include "FileLoader.h"

struct AppState_oxr : public AppState
{
	pid_t EngineThreadId;
	pid_t RenderThreadId;
	PFN_xrSetAndroidApplicationThreadKHR xrSetAndroidApplicationThreadKHR;
	bool Terminated;

	void RenderScreen(ScreenPerFrame& perFrame);
};
