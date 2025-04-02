#pragma once

#include "LightmapRGB.h"

struct LightmapsRGBToDelete
{
	LightmapRGB* oldLightmaps;

	void Dispose(LightmapRGB* lightmap);
	void DeleteOld(AppState& appState);
	void Delete(AppState& appState);
};
