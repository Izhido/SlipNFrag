#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"

struct CachedSharedMemoryBuffers
{
	SharedMemoryBuffer* buffers;
	SharedMemoryBuffer* oldBuffers;
	LoadedSharedMemoryBuffer* firstVertices;
	LoadedSharedMemoryBuffer* currentVertices;
	LoadedSharedMemoryBuffer* firstSurfaceTexturePosition;
	LoadedSharedMemoryBuffer* currentSurfaceTexturePosition;
	LoadedSharedMemoryBuffer* firstTurbulentTexturePosition;
	LoadedSharedMemoryBuffer* currentTurbulentTexturePosition;
	LoadedSharedMemoryBuffer* firstAliasVertices;
	LoadedSharedMemoryBuffer* currentAliasVertices;
	LoadedSharedMemoryTexCoordsBuffer* firstAliasTexCoords;
	LoadedSharedMemoryTexCoordsBuffer* currentAliasTexCoords;

	void SetupVertices(AppState& appState, LoadedSharedMemoryBuffer& loaded);
	void SetupSurfaceTexturePosition(AppState& appState, LoadedSharedMemoryBuffer& loaded);
	void SetupTurbulentTexturePosition(AppState& appState, LoadedSharedMemoryBuffer& loaded);
	void SetupAliasVertices(AppState& appState, LoadedSharedMemoryBuffer& loaded);
	void SetupAliasTexCoords(AppState& appState, LoadedSharedMemoryTexCoordsBuffer& loaded);
	void DisposeFront();
	void MoveToFront(SharedMemoryBuffer* buffer);
	void Delete(AppState& appState);
	void DeleteOld(AppState& appState);
};
