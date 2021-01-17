#include "CachedTextures.h"
#include "Constants.h"

void CachedTextures::Reset(AppState& appState)
{
	if (oldTextures != nullptr)
	{
		for (Texture** t = &oldTextures; *t != nullptr; )
		{
			(*t)->unusedCount++;
			if ((*t)->unusedCount >= MAX_UNUSED_COUNT)
			{
				Texture* next = (*t)->next;
				(*t)->Delete(appState);
				delete *t;
				*t = next;
			}
			else
			{
				t = &(*t)->next;
			}
		}
	}
	for (Texture* t = textures, *next = nullptr; t != nullptr; t = next)
	{
		next = t->next;
		t->next = oldTextures;
		oldTextures = t;
	}
	textures = nullptr;
}

void CachedTextures::MoveToFront(Texture* texture)
{
	texture->unusedCount = 0;
	texture->next = textures;
	textures = texture;
}

void CachedTextures::Delete(AppState& appState)
{
	for (Texture* t = textures, *next = nullptr; t != nullptr; t = next)
	{
		next = t->next;
		t->Delete(appState);
		delete t;
	}
	for (Texture* t = oldTextures, *next = nullptr; t != nullptr; t = next)
	{
		next = t->next;
		t->Delete(appState);
		delete t;
	}
}
