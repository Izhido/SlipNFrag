#pragma once

#include "LoadedLightmapRGB.h"
#include <unordered_map>

struct CachedLightmapsRGB
{
	LightmapRGB* oldLightmaps;
	LoadedLightmapRGB* first;
	LoadedLightmapRGB* current;

	void Setup(LoadedLightmapRGB& lightmap);
	void Delete(AppState& appState);
	void DeleteOld(AppState& appState);
};
