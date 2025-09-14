#pragma once

#include "AppState.h"

struct AppState_pcxr : public AppState
{
	bool DestroyRequested;

	void RenderScreen(ScreenPerFrame& perFrame);
};
