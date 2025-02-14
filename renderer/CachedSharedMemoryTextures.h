#pragma once

#include "LoadedSharedMemoryTexture.h"

struct CachedSharedMemoryTextures
{
	int width;
	int height;
	SharedMemoryTexture* textures;
	SharedMemoryTexture* oldTextures;
	LoadedSharedMemoryTexture* first;
	LoadedSharedMemoryTexture* current;
	int currentIndex;

	void Setup(LoadedSharedMemoryTexture& loaded);
	void DisposeFront();
	void MoveToFront(SharedMemoryTexture* texture);
	void Delete(AppState& appState);
	void DeleteOld(AppState& appState);
};
