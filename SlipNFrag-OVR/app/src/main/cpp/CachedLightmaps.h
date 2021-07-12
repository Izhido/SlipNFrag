#pragma once

#include "LoadedLightmap.h"
#include <unordered_map>

struct CachedLightmaps
{
	std::unordered_map<TwinKey, Lightmap*> lightmaps;
	Lightmap* oldLightmaps;
	std::vector<TwinKey> toDelete;
	LoadedLightmap* first;
	LoadedLightmap* current;

	void Setup(AppState& appState, LoadedLightmap& lightmap);
	void DisposeFront();
	void DeleteOld(AppState& appState);
};
