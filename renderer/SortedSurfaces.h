#pragma once

#include <unordered_map>
#include <list>
#include <vulkan/vulkan.h>
#include "SortedSurfaceLightmapsWithTextures.h"
#include "SortedSurfaceLightmapsWith2Textures.h"
#include "SortedSurfaceTextures.h"
#include "LoadedSurfaceRotatedColoredLights.h"
#include "LoadedSurfaceRotated.h"
#include "LoadedSurfaceRotated2Textures.h"
#include "LoadedSurfaceRotated2TexturesColoredLights.h"
#include "LoadedTurbulentRotated.h"

struct AppState;

struct SortedSurfaces
{
    static void Initialize(SortedSurfaceLightmapsWithTextures& sorted);
    static void Initialize(SortedSurfaceLightmapsWith2Textures& sorted);
    static void Initialize(SortedSurfaceTextures& sorted);
    static void Sort(AppState& appState, LoadedSurface& loaded, int index, SortedSurfaceLightmapsWithTextures& sorted);
    static void Sort(AppState& appState, LoadedSurfaceColoredLights& loaded, int index, SortedSurfaceLightmapsWithTextures& sorted);
    static void Sort(AppState& appState, LoadedSurface2Textures& loaded, int index, SortedSurfaceLightmapsWith2Textures& sorted);
    static void Sort(AppState& appState, LoadedSurface2TexturesColoredLights& loaded, int index, SortedSurfaceLightmapsWith2Textures& sorted);
    static void Sort(AppState& appState, LoadedTurbulent& loaded, int index, SortedSurfaceTextures& sorted);
    static float* CopyVertices(LoadedTurbulent& loaded, float attribute, float* target);
	static void LoadVertices(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurface>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2Textures>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotated>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
};
