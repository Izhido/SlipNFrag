#pragma once

#include <vulkan/vulkan.h>
#include "Pipeline.h"
#include "PipelineAttributes.h"
#include "LoadedSurface.h"
#include "LoadedSprite.h"
#include "LoadedTurbulent.h"
#include "LoadedAlias.h"
#include "CachedSharedMemoryBuffers.h"
#include "CachedBuffers.h"
#include "CachedSharedMemoryTextures.h"
#include "CachedLightmaps.h"
#include "VrApi.h"
#include "Instance.h"
#include "DescriptorSets.h"

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
	int hostClearCount;
	CachedSharedMemoryBuffers buffers;
	std::unordered_map<void*, SharedMemoryBuffer*> verticesPerKey;
	std::unordered_map<void*, SharedMemoryBuffer*> texturePositionsPerKey;
	std::unordered_map<void*, SharedMemoryBuffer*> texCoordsPerKey;
	std::unordered_map<TwinKey, SharedMemoryBuffer*> indicesPerKey;
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
	SharedMemory* latestTextureSharedMemory;
	VkDeviceSize usedInLatestTextureSharedMemory;
	DescriptorSets* latestTextureDescriptorSets;
	ovrTextureSwapChain* skybox;
	VkDeviceSize verticesSize;
	VkDeviceSize paletteSize;
	VkDeviceSize host_colormapSize;
	VkDeviceSize skySize;
	VkDeviceSize controllerVerticesSize;
	ovrQuatf orientation;
	ovrPosef pose;
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
	void Create(AppState& appState, VkCommandBufferAllocateInfo& commandBufferAllocateInfo, VkCommandBuffer& setupCommandBuffer, VkCommandBufferBeginInfo& commandBufferBeginInfo, VkSubmitInfo& setupSubmitInfo, Instance& instance, VkFenceCreateInfo& fenceCreateInfo, struct android_app* app);
	void CreateShader(AppState& appState, struct android_app* app, const char* filename, VkShaderModule* shaderModule);
	void Initialize();
	void Reset();
};
