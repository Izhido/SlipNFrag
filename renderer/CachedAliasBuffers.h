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
	void ChainToAliasVertices(LoadedBuffer& loaded);
	void ChainToAliasTexCoords(LoadedTexCoordsBuffer& loaded);
	void DisposeFront();
	void MoveToFront(Buffer* buffer);
	void Delete(AppState& appState);
	void DeleteOld(AppState& appState);
};
