#include "CachedSharedMemoryTextures.h"

void CachedSharedMemoryTextures::DisposeFront()
{
	for (SharedMemoryTexture* t = textures, *next = nullptr; t != nullptr; t = next)
	{
		next = t->next;
		t->next = oldTextures;
		oldTextures = t;
	}
	textures = nullptr;
}

void CachedSharedMemoryTextures::MoveToFront(SharedMemoryTexture* texture)
{
	texture->unusedCount = 0;
	texture->next = textures;
	textures = texture;
}

void CachedSharedMemoryTextures::Delete(AppState& appState)
{
	for (SharedMemoryTexture* t = textures, *next = nullptr; t != nullptr; t = next)
	{
		next = t->next;
		t->Delete(appState);
		delete t;
	}
	for (SharedMemoryTexture* t = oldTextures, *next = nullptr; t != nullptr; t = next)
	{
		next = t->next;
		t->Delete(appState);
		delete t;
	}
}
