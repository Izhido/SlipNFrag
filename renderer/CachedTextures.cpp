#include "CachedTextures.h"
#include "Constants.h"

void CachedTextures::Reset(AppState& appState)
{
	for (auto t = oldTextures.begin(); t != oldTextures.end(); )
	{
		(*t)->unusedCount++;
		if ((*t)->unusedCount >= Constants::maxUnusedCount)
		{
			auto texture = *t;
			texture->Delete(appState);
			delete texture;
			t = oldTextures.erase(t);
		}
		else
		{
			t++;
		}
	}
	oldTextures.splice(oldTextures.begin(), textures);
}

void CachedTextures::MoveToFront(Texture* texture)
{
	texture->unusedCount = 0;
	textures.push_back(texture);
}

void CachedTextures::Delete(AppState& appState)
{
	for (auto b : oldTextures)
	{
		b->Delete(appState);
	}
	oldTextures.clear();
	for (auto b : textures)
	{
		b->Delete(appState);
	}
	textures.clear();
}
