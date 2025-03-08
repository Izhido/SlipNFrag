#include "CachedSharedMemoryTextures.h"
#include "Constants.h"

void CachedSharedMemoryTextures::Setup(LoadedSharedMemoryTexture& loaded)
{
	loaded.next = nullptr;
	if (current == nullptr)
	{
		first = &loaded;
	}
	else
	{
		current->next = &loaded;
	}
	current = &loaded;
}

void CachedSharedMemoryTextures::DeleteOld(AppState& appState)
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
}

void CachedSharedMemoryTextures::DisposeFront()
{
	oldTextures.splice(oldTextures.begin(), textures);
}

void CachedSharedMemoryTextures::MoveToFront(SharedMemoryTexture* texture)
{
	texture->unusedCount = 0;
	textures.push_back(texture);
}

void CachedSharedMemoryTextures::Delete(AppState& appState)
{
	for (auto t : oldTextures)
	{
		t->Delete(appState);
	}
	oldTextures.clear();
	for (auto t : textures)
	{
		t->Delete(appState);
	}
	textures.clear();
}
