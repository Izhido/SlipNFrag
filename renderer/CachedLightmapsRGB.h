#pragma once

#include "LoadedLightmapRGB.h"
#include <unordered_map>

struct CachedLightmapsRGB
{
	std::unordered_map<void*, LightmapRGB*> lightmaps;
	std::list<LightmapRGB*> oldLightmaps;
	LoadedLightmapRGB* first;
	LoadedLightmapRGB* current;

	void Setup(LoadedLightmapRGB& lightmap);
	void DisposeFront();
	void Delete(AppState& appState);
	void DeleteOld(AppState& appState);
};
