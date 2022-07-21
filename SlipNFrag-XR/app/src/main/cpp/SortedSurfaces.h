#pragma once

#include <unordered_map>
#include <list>
#include <vulkan/vulkan.h>
#include "SortedSurfaceLightmap.h"
#include "SortedSurface2TexturesLightmap.h"
#include "LoadedSurface2Textures.h"
#include "LoadedSurfaceRotated.h"
#include "LoadedSurfaceRotated2Textures.h"

struct AppState;

struct SortedSurfaces
{
	std::unordered_map<VkDescriptorSet, std::list<SortedSurfaceLightmap>::iterator> addedLightmaps;
	std::unordered_map<VkDescriptorSet, std::list<SortedSurface2TexturesLightmap>::iterator> added2TexturesLightmaps;
	std::unordered_map<VkDescriptorSet, std::list<SortedSurfaceTexture>::iterator> addedTextures;
	std::list<SortedSurfaceLightmap> surfaces;
	std::list<SortedSurface2TexturesLightmap> surfacesRGBA;
	std::list<SortedSurfaceLightmap> surfacesRGBANoGlow;
	std::list<SortedSurfaceLightmap> surfacesRotated;
	std::list<SortedSurface2TexturesLightmap> surfacesRotatedRGBA;
	std::list<SortedSurfaceLightmap> surfacesRotatedRGBANoGlow;
	std::list<SortedSurfaceLightmap> fences;
	std::list<SortedSurface2TexturesLightmap> fencesRGBA;
	std::list<SortedSurfaceLightmap> fencesRGBANoGlow;
	std::list<SortedSurfaceLightmap> fencesRotated;
	std::list<SortedSurface2TexturesLightmap> fencesRotatedRGBA;
	std::list<SortedSurfaceLightmap> fencesRotatedRGBANoGlow;
	std::list<SortedSurfaceTexture> turbulent;
	std::list<SortedSurfaceTexture> turbulentRGBA;
	std::list<SortedSurfaceLightmap> turbulentLit;
	std::list<SortedSurfaceLightmap> turbulentLitRGBA;

	void Initialize(std::list<SortedSurfaceLightmap>& sorted);
	void Initialize(std::list<SortedSurface2TexturesLightmap>& sorted);
	void Initialize(std::list<SortedSurfaceTexture>& sorted);
	static void CacheVertices(AppState& appState, LoadedTurbulent& loaded);
	void Sort(AppState& appState, LoadedSurface& loaded, int index, std::list<SortedSurfaceLightmap>& sorted);
	void Sort(AppState& appState, LoadedSurface2Textures& loaded, int index, std::list<SortedSurface2TexturesLightmap>& sorted);
	void Sort(AppState& appState, LoadedTurbulent& loaded, int index, std::list<SortedSurfaceTexture>& sorted);
	static void Cleanup(std::list<SortedSurfaceLightmap>& sorted);
	static void Cleanup(std::list<SortedSurface2TexturesLightmap>& sorted);
	static void Cleanup(std::list<SortedSurfaceTexture>& sorted);
	static void LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadAttributes(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadAttributes(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadAttributes(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices16(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices16(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices16(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices16(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices16(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices32(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices32(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices32(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices32(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices32(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
};
