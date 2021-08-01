#pragma once

#include <vulkan/vulkan.h>
#include "Pipeline.h"
#include "Buffer.h"
#include "CachedSharedMemoryBuffers.h"
#include <unordered_map>
#include "AliasVertices.h"
#include "TwinKey.h"
#include "SharedMemoryBufferWithOffset.h"
#include "LoadedSurface.h"
#include "LoadedSprite.h"
#include "LoadedTurbulent.h"
#include "LoadedAlias.h"
#include "AllocationList.h"
#include "CachedLightmaps.h"
#include "CachedSharedMemoryTextures.h"
#include <openxr/openxr.h>
#include <common/xr_linear.h>

struct AppState;

struct Scene
{
	bool created;
	VkShaderModule surfaceVertex;
	VkShaderModule surfaceFragment;
	VkShaderModule fenceFragment;
	VkShaderModule spriteVertex;
	VkShaderModule spriteFragment;
	VkShaderModule turbulentVertex;
	VkShaderModule turbulentFragment;
	VkShaderModule aliasVertex;
	VkShaderModule aliasFragment;
	VkShaderModule viewmodelVertex;
	VkShaderModule viewmodelFragment;
	VkShaderModule coloredVertex;
	VkShaderModule coloredFragment;
	VkShaderModule skyVertex;
	VkShaderModule skyFragment;
	VkShaderModule floorVertex;
	VkShaderModule floorFragment;
	VkShaderModule consoleVertex;
	VkShaderModule consoleFragment;
	VkDescriptorSetLayout singleBufferLayout;
	VkDescriptorSetLayout bufferAndImageLayout;
	VkDescriptorSetLayout bufferAndTwoImagesLayout;
	VkDescriptorSetLayout singleImageLayout;
	VkDescriptorSetLayout doubleImageLayout;
	Pipeline surfaces;
	Pipeline fences;
	Pipeline sprites;
	Pipeline turbulent;
	Pipeline alias;
	Pipeline viewmodel;
	Pipeline colored;
	Pipeline sky;
	Pipeline floor;
	Pipeline console;
	Buffer matrices;
	int numBuffers;
	std::vector<XrMatrix4x4f> viewMatrices;
	std::vector<XrMatrix4x4f> projectionMatrices;
	int hostClearCount;
	CachedSharedMemoryBuffers buffers;
	std::unordered_map<void*, SharedMemoryBuffer*> verticesPerKey;
	std::unordered_map<void*, SharedMemoryBuffer*> texturePositionsPerKey;
	std::unordered_map<void*, AliasVertices> aliasVerticesPerKey;
	std::unordered_map<TwinKey, SharedMemoryBufferWithOffset> indicesPerKey;
	std::unordered_map<void*, SharedMemoryBufferWithOffset> aliasIndicesPerKey;
	int lastSurface16;
	int lastSurface32;
	int lastFence16;
	int lastFence32;
	int lastSprite;
	int lastTurbulent16;
	int lastTurbulent32;
	int lastAlias16;
	int lastAlias32;
	int lastViewmodel16;
	int lastViewmodel32;
	int lastColoredIndex16;
	int lastColoredIndex32;
	int lastSky;
	std::vector<LoadedSurface> loadedSurfaces16;
	std::vector<LoadedSurface> loadedSurfaces32;
	std::vector<LoadedSurface> loadedFences16;
	std::vector<LoadedSurface> loadedFences32;
	std::vector<LoadedSprite> loadedSprites;
	std::vector<LoadedTurbulent> loadedTurbulent16;
	std::vector<LoadedTurbulent> loadedTurbulent32;
	std::vector<LoadedAlias> loadedAlias16;
	std::vector<LoadedAlias> loadedAlias32;
	std::vector<LoadedAlias> loadedViewmodels16;
	std::vector<LoadedAlias> loadedViewmodels32;
	int skyCount;
	int firstSkyVertex;
	std::unordered_map<VkDeviceSize, AllocationList> allocations;
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
	SharedMemoryBuffer* latestSharedMemoryIndexBuffer16;
	VkDeviceSize usedInLatestSharedMemoryIndexBuffer16;
	SharedMemoryBuffer* latestSharedMemoryIndexBuffer32;
	VkDeviceSize usedInLatestSharedMemoryIndexBuffer32;
	SharedMemory* latestTextureSharedMemory;
	VkDeviceSize usedInLatestTextureSharedMemory;
	DescriptorSets* latestTextureDescriptorSets;
	VkDeviceSize verticesSize;
	VkDeviceSize paletteSize;
	VkDeviceSize host_colormapSize;
	VkDeviceSize skySize;
	VkDeviceSize controllerVerticesSize;
	XrQuaternionf orientation;
	StagingBuffer stagingBuffer;
	void* previousVertexes;
	void* previousSurface;
	void* previousTexture;
	void* previousApverts;
	SharedMemoryBuffer* previousVertexBuffer;
	SharedMemoryBuffer* previousTexturePosition;
	SharedMemoryBuffer* previousTexCoordsBuffer;
	SharedMemoryTexture* previousSharedMemoryTexture;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

	void CopyImage(AppState& appState, unsigned char* source, uint32_t* target, int width, int height);
	void AddBorder(AppState& appState, std::vector<uint32_t>& target);
	void Create(AppState& appState, VkCommandBufferAllocateInfo& commandBufferAllocateInfo, VkCommandBuffer& setupCommandBuffer, VkCommandBufferBeginInfo& commandBufferBeginInfo, VkSubmitInfo& setupSubmitInfo, struct android_app* app);
	void CreateShader(AppState& appState, struct android_app* app, const char* filename, VkShaderModule* shaderModule);
	void Initialize();
	void AddToBufferBarrier(VkBuffer buffer);
	void Reset();
};
