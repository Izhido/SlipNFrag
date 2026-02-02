#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"

struct CachedAliasBuffers
{
	Buffer* buffers;
	Buffer* oldBuffers;
	LoadedSharedMemoryBuffer* firstAliasVertices;
	LoadedSharedMemoryBuffer* currentAliasVertices;
	LoadedSharedMemoryTexCoordsBuffer* firstAliasTexCoords;
	LoadedSharedMemoryTexCoordsBuffer* currentAliasTexCoords;

	void Initialize();
	void SetupAliasVertices(LoadedSharedMemoryBuffer& loaded);
	void SetupAliasTexCoords(LoadedSharedMemoryTexCoordsBuffer& loaded);
	void DisposeFront();
	void MoveToFront(Buffer* buffer);
	void Delete(AppState& appState);
	void DeleteOld(AppState& appState);
};
