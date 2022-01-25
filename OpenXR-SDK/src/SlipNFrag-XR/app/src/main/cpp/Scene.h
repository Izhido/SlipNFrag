#pragma once

#include <vulkan/vulkan.h>
#include "Pipeline.h"
#include "CachedSharedMemoryBuffers.h"
#include "CachedIndexBuffers.h"
#include <unordered_map>
#include "AliasVertices.h"
#include "PerSurface.h"
#include "LoadedSurfaceRotated.h"
#include "LoadedSprite.h"
#include "LoadedTurbulentLit.h"
#include "LoadedTurbulentRotatedLit.h"
#include "LoadedAlias.h"
#include "CachedLightmaps.h"
#include "CachedSharedMemoryTextures.h"
#include "Skybox.h"
#include <common/xr_linear.h>
#include "SortedSurfaceTexture.h"

struct AppState;

struct Scene
{
	bool created;
	VkDescriptorSetLayout singleBufferLayout;
	VkDescriptorSetLayout doubleBufferLayout;
	VkDescriptorSetLayout twoBuffersAndImageLayout;
	VkDescriptorSetLayout singleImageLayout;
	Pipeline surfaces;
	Pipeline surfacesRotated;
	Pipeline fences;
	Pipeline fencesRotated;
	Pipeline sprites;
	Pipeline turbulent;
	Pipeline turbulentLit;
	Pipeline turbulentRotated;
	Pipeline turbulentRotatedLit;
	Pipeline alias;
	Pipeline viewmodel;
	Pipeline colored;
	Pipeline sky;
	Pipeline floor;
	int hostClearCount;
	CachedSharedMemoryBuffers buffers;
	CachedIndexBuffers indexBuffers;
	std::unordered_map<void*, SharedMemoryBuffer*> verticesPerKey;
	std::unordered_map<void*, PerSurface> perSurface;
	std::unordered_map<void*, AliasVertices> aliasVerticesPerKey;
	std::unordered_map<void*, IndexBuffer> aliasIndicesPerKey;
	int lastSurface;
	int lastSurfaceRotated;
	int lastFence;
	int lastFenceRotated;
	int lastSprite;
	int lastTurbulent;
	int lastTurbulentLit;
	int lastTurbulentRotated;
	int lastTurbulentRotatedLit;
	int lastAlias;
	int lastViewmodel;
	int lastColoredIndex8;
	int lastColoredIndex16;
	int lastColoredIndex32;
	int lastSky;
	std::vector<LoadedSurface> loadedSurfaces;
	std::vector<LoadedSurfaceRotated> loadedSurfacesRotated;
	std::vector<LoadedSurface> loadedFences;
	std::vector<LoadedSurfaceRotated> loadedFencesRotated;
	std::vector<LoadedSprite> loadedSprites;
	std::vector<LoadedTurbulent> loadedTurbulent;
	std::vector<LoadedTurbulentLit> loadedTurbulentLit;
	std::vector<LoadedTurbulentRotated> loadedTurbulentRotated;
	std::vector<LoadedTurbulentRotatedLit> loadedTurbulentRotatedLit;
	std::vector<LoadedAlias> loadedAlias;
	std::vector<LoadedAlias> loadedViewmodels;
	int skyCount;
	int firstSkyVertex;
	std::unordered_map<VkDeviceSize, std::list<LightmapTexture>> lightmapTextures;
	CachedLightmaps lightmaps;
	CachedSharedMemoryTextures textures;
	std::unordered_map<void*, SharedMemoryTexture*> surfaceTexturesPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> spritesPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> turbulentPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> aliasTexturesPerKey;
	Texture floorTexture;
	Texture controllerTexture;
	std::vector<VkSampler> samplers;
	VkSampler lightmapSampler;
	SharedMemory* latestBufferSharedMemory;
	VkDeviceSize usedInLatestBufferSharedMemory;
	SharedMemoryBuffer* latestSharedMemoryTexturePositionBuffer;
	VkDeviceSize usedInLatestSharedMemoryTexturePositionBuffer;
	Buffer* latestIndexBuffer8;
	VkDeviceSize usedInLatestIndexBuffer8;
	Buffer* latestIndexBuffer16;
	VkDeviceSize usedInLatestIndexBuffer16;
	Buffer* latestIndexBuffer32;
	VkDeviceSize usedInLatestIndexBuffer32;
	SharedMemory* latestTextureSharedMemory;
	VkDeviceSize usedInLatestTextureSharedMemory;
	DescriptorSets* latestTextureDescriptorSets;
	Skybox* previousSkyboxes;
	Skybox* skybox;
	VkDeviceSize floorVerticesSize;
	VkDeviceSize texturedVerticesSize;
	VkDeviceSize coloredVerticesSize;
	VkDeviceSize controllerVerticesSize;
	VkDeviceSize verticesSize;
	VkDeviceSize floorAttributesSize;
	VkDeviceSize texturedAttributesSize;
	VkDeviceSize colormappedLightsSize;
	VkDeviceSize controllerAttributesSize;
	VkDeviceSize attributesSize;
	VkDeviceSize floorIndicesSize;
	VkDeviceSize controllerIndicesSize;
	VkDeviceSize coloredIndices8Size;
	VkDeviceSize coloredIndices16Size;
	VkDeviceSize indices8Size;
	VkDeviceSize indices16Size;
	VkDeviceSize indices32Size;
	VkDeviceSize colorsSize;
	VkDeviceSize paletteSize;
	VkDeviceSize host_colormapSize;
	VkDeviceSize skySize;
	StagingBuffer stagingBuffer;
	void* previousVertexes;
	void* previousTexture;
	void* previousApverts;
	SharedMemoryBuffer* previousVertexBuffer;
	SharedMemoryBuffer* previousTexCoordsBuffer;
	SharedMemoryTexture* previousSharedMemoryTexture;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	std::list<SortedSurfaceTexture> sortedSurfaces;

	static void CopyImage(AppState& appState, unsigned char* source, uint32_t* target, int width, int height);
	static void AddBorder(AppState& appState, std::vector<uint32_t>& target);
	void Create(AppState& appState, VkCommandBufferAllocateInfo& commandBufferAllocateInfo, VkCommandBuffer& setupCommandBuffer, VkCommandBufferBeginInfo& commandBufferBeginInfo, VkSubmitInfo& setupSubmitInfo, struct android_app* app);
	static void CreateShader(AppState& appState, struct android_app* app, const char* filename, VkShaderModule* shaderModule);
	void Initialize();
	void GetSurfaceStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	void GetSurfaceRotatedStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	void GetTurbulentStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size);
	void GetTurbulentLitStagingBufferSize(AppState& appState, const dturbulentlit_t& turbulent, LoadedTurbulentLit& loaded, VkDeviceSize& size);
	void GetTurbulentRotatedStagingBufferSize(AppState& appState, const dturbulentrotated_t& turbulent, LoadedTurbulentRotated& loaded, VkDeviceSize& size);
	void GetTurbulentRotatedLitStagingBufferSize(AppState& appState, const dturbulentrotatedlit_t& turbulent, LoadedTurbulentRotatedLit& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dalias_t& alias, LoadedAlias& loaded, Texture* host_colormap, VkDeviceSize& size);
	void AddToBufferBarrier(VkBuffer buffer);
	void Reset();
};
