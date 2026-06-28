#pragma once

#include <list>
#include <vulkan/vulkan.h>
#include "SortedSurfaceTexturesWithLightmaps.h"
#include "SortedSurfaceTexturePairsWithLightmaps.h"
#include "SortedSurfaceTextures.h"
#include "SortedAliasColormaps.h"
#include "SortedAliasIndices.h"
#include "LoadedSurfaceRotatedColoredLights.h"
#include "LoadedSurfaceRotated.h"
#include "LoadedSurfaceRotated2Textures.h"
#include "LoadedSurfaceRotated2TexturesColoredLights.h"
#include "LoadedTurbulentRotated.h"
#include "LoadedSprite.h"
#include "LoadedAlias.h"
#include "Vertex.h"

struct SortedSurfaces
{
    static void Initialize(SortedSurfaceTexturesWithLightmaps& sorted);
    static void Initialize(SortedSurfaceTexturePairsWithLightmaps& sorted);
    static void Initialize(SortedSurfaceTextures& sorted);
	static void Initialize(SortedAliasColormaps& sorted);
	static void Initialize(SortedAliasIndices& sorted);
    static void Sort(AppState& appState, LoadedSurface& loaded, int index, SortedSurfaceTexturesWithLightmaps& sorted);
    static void Sort(AppState& appState, LoadedSurfaceColoredLights& loaded, int index, SortedSurfaceTexturesWithLightmaps& sorted);
    static void Sort(AppState& appState, LoadedSurface2Textures& loaded, int index, SortedSurfaceTexturePairsWithLightmaps& sorted);
    static void Sort(AppState& appState, LoadedSurface2TexturesColoredLights& loaded, int index, SortedSurfaceTexturePairsWithLightmaps& sorted);
    static void Sort(AppState& appState, LoadedTurbulent& loaded, int index, SortedSurfaceTextures& sorted);
	static void Sort(AppState& appState, LoadedSprite& loaded, int index, SortedSurfaceTextures& sorted);
	static void Sort(AppState& appState, LoadedAlias& loaded, int index, SortedAliasColormaps& sorted);
	static void Sort(AppState& appState, LoadedAliasColoredLights& loaded, int index, SortedAliasIndices& sorted);
    static Vertex* CopyVertices(LoadedTurbulent& loaded, uint32_t attribute, Vertex* target);
	static void LoadVertices(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurface>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2Textures>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotated>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceTextures& sorted, std::vector<LoadedSprite>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedAliasColormaps& sorted, std::vector<LoadedAlias>& loaded, Buffer* stagingBuffer, VkDeviceSize lightBase, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedAliasIndices& sorted, std::vector<LoadedAliasColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize lightBase, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceTextures& sorted, std::vector<LoadedSprite>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceTextures& sorted, std::vector<LoadedSprite>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
};
