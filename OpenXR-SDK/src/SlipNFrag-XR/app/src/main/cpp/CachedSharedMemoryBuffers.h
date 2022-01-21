#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryIndexBuffer.h"
#include "LoadedSharedMemoryTexturePositionsBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"

struct CachedSharedMemoryBuffers
{
	SharedMemoryBuffer* buffers;
	SharedMemoryBuffer* oldBuffers;
	LoadedSharedMemoryBuffer* firstVertices;
	LoadedSharedMemoryBuffer* currentVertices;
	LoadedSharedMemoryIndexBuffer* firstIndices8;
	LoadedSharedMemoryIndexBuffer* currentIndices8;
	LoadedSharedMemoryIndexBuffer* firstIndices16;
	LoadedSharedMemoryIndexBuffer* currentIndices16;
	LoadedSharedMemoryIndexBuffer* firstIndices32;
	LoadedSharedMemoryIndexBuffer* currentIndices32;
	LoadedSharedMemoryTexturePositionsBuffer* firstSurfaceTexturePositions;
	LoadedSharedMemoryTexturePositionsBuffer* currentSurfaceTexturePositions;
	LoadedSharedMemoryTexturePositionsBuffer* firstTurbulentTexturePositions;
	LoadedSharedMemoryTexturePositionsBuffer* currentTurbulentTexturePositions;
	LoadedSharedMemoryBuffer* firstAliasVertices;
	LoadedSharedMemoryBuffer* currentAliasVertices;
	LoadedSharedMemoryTexCoordsBuffer* firstAliasTexCoords;
	LoadedSharedMemoryTexCoordsBuffer* currentAliasTexCoords;
	LoadedSharedMemoryIndexBuffer* firstAliasIndices8;
	LoadedSharedMemoryIndexBuffer* currentAliasIndices8;
	LoadedSharedMemoryIndexBuffer* firstAliasIndices16;
	LoadedSharedMemoryIndexBuffer* currentAliasIndices16;
	LoadedSharedMemoryIndexBuffer* firstAliasIndices32;
	LoadedSharedMemoryIndexBuffer* currentAliasIndices32;

	void Initialize();
	void SetupVertices(LoadedSharedMemoryBuffer& loaded);
	void SetupIndices8(LoadedSharedMemoryIndexBuffer& loaded);
	void SetupIndices16(LoadedSharedMemoryIndexBuffer& loaded);
	void SetupIndices32(LoadedSharedMemoryIndexBuffer& loaded);
	void SetupSurfaceTexturePositions(LoadedSharedMemoryTexturePositionsBuffer& loaded);
	void SetupTurbulentTexturePositions(LoadedSharedMemoryTexturePositionsBuffer& loaded);
	void SetupAliasVertices(LoadedSharedMemoryBuffer& loaded);
	void SetupAliasTexCoords(LoadedSharedMemoryTexCoordsBuffer& loaded);
	void SetupAliasIndices8(LoadedSharedMemoryIndexBuffer& loaded);
	void SetupAliasIndices16(LoadedSharedMemoryIndexBuffer& loaded);
	void SetupAliasIndices32(LoadedSharedMemoryIndexBuffer& loaded);
	void DisposeFront();
	void MoveToFront(SharedMemoryBuffer* buffer);
	void Delete(AppState& appState) const;
	void DeleteOld(AppState& appState);
};
