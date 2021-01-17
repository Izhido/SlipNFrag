#pragma once

#include "SharedMemoryTexture.h"

struct CachedSharedMemoryTextures
{
	SharedMemoryTexture* textures = nullptr;
	SharedMemoryTexture* oldTextures = nullptr;

	void DisposeFront();
	void MoveToFront(SharedMemoryTexture* texture);
	void Delete(AppState& appState);
};
