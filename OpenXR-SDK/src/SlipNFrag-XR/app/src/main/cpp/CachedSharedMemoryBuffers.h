#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexturePositionsBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"

struct CachedSharedMemoryBuffers
{
	SharedMemoryBuffer* buffers;
	SharedMemoryBuffer* oldBuffers;
	LoadedSharedMemoryBuffer* firstVertices;
	LoadedSharedMemoryBuffer* currentVertices;
	LoadedSharedMemoryTexturePositionsBuffer* firstSurfaceTexturePositions;
	LoadedSharedMemoryTexturePositionsBuffer* currentSurfaceTexturePositions;
	LoadedSharedMemoryTexturePositionsBuffer* firstTurbulentTexturePositions;
	LoadedSharedMemoryTexturePositionsBuffer* currentTurbulentTexturePositions;
	LoadedSharedMemoryBuffer* firstAliasVertices;
	LoadedSharedMemoryBuffer* currentAliasVertices;
	LoadedSharedMemoryTexCoordsBuffer* firstAliasTexCoords;
	LoadedSharedMemoryTexCoordsBuffer* currentAliasTexCoords;

	void Initialize();
	void SetupVertices(LoadedSharedMemoryBuffer& loaded);
	void SetupSurfaceTexturePositions(LoadedSharedMemoryTexturePositionsBuffer& loaded);
	void SetupTurbulentTexturePositions(LoadedSharedMemoryTexturePositionsBuffer& loaded);
	void SetupAliasVertices(LoadedSharedMemoryBuffer& loaded);
	void SetupAliasTexCoords(LoadedSharedMemoryTexCoordsBuffer& loaded);
	void DisposeFront();
	void MoveToFront(SharedMemoryBuffer* buffer);
	void Delete(AppState& appState) const;
	void DeleteOld(AppState& appState);
};
