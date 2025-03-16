#pragma once

#include "LoadedLightmap.h"
#include <unordered_map>

struct CachedLightmaps
{
	std::list<Lightmap*> oldLightmaps;
	LoadedLightmap* first;
	LoadedLightmap* current;

	void Setup(LoadedLightmap& lightmap);
	void Delete(AppState& appState);
	void DeleteOld(AppState& appState);
};
