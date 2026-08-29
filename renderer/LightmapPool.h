#pragma once

#include "Lightmap.h"

struct LightmapPool
{
	Lightmap* toDispose;
	Lightmap* reusable;

	Lightmap* Acquire();
	void Dispose(Lightmap* lightmap);
	void DeleteOld(AppState& appState);
	void Delete(AppState& appState);
};
