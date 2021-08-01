#pragma once

#include "LoadedSharedMemoryTexture.h"

struct CachedSharedMemoryTextures
{
	SharedMemoryTexture* textures;
	SharedMemoryTexture* oldTextures;
	LoadedSharedMemoryTexture* first;
	LoadedSharedMemoryTexture* current;

	void Setup(LoadedSharedMemoryTexture& loaded);
	void DisposeFront();
	void MoveToFront(SharedMemoryTexture* texture);
	void Delete(AppState& appState);
	void DeleteOld(AppState& appState);
};
