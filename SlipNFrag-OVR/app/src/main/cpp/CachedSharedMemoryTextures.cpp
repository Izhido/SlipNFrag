#include "CachedSharedMemoryTextures.h"
#include "Constants.h"

void CachedSharedMemoryTextures::Setup(AppState& appState, LoadedSharedMemoryTexture& sharedMemoryTexture)
{
	sharedMemoryTexture.next = nullptr;
	if (current == nullptr)
	{
		first = &sharedMemoryTexture;
	}
	else
	{
		current->next = &sharedMemoryTexture;
	}
	current = &sharedMemoryTexture;
}

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

void CachedSharedMemoryTextures::DeleteOld(AppState& appState)
{
	if (oldTextures != nullptr)
	{
		for (auto t = &oldTextures; *t != nullptr; )
		{
			(*t)->unusedCount++;
			if ((*t)->unusedCount >= MAX_UNUSED_COUNT)
			{
				auto next = (*t)->next;
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
}
