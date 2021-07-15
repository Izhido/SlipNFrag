#pragma once

#include <vulkan/vulkan.h>
#include "Pipeline.h"
#include "PipelineAttributes.h"
#include "CachedSharedMemoryBuffers.h"
#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"
#include "CachedBuffers.h"
#include <unordered_map>
#include "CachedSharedMemoryTextures.h"
#include "CachedLightmaps.h"
#include "LoadedSharedMemoryTexture.h"
#include "LoadedTexture.h"
#include "LoadedColormappedTexture.h"
#include "VrApi.h"
#include "Instance.h"
#include "StagingBuffer.h"
#include "DescriptorSets.h"

struct Scene
{
	bool createdScene;
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
	std::unordered_map<void*, SharedMemoryBuffer*> texCoordsPerKey;
	std::vector<LoadedSharedMemoryBuffer> surfaceVertex16List;
	std::vector<LoadedSharedMemoryBuffer> surfaceVertex32List;
	std::vector<LoadedSharedMemoryBuffer> fenceVertex16List;
	std::vector<LoadedSharedMemoryBuffer> fenceVertex32List;
	std::vector<LoadedSharedMemoryBuffer> turbulentVertex16List;
	std::vector<LoadedSharedMemoryBuffer> turbulentVertex32List;
	std::vector<LoadedSharedMemoryBuffer> aliasVertex16List;
	std::vector<LoadedSharedMemoryTexCoordsBuffer> aliasTexCoords16List;
	std::vector<LoadedSharedMemoryBuffer> aliasVertex32List;
	std::vector<LoadedSharedMemoryTexCoordsBuffer> aliasTexCoords32List;
	std::vector<LoadedSharedMemoryBuffer> viewmodelVertex16List;
	std::vector<LoadedSharedMemoryTexCoordsBuffer> viewmodelTexCoords16List;
	std::vector<LoadedSharedMemoryBuffer> viewmodelVertex32List;
	std::vector<LoadedSharedMemoryTexCoordsBuffer> viewmodelTexCoords32List;
	std::unordered_map<VkDeviceSize, AllocationList> allocations;
	CachedLightmaps lightmaps;
	CachedSharedMemoryTextures textures;
	std::unordered_map<void*, SharedMemoryTexture*> surfaceTexturesPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> fenceTexturesPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> spritesPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> turbulentPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> aliasTexturesPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> viewmodelTexturesPerKey;
	std::vector<LoadedLightmap> surfaceLightmap16List;
	std::vector<LoadedLightmap> surfaceLightmap32List;
	std::vector<LoadedLightmap> fenceLightmap16List;
	std::vector<LoadedLightmap> fenceLightmap32List;
	std::vector<LoadedSharedMemoryTexture> surfaceTexture16List;
	std::vector<LoadedSharedMemoryTexture> surfaceTexture32List;
	std::vector<LoadedSharedMemoryTexture> fenceTexture16List;
	std::vector<LoadedSharedMemoryTexture> fenceTexture32List;
	std::vector<LoadedSharedMemoryTexture> spriteList;
	std::vector<LoadedSharedMemoryTexture> turbulent16List;
	std::vector<LoadedSharedMemoryTexture> turbulent32List;
	std::vector<LoadedColormappedTexture> alias16List;
	std::vector<LoadedColormappedTexture> alias32List;
	std::vector<LoadedColormappedTexture> viewmodel16List;
	std::vector<LoadedColormappedTexture> viewmodel32List;
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
	SharedMemoryBuffer* previousVertexBuffer;
	SharedMemoryBuffer* previousTexCoordsBuffer;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

	void CopyImage(AppState& appState, unsigned char* source, uint32_t* target, int width, int height);
	void AddBorder(AppState& appState, std::vector<uint32_t>& target);
	void Create(AppState& appState, VkCommandBufferAllocateInfo& commandBufferAllocateInfo, VkCommandBuffer& setupCommandBuffer, VkCommandBufferBeginInfo& commandBufferBeginInfo, VkSubmitInfo& setupSubmitInfo, Instance& instance, VkFenceCreateInfo& fenceCreateInfo, struct android_app* app);
	void CreateShader(AppState& appState, struct android_app* app, const char* filename, VkShaderModule* shaderModule);
	void Initialize();
	void Reset();
};
