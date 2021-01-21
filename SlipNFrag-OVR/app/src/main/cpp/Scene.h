#pragma once

#include <vulkan/vulkan.h>
#include "Pipeline.h"
#include "PipelineAttributes.h"
#include "CachedBuffers.h"
#include "TextureFromAllocation.h"
#include <unordered_map>
#include "CachedSharedMemoryTextures.h"
#include "BufferWithOffset.h"
#include "LoadedTextureFromAllocation.h"
#include "LoadedSharedMemoryTexture.h"
#include "LoadedTexture.h"
#include "LoadedColormappedTexture.h"
#include "VrApi.h"
#include "Instance.h"

struct Scene
{
	bool createdScene;
	VkShaderModule texturedVertex;
	VkShaderModule texturedFragment;
	VkShaderModule surfaceVertex;
	VkShaderModule spritesFragment;
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
	VkDescriptorSetLayout singleImageLayout;
	VkDescriptorSetLayout doubleImageLayout;
	Pipeline textured;
	Pipeline sprites;
	Pipeline turbulent;
	Pipeline alias;
	Pipeline viewmodel;
	Pipeline colored;
	Pipeline sky;
	Pipeline floor;
	Pipeline console;
	PipelineAttributes texturedAttributes;
	PipelineAttributes colormappedAttributes;
	PipelineAttributes coloredAttributes;
	PipelineAttributes skyAttributes;
	PipelineAttributes floorAttributes;
	PipelineAttributes consoleAttributes;
	Buffer matrices;
	int numBuffers;
	int hostClearCount;
	CachedBuffers colormappedBuffers;
	int resetDescriptorSetsCount;
	TextureFromAllocation* oldSurfaces;
	std::unordered_map<VkDeviceSize, AllocationList> allocations;
	std::unordered_map<TwinKey, TextureFromAllocation*> surfaces;
	CachedSharedMemoryTextures spriteTextures;
	int spriteTextureCount;
	CachedSharedMemoryTextures aliasTextures;
	int aliasTextureCount;
	CachedSharedMemoryTextures viewmodelTextures;
	int viewmodelTextureCount;
	std::unordered_map<void*, SharedMemoryTexture*> spritesPerKey;
	std::unordered_map<void*, int> colormappedVerticesPerKey;
	std::unordered_map<void*, int> colormappedTexCoordsPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> aliasPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> viewmodelsPerKey;
	std::vector<BufferWithOffset> colormappedBufferList;
	std::vector<LoadedTextureFromAllocation> surfaceList;
	std::vector<LoadedSharedMemoryTexture> spriteList;
	std::vector<LoadedTexture> turbulentList;
	std::vector<LoadedColormappedTexture> aliasList;
	std::vector<LoadedColormappedTexture> viewmodelList;
	std::vector<int> newVertices;
	std::vector<int> newTexCoords;
	std::vector<int> aliasVerticesList;
	std::vector<int> aliasTexCoordsList;
	std::vector<int> viewmodelVerticesList;
	std::vector<int> viewmodelTexCoordsList;
	std::unordered_map<void*, Texture*> turbulentPerKey;
	int floorOffset;
	Texture* floorTexture;
	std::vector<VkSampler> samplers;
	Buffer* latestColormappedBuffer;
	VkDeviceSize usedInLatestColormappedBuffer;
	SharedMemory* latestTextureSharedMemory;
	VkDeviceSize usedInLatestTextureSharedMemory;
	ovrTextureSwapChain* skybox;
	VkDeviceSize stagingBufferSize;
	VkDeviceSize texturedDescriptorSetCount;
	VkDeviceSize spriteDescriptorSetCount;
	VkDeviceSize colormapDescriptorSetCount;
	VkDeviceSize aliasDescriptorSetCount;
	VkDeviceSize viewmodelDescriptorSetCount;
	VkDeviceSize floorVerticesSize;
	VkDeviceSize texturedVerticesSize;
	VkDeviceSize coloredVerticesSize;
	VkDeviceSize verticesSize;
	VkDeviceSize colormappedVerticesSize;
	VkDeviceSize colormappedTexCoordsSize;
	VkDeviceSize floorAttributesSize;
	VkDeviceSize texturedAttributesSize;
	VkDeviceSize colormappedLightsSize;
	VkDeviceSize vertexTransformSize;
	VkDeviceSize attributesSize;
	VkDeviceSize floorIndicesSize;
	VkDeviceSize colormappedIndices16Size;
	VkDeviceSize coloredIndices16Size;
	VkDeviceSize indices16Size;
	VkDeviceSize colormappedIndices32Size;
	VkDeviceSize coloredIndices32Size;
	VkDeviceSize indices32Size;
	VkDeviceSize floorSize;
	Buffer* vertices;
	Buffer* attributes;
	Buffer* indices16;
	Buffer* indices32;
	ovrQuatf orientation;
	ovrPosef pose;

	void Create(AppState& appState, VkCommandBufferAllocateInfo& commandBufferAllocateInfo, VkCommandBuffer& setupCommandBuffer, VkCommandBufferBeginInfo& commandBufferBeginInfo, VkSubmitInfo& setupSubmitInfo, Instance& instance, VkFenceCreateInfo& fenceCreateInfo, struct android_app* app);
	void CreateShader(AppState& appState, struct android_app* app, const char* filename, VkShaderModule* shaderModule);
	void ClearBuffersAndSizes();
	void Reset();
};
