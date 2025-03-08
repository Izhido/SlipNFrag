#pragma once

#include "Texture.h"
#include <list>

struct CachedTextures
{
	std::list<Texture*> textures;
	std::list<Texture*> oldTextures;

	void Reset(AppState& appState);
	void MoveToFront(Texture* texture);
	void Delete(AppState& appState);
};
