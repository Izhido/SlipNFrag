#pragma once

#include "LoadedSharedMemoryTexture.h"
#include <list>

struct CachedSharedMemoryTextures
{
	int width;
	int height;
	std::list<SharedMemoryTexture*> textures;
	std::list<SharedMemoryTexture*> oldTextures;
	LoadedSharedMemoryTexture* first;
	LoadedSharedMemoryTexture* current;
	int currentIndex;

	void Chain(LoadedSharedMemoryTexture& loaded);
    void Reset();
	void DeleteOld(AppState& appState);
	void DisposeFront();
	void MoveToFront(SharedMemoryTexture* texture);
	void Delete(AppState& appState);
};
