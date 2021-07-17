#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"

struct CachedSharedMemoryBuffers
{
	SharedMemoryBuffer* buffers;
	SharedMemoryBuffer* oldBuffers;
	LoadedSharedMemoryBuffer* firstVertices;
	LoadedSharedMemoryBuffer* currentVertices;
	LoadedSharedMemoryBuffer* firstTexturePositions;
	LoadedSharedMemoryBuffer* currentTexturePositions;
	LoadedSharedMemoryBuffer* firstAliasVertices;
	LoadedSharedMemoryBuffer* currentAliasVertices;
	LoadedSharedMemoryTexCoordsBuffer* firstAliasTexCoords;
	LoadedSharedMemoryTexCoordsBuffer* currentAliasTexCoords;

	void SetupVertices(AppState& appState, LoadedSharedMemoryBuffer& loaded);
	void SetupTexturePositions(AppState& appState, LoadedSharedMemoryBuffer& loaded);
	void SetupAliasVertices(AppState& appState, LoadedSharedMemoryBuffer& loaded);
	void SetupAliasTexCoords(AppState& appState, LoadedSharedMemoryTexCoordsBuffer& loaded);
	void DisposeFront();
	void MoveToFront(SharedMemoryBuffer* buffer);
	void Delete(AppState& appState);
	void DeleteOld(AppState& appState);
};
