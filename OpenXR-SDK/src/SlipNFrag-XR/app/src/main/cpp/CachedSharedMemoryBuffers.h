#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryIndexBuffer.h"
#include "LoadedSharedMemoryWithOffsetBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"

struct CachedSharedMemoryBuffers
{
	SharedMemoryBuffer* buffers;
	SharedMemoryBuffer* oldBuffers;
	LoadedSharedMemoryBuffer* firstVertices;
	LoadedSharedMemoryBuffer* currentVertices;
	LoadedSharedMemoryIndexBuffer* firstIndices16;
	LoadedSharedMemoryIndexBuffer* currentIndices16;
	LoadedSharedMemoryIndexBuffer* firstIndices32;
	LoadedSharedMemoryIndexBuffer* currentIndices32;
	LoadedSharedMemoryWithOffsetBuffer* firstAliasIndices16;
	LoadedSharedMemoryWithOffsetBuffer* currentAliasIndices16;
	LoadedSharedMemoryWithOffsetBuffer* firstAliasIndices32;
	LoadedSharedMemoryWithOffsetBuffer* currentAliasIndices32;
	LoadedSharedMemoryWithOffsetBuffer* firstSurfaceTexturePositions;
	LoadedSharedMemoryWithOffsetBuffer* currentSurfaceTexturePositions;
	LoadedSharedMemoryWithOffsetBuffer* firstTurbulentTexturePositions;
	LoadedSharedMemoryWithOffsetBuffer* currentTurbulentTexturePositions;
	LoadedSharedMemoryBuffer* firstAliasVertices;
	LoadedSharedMemoryBuffer* currentAliasVertices;
	LoadedSharedMemoryTexCoordsBuffer* firstAliasTexCoords;
	LoadedSharedMemoryTexCoordsBuffer* currentAliasTexCoords;

	void Initialize();
	void SetupVertices(LoadedSharedMemoryBuffer& loaded);
	void SetupIndices16(LoadedSharedMemoryIndexBuffer& loaded);
	void SetupIndices32(LoadedSharedMemoryIndexBuffer& loaded);
	void SetupAliasIndices16(LoadedSharedMemoryWithOffsetBuffer& loaded);
	void SetupAliasIndices32(LoadedSharedMemoryWithOffsetBuffer& loaded);
	void SetupSurfaceTexturePositions(LoadedSharedMemoryWithOffsetBuffer& loaded);
	void SetupTurbulentTexturePositions(LoadedSharedMemoryWithOffsetBuffer& loaded);
	void SetupAliasVertices(LoadedSharedMemoryBuffer& loaded);
	void SetupAliasTexCoords(LoadedSharedMemoryTexCoordsBuffer& loaded);
	void DisposeFront();
	void MoveToFront(SharedMemoryBuffer* buffer);
	void Delete(AppState& appState) const;
	void DeleteOld(AppState& appState);
};
