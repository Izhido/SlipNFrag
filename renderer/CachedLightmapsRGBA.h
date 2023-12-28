#pragma once

#include "LoadedLightmapRGBA.h"
#include <unordered_map>

struct CachedLightmapsRGBA
{
	std::unordered_map<void*, LightmapRGBA*> lightmaps;
	LightmapRGBA* oldLightmaps;
	LoadedLightmapRGBA* first;
	LoadedLightmapRGBA* current;

	void Setup(LoadedLightmapRGBA& lightmap);
	void DisposeFront();
	void Delete(AppState& appState);
	void DeleteOld(AppState& appState);
};
