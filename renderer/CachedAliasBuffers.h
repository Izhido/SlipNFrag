#pragma once

#include "LoadedBuffer.h"
#include "LoadedTexCoordsBuffer.h"

struct CachedAliasBuffers
{
	Buffer* buffers;
	Buffer* oldBuffers;
	LoadedBuffer* firstAliasVertices;
	LoadedBuffer* currentAliasVertices;
	LoadedTexCoordsBuffer* firstAliasTexCoords;
	LoadedTexCoordsBuffer* currentAliasTexCoords;

	void Initialize();
	void SetupAliasVertices(LoadedBuffer& loaded);
	void SetupAliasTexCoords(LoadedTexCoordsBuffer& loaded);
	void DisposeFront();
	void MoveToFront(Buffer* buffer);
	void Delete(AppState& appState);
	void DeleteOld(AppState& appState);
};
