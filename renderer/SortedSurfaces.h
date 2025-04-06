#pragma once

#include <unordered_map>
#include <list>
#include <vulkan/vulkan.h>
#include "SortedSurfaceTexturesWithLightmaps.h"
#include "SortedSurfaceTexturePairsWithLightmaps.h"
#include "SortedSurfaceTextures.h"
#include "LoadedSurfaceRotatedColoredLights.h"
#include "LoadedSurfaceRotated.h"
#include "LoadedSurfaceRotated2Textures.h"
#include "LoadedSurfaceRotated2TexturesColoredLights.h"
#include "LoadedTurbulentRotated.h"
#include "LoadedSprite.h"

struct AppState;

struct SortedSurfaces
{
    static void Initialize(SortedSurfaceTexturesWithLightmaps& sorted);
    static void Initialize(SortedSurfaceTexturePairsWithLightmaps& sorted);
    static void Initialize(SortedSurfaceTextures& sorted);
    static void Sort(AppState& appState, LoadedSurface& loaded, int index, SortedSurfaceTexturesWithLightmaps& sorted);
    static void Sort(AppState& appState, LoadedSurfaceColoredLights& loaded, int index, SortedSurfaceTexturesWithLightmaps& sorted);
    static void Sort(AppState& appState, LoadedSurface2Textures& loaded, int index, SortedSurfaceTexturePairsWithLightmaps& sorted);
    static void Sort(AppState& appState, LoadedSurface2TexturesColoredLights& loaded, int index, SortedSurfaceTexturePairsWithLightmaps& sorted);
    static void Sort(AppState& appState, LoadedTurbulent& loaded, int index, SortedSurfaceTextures& sorted);
	static void Sort(AppState& appState, LoadedSprite& loaded, int index, SortedSurfaceTextures& sorted);
    static float* CopyVertices(LoadedTurbulent& loaded, float attribute, float* target);
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
