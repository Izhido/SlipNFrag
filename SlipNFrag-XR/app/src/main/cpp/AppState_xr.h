#pragma once

#include "AppState.h"

struct AppState_xr : public AppState
{
	pid_t EngineThreadId;
	pid_t RenderThreadId;
	PFN_xrSetAndroidApplicationThreadKHR xrSetAndroidApplicationThreadKHR;
	bool Terminated;
};
