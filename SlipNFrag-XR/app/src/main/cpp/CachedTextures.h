#pragma once

#include "Texture.h"

struct CachedTextures
{
	Texture* textures = nullptr;
	Texture* oldTextures = nullptr;

	void Reset(AppState& appState);
	void MoveToFront(Texture* texture);
	void Delete(AppState& appState) const;
};
