#pragma once

#include <unordered_map>
#include <list>
#include <vulkan/vulkan.h>
#include "SortedSurfaceLightmap.h"
#include "SortedSurface2TexturesLightmap.h"
#include "LoadedSurfaceRotatedColoredLights.h"
#include "LoadedSurfaceRotated.h"
#include "LoadedSurfaceRotated2Textures.h"
#include "LoadedSurfaceRotated2TexturesColoredLights.h"
#include "LoadedTurbulentRotated.h"

struct AppState;

struct SortedSurfaces
{
	std::unordered_map<VkDescriptorSet, std::list<SortedSurfaceLightmap>::iterator> addedLightmaps;
	std::unordered_map<VkDescriptorSet, std::list<SortedSurface2TexturesLightmap>::iterator> added2TexturesLightmaps;
	std::unordered_map<VkDescriptorSet, std::list<SortedSurfaceTexture>::iterator> addedTextures;

	void Initialize(std::list<SortedSurfaceLightmap>& sorted);
	void Initialize(std::list<SortedSurface2TexturesLightmap>& sorted);
	void Initialize(std::list<SortedSurfaceTexture>& sorted);
	void Sort(AppState& appState, LoadedSurface& loaded, int index, std::list<SortedSurfaceLightmap>& sorted);
	void Sort(AppState& appState, LoadedSurfaceColoredLights& loaded, int index, std::list<SortedSurfaceLightmap>& sorted);
	void Sort(AppState& appState, LoadedSurface2Textures& loaded, int index, std::list<SortedSurface2TexturesLightmap>& sorted);
	void Sort(AppState& appState, LoadedSurface2TexturesColoredLights& loaded, int index, std::list<SortedSurface2TexturesLightmap>& sorted);
	void Sort(AppState& appState, LoadedTurbulent& loaded, int index, std::list<SortedSurfaceTexture>& sorted);
	static void Cleanup(std::list<SortedSurfaceLightmap>& sorted);
	static void Cleanup(std::list<SortedSurface2TexturesLightmap>& sorted);
	static void Cleanup(std::list<SortedSurfaceTexture>& sorted);
    static float* CopyVertices(LoadedTurbulent& loaded, float attribute, float* target);
	static void LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2Textures>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulentRotated>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset);
	static VkDeviceSize LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadAttributes(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices16(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
	static VkDeviceSize LoadIndices32(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset);
};
