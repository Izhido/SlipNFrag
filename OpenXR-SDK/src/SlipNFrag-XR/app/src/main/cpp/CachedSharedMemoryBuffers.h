#pragma once

#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryIndexBuffer.h"
#include "LoadedSharedMemoryAliasIndexBuffer.h"
#include "LoadedSharedMemoryTexturePositionBuffer.h"
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
	LoadedSharedMemoryAliasIndexBuffer* firstAliasIndices16;
	LoadedSharedMemoryAliasIndexBuffer* currentAliasIndices16;
	LoadedSharedMemoryAliasIndexBuffer* firstAliasIndices32;
	LoadedSharedMemoryAliasIndexBuffer* currentAliasIndices32;
	LoadedSharedMemoryTexturePositionBuffer* firstSurfaceTexturePosition;
	LoadedSharedMemoryTexturePositionBuffer* currentSurfaceTexturePosition;
	LoadedSharedMemoryTexturePositionBuffer* firstTurbulentTexturePosition;
	LoadedSharedMemoryTexturePositionBuffer* currentTurbulentTexturePosition;
	LoadedSharedMemoryBuffer* firstAliasVertices;
	LoadedSharedMemoryBuffer* currentAliasVertices;
	LoadedSharedMemoryTexCoordsBuffer* firstAliasTexCoords;
	LoadedSharedMemoryTexCoordsBuffer* currentAliasTexCoords;

	void Initialize();
	void SetupVertices(LoadedSharedMemoryBuffer& loaded);
	void SetupIndices16(LoadedSharedMemoryIndexBuffer& loaded);
	void SetupIndices32(LoadedSharedMemoryIndexBuffer& loaded);
	void SetupAliasIndices16(LoadedSharedMemoryAliasIndexBuffer& loaded);
	void SetupAliasIndices32(LoadedSharedMemoryAliasIndexBuffer& loaded);
	void SetupSurfaceTexturePosition(LoadedSharedMemoryTexturePositionBuffer& loaded);
	void SetupTurbulentTexturePosition(LoadedSharedMemoryTexturePositionBuffer& loaded);
	void SetupAliasVertices(LoadedSharedMemoryBuffer& loaded);
	void SetupAliasTexCoords(LoadedSharedMemoryTexCoordsBuffer& loaded);
	void DisposeFront();
	void MoveToFront(SharedMemoryBuffer* buffer);
	void Delete(AppState& appState) const;
	void DeleteOld(AppState& appState);
};
