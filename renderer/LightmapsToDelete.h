#pragma once

#include "Lightmap.h"

struct LightmapsToDelete
{
	Lightmap* oldLightmaps;

	void Dispose(Lightmap* lightmap);
	void DeleteOld(AppState& appState);
	void Delete(AppState& appState);
};
