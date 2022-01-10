#pragma once

#include <vulkan/vulkan.h>
#include "Pipeline.h"
#include "Buffer.h"
#include "CachedSharedMemoryBuffers.h"
#include <unordered_map>
#include "AliasVertices.h"
#include "SharedMemoryIndexBuffer.h"
#include "SharedMemoryBufferWithOffset.h"
#include "LoadedSurface.h"
#include "LoadedSurfaceRotated.h"
#include "LoadedSprite.h"
#include "LoadedTurbulent.h"
#include "LoadedTurbulentRotated.h"
#include "LoadedAlias.h"
#include "CachedLightmaps.h"
#include "CachedSharedMemoryTextures.h"
#include "Skybox.h"
#include <common/xr_linear.h>

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
	Pipeline turbulentRotated;
	Pipeline alias;
	Pipeline viewmodel;
	Pipeline colored;
	Pipeline sky;
	Pipeline floor;
	int hostClearCount;
	CachedSharedMemoryBuffers buffers;
	std::unordered_map<void*, SharedMemoryBuffer*> verticesPerKey;
	std::unordered_map<void*, SharedMemoryBufferWithOffset> texturePositionsPerKey;
	std::unordered_map<void*, AliasVertices> aliasVerticesPerKey;
	std::unordered_map<void*, SharedMemoryIndexBuffer> surfaceIndicesPerKey;
	std::unordered_map<void*, SharedMemoryBufferWithOffset> indicesPerKey;
	std::unordered_map<void*, SharedMemoryBufferWithOffset> aliasIndicesPerKey;
	int lastSurface;
	int lastSurfaceRotated16;
	int lastSurfaceRotated32;
	int lastFence16;
	int lastFence32;
	int lastFenceRotated16;
	int lastFenceRotated32;
	int lastSprite;
	int lastTurbulent16;
	int lastTurbulent32;
	int lastTurbulentRotated16;
	int lastTurbulentRotated32;
	int lastAlias16;
	int lastAlias32;
	int lastViewmodel16;
	int lastViewmodel32;
	int lastColoredIndex16;
	int lastColoredIndex32;
	int lastSky;
	std::vector<LoadedSurface> loadedSurfaces;
	std::vector<LoadedSurfaceRotated> loadedSurfacesRotated16;
	std::vector<LoadedSurfaceRotated> loadedSurfacesRotated32;
	std::vector<LoadedSurface> loadedFences16;
	std::vector<LoadedSurface> loadedFences32;
	std::vector<LoadedSurfaceRotated> loadedFencesRotated16;
	std::vector<LoadedSurfaceRotated> loadedFencesRotated32;
	std::vector<LoadedSprite> loadedSprites;
	std::vector<LoadedTurbulent> loadedTurbulent16;
	std::vector<LoadedTurbulent> loadedTurbulent32;
	std::vector<LoadedTurbulentRotated> loadedTurbulentRotated16;
	std::vector<LoadedTurbulentRotated> loadedTurbulentRotated32;
	std::vector<LoadedAlias> loadedAlias16;
	std::vector<LoadedAlias> loadedAlias32;
	std::vector<LoadedAlias> loadedViewmodels16;
	std::vector<LoadedAlias> loadedViewmodels32;
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
	SharedMemoryBuffer* latestSharedMemoryIndexBuffer16;
	VkDeviceSize usedInLatestSharedMemoryIndexBuffer16;
	SharedMemoryBuffer* latestSharedMemoryIndexBuffer32;
	VkDeviceSize usedInLatestSharedMemoryIndexBuffer32;
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
	VkDeviceSize coloredIndices16Size;
	VkDeviceSize controllerIndices16Size;
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

	static void CopyImage(AppState& appState, unsigned char* source, uint32_t* target, int width, int height);
	static void AddBorder(AppState& appState, std::vector<uint32_t>& target);
	void Create(AppState& appState, VkCommandBufferAllocateInfo& commandBufferAllocateInfo, VkCommandBuffer& setupCommandBuffer, VkCommandBufferBeginInfo& commandBufferBeginInfo, VkSubmitInfo& setupSubmitInfo, struct android_app* app);
	static void CreateShader(AppState& appState, struct android_app* app, const char* filename, VkShaderModule* shaderModule);
	void Initialize();
	void GetSurfaceIndicesStagingBufferSize(AppState& appState, dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	void GetIndices16StagingBufferSize(AppState& appState, dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	void GetIndices32StagingBufferSize(AppState& appState, dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	void GetIndices16StagingBufferSize(AppState& appState, dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	void GetIndices32StagingBufferSize(AppState& appState, dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	void GetAliasIndices16StagingBufferSize(AppState& appState, dalias_t& alias, LoadedAlias& loaded, VkDeviceSize& size);
	void GetAliasIndices32StagingBufferSize(AppState& appState, dalias_t& alias, LoadedAlias& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dturbulentrotated_t& turbulent, LoadedTurbulentRotated& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dalias_t& alias, LoadedAlias& loaded, Texture* host_colormap, VkDeviceSize& size);
	void AddToBufferBarrier(VkBuffer buffer);
	void Reset();
};
