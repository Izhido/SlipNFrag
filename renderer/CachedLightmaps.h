#pragma once

#include "LoadedLightmap.h"
#include <unordered_map>

struct CachedLightmaps
{
	std::unordered_map<void*, Lightmap*> lightmaps;
	std::list<Lightmap*> oldLightmaps;
	LoadedLightmap* first;
	LoadedLightmap* current;

	void Setup(LoadedLightmap& lightmap);
	void DisposeFront();
	void Delete(AppState& appState);
	void DeleteOld(AppState& appState);
};
