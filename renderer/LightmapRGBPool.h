#pragma once

#include "LightmapRGB.h"

struct LightmapRGBPool
{
	LightmapRGB* toDispose;
	LightmapRGB* reusable;

	LightmapRGB* Acquire();
	void Dispose(LightmapRGB* lightmap);
	void DeleteOld(AppState& appState);
	void Delete(AppState& appState);
};
