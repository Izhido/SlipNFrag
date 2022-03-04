#pragma once

#include <unordered_map>
#include <list>
#include <vulkan/vulkan.h>
#include "SortedSurfaceLightmap.h"
#include "LoadedSurfaceRotated.h"

struct AppState;

struct SortedSurfaces
{
	std::unordered_map<VkDescriptorSet, std::list<SortedSurfaceLightmap>::iterator> addedLightmaps;
	std::unordered_map<VkDescriptorSet, std::list<SortedSurfaceTexture>::iterator> addedTextures;
	std::list<SortedSurfaceLightmap> surfaces;
	std::list<SortedSurfaceLightmap> surfacesRotated;
	std::list<SortedSurfaceLightmap> fences;
	std::list<SortedSurfaceLightmap> fencesRotated;
	std::list<SortedSurfaceTexture> turbulent;

	void Initialize(std::list<SortedSurfaceLightmap>& sorted);
	void Initialize(std::list<SortedSurfaceTexture>& sorted);
	void Sort(LoadedSurface& loaded, int index, std::list<SortedSurfaceLightmap>& sorted);
	void Sort(LoadedTurbulent& loaded, int index, std::list<SortedSurfaceTexture>& sorted);
	static void Cleanup(std::list<SortedSurfaceLightmap>& sorted);
	static void Cleanup(std::list<SortedSurfaceTexture>& sorted);
	static void LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadVertices(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadAttributes(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices16(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices16(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices16(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices32(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices32(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
	static void LoadIndices32(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset);
};
