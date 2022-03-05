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
#include "LoadedAlias.h"
#include "CachedLightmaps.h"
#include "CachedSharedMemoryTextures.h"
#include "Skybox.h"
#include <common/xr_linear.h>
#include "SortedSurfaceLightmap.h"
#include "UsedInSharedMemory.h"
#include "PerFrame.h"
#include "SortedSurfaces.h"

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
	Pipeline turbulent;
	Pipeline turbulentLit;
	Pipeline sprites;
	Pipeline alias;
	Pipeline viewmodel;
	Pipeline particle;
	Pipeline colored;
	Pipeline sky;
	Pipeline floor;
	int hostClearCount;
	CachedSharedMemoryBuffers buffers;
	CachedIndexBuffers indexBuffers;
	std::unordered_map<void*, SharedMemoryBuffer*> vertexCache;
	std::unordered_map<void*, PerSurface> perSurfaceCache;
	std::unordered_map<void*, AliasVertices> aliasVerticesCache;
	std::unordered_map<void*, IndexBuffer> aliasIndicesCache;
	int lastSurface;
	int lastSurfaceRotated;
	int lastFence;
	int lastFenceRotated;
	int lastTurbulent;
	int lastTurbulentLit;
	int lastSprite;
	int lastAlias;
	int lastViewmodel;
	int lastParticle;
	int lastColoredIndex8;
	int lastColoredIndex16;
	int lastColoredIndex32;
	int lastSky;
	std::vector<LoadedSurface> loadedSurfaces;
	std::vector<LoadedSurfaceRotated> loadedSurfacesRotated;
	std::vector<LoadedSurface> loadedFences;
	std::vector<LoadedSurfaceRotated> loadedFencesRotated;
	std::vector<LoadedTurbulent> loadedTurbulent;
	std::vector<LoadedSurface> loadedTurbulentLit;
	std::vector<LoadedSprite> loadedSprites;
	std::vector<LoadedAlias> loadedAlias;
	std::vector<LoadedAlias> loadedViewmodels;
	int skyCount;
	int firstSkyVertex;
	std::unordered_map<VkDeviceSize, std::list<LightmapTexture>> lightmapTextures;
	CachedLightmaps lightmaps;
	CachedSharedMemoryTextures textures;
	std::unordered_map<void*, SharedMemoryTexture*> textureCache;
	std::unordered_map<void*, SharedMemoryTexture*> spriteCache;
	std::unordered_map<void*, SharedMemoryTexture*> aliasTexturesCache;
	Texture floorTexture;
	Texture controllerTexture;
	std::vector<VkSampler> samplers;
	VkSampler lightmapSampler;
	std::unordered_map<uint32_t, std::list<UsedInSharedMemory>> latestMemory;
	SharedMemoryBuffer* latestSharedMemoryTexturePositionBuffer;
	VkDeviceSize usedInLatestSharedMemoryTexturePositionBuffer;
	SharedMemoryBuffer* latestIndexBuffer8;
	VkDeviceSize usedInLatestIndexBuffer8;
	SharedMemoryBuffer* latestIndexBuffer16;
	VkDeviceSize usedInLatestIndexBuffer16;
	SharedMemoryBuffer* latestIndexBuffer32;
	VkDeviceSize usedInLatestIndexBuffer32;
	DescriptorSets* latestTextureDescriptorSets;
	Skybox* previousSkyboxes;
	Skybox* skybox;
	VkDeviceSize floorVerticesSize;
	VkDeviceSize controllerVerticesSize;
	VkDeviceSize texturedVerticesSize;
	VkDeviceSize particlePositionsSize;
	VkDeviceSize coloredVerticesSize;
	VkDeviceSize verticesSize;
	VkDeviceSize floorAttributesSize;
	VkDeviceSize controllerAttributesSize;
	VkDeviceSize texturedAttributesSize;
	VkDeviceSize colormappedLightsSize;
	VkDeviceSize attributesSize;
	VkDeviceSize particleColorsSize;
	VkDeviceSize coloredColorsSize;
	VkDeviceSize colorsSize;
	VkDeviceSize floorIndicesSize;
	VkDeviceSize controllerIndicesSize;
	VkDeviceSize coloredIndices8Size;
	VkDeviceSize coloredIndices16Size;
	VkDeviceSize indices8Size;
	VkDeviceSize indices16Size;
	VkDeviceSize indices32Size;
	VkDeviceSize sortedVerticesSize;
	VkDeviceSize sortedAttributesSize;
	VkDeviceSize sortedIndicesCount;
	VkDeviceSize sortedIndices16Size;
	VkDeviceSize sortedIndices32Size;
	VkDeviceSize sortedSurfaceRotatedVerticesBase;
	VkDeviceSize sortedSurfaceRotatedAttributesBase;
	VkDeviceSize sortedSurfaceRotatedIndicesBase;
	VkDeviceSize sortedFenceVerticesBase;
	VkDeviceSize sortedFenceAttributesBase;
	VkDeviceSize sortedFenceIndicesBase;
	VkDeviceSize sortedFenceRotatedVerticesBase;
	VkDeviceSize sortedFenceRotatedAttributesBase;
	VkDeviceSize sortedFenceRotatedIndicesBase;
	VkDeviceSize sortedTurbulentVerticesBase;
	VkDeviceSize sortedTurbulentAttributesBase;
	VkDeviceSize sortedTurbulentIndicesBase;
	VkDeviceSize sortedTurbulentLitVerticesBase;
	VkDeviceSize sortedTurbulentLitAttributesBase;
	VkDeviceSize sortedTurbulentLitIndicesBase;
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
	SortedSurfaces sorted;

	static void CopyImage(AppState& appState, unsigned char* source, uint32_t* target, int width, int height);
	static void AddBorder(AppState& appState, std::vector<uint32_t>& target);
	void Create(AppState& appState, VkCommandBufferAllocateInfo& commandBufferAllocateInfo, VkCommandBuffer& setupCommandBuffer, VkCommandBufferBeginInfo& commandBufferBeginInfo, VkSubmitInfo& setupSubmitInfo, struct android_app* app);
	static void CreateShader(AppState& appState, struct android_app* app, const char* filename, VkShaderModule* shaderModule);
	void Initialize();
	void AddToBufferBarrier(VkBuffer buffer);
	void GetVerticesStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size);
	void GetTextureStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dspritedata_t& sprite, LoadedSprite& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dalias_t& alias, LoadedAlias& loaded, Texture* host_colormap, VkDeviceSize& size);
	VkDeviceSize GetStagingBufferSize(AppState& appState, PerFrame& perFrame);
	void Reset();
};
