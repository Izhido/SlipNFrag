#pragma once

#include <vulkan/vulkan.h>
#include "Pipeline.h"
#include "PipelineAttributes.h"
#include "CachedBuffers.h"
#include "Lightmap.h"
#include <unordered_map>
#include "CachedSharedMemoryTextures.h"
#include "BufferWithOffset.h"
#include "LoadedLightmap.h"
#include "LoadedSharedMemoryTexture.h"
#include "LoadedTexture.h"
#include "LoadedColormappedTexture.h"
#include "VrApi.h"
#include "Instance.h"

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
	PipelineAttributes surfaceAttributes;
	PipelineAttributes spriteAttributes;
	PipelineAttributes colormappedAttributes;
	PipelineAttributes coloredAttributes;
	PipelineAttributes skyAttributes;
	PipelineAttributes floorAttributes;
	PipelineAttributes consoleAttributes;
	Buffer matrices;
	int numBuffers;
	int hostClearCount;
	Buffer* oldSurfaceVerticesPerModel;
	std::unordered_map<void*, Buffer*> surfaceVerticesPerModel;
	CachedBuffers colormappedBuffers;
	int resetDescriptorSetsCount;
	Lightmap* oldSurfaces;
	std::unordered_map<VkDeviceSize, AllocationList> allocations;
	std::unordered_map<TwinKey, Lightmap*> lightmaps;
	std::vector<TwinKey> lightmapsToDelete;
	CachedSharedMemoryTextures surfaceTextures;
	int surfaceTextureCount;
	CachedSharedMemoryTextures fenceTextures;
	int fenceTextureCount;
	CachedSharedMemoryTextures spriteTextures;
	int spriteTextureCount;
	CachedSharedMemoryTextures aliasTextures;
	int aliasTextureCount;
	CachedSharedMemoryTextures viewmodelTextures;
	int viewmodelTextureCount;
	std::unordered_map<void*, SharedMemoryTexture*> surfaceTexturesPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> fenceTexturesPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> spritesPerKey;
	std::unordered_map<void*, int> colormappedVerticesPerKey;
	std::unordered_map<void*, int> colormappedTexCoordsPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> aliasPerKey;
	std::unordered_map<void*, SharedMemoryTexture*> viewmodelsPerKey;
	std::vector<BufferWithOffset> colormappedBufferList;
	LoadedLightmap* firstLightmapToCreate;
	LoadedLightmap* currentLightmapToCreate;
	std::vector<LoadedLightmap> surfaceLightmap16List;
	std::vector<LoadedLightmap> surfaceLightmap32List;
	std::vector<LoadedSharedMemoryTexture> surfaceTexture16List;
	std::vector<LoadedSharedMemoryTexture> surfaceTexture32List;
	std::vector<LoadedLightmap> fenceLightmap16List;
	std::vector<LoadedLightmap> fenceLightmap32List;
	std::vector<LoadedSharedMemoryTexture> fenceTexture16List;
	std::vector<LoadedSharedMemoryTexture> fenceTexture32List;
	std::vector<LoadedSharedMemoryTexture> spriteList;
	std::vector<LoadedTexture> turbulent16List;
	std::vector<LoadedTexture> turbulent32List;
	std::vector<LoadedColormappedTexture> alias16List;
	std::vector<LoadedColormappedTexture> alias32List;
	std::vector<LoadedColormappedTexture> viewmodel16List;
	std::vector<LoadedColormappedTexture> viewmodel32List;
	std::vector<int> newVertices;
	std::vector<int> newTexCoords;
	std::vector<int> aliasVertices16List;
	std::vector<int> aliasVertices32List;
	std::vector<int> aliasTexCoords16List;
	std::vector<int> aliasTexCoords32List;
	std::vector<int> viewmodelVertices16List;
	std::vector<int> viewmodelVertices32List;
	std::vector<int> viewmodelTexCoords16List;
	std::vector<int> viewmodelTexCoords32List;
	std::unordered_map<void*, Texture*> turbulentPerKey;
	Texture* floorTexture;
	std::vector<VkSampler> textureSamplers;
	VkSampler lightmapSampler;
	Buffer* latestColormappedBuffer;
	VkDeviceSize usedInLatestColormappedBuffer;
	SharedMemory* latestTextureSharedMemory;
	VkDeviceSize usedInLatestTextureSharedMemory;
	ovrTextureSwapChain* skybox;
	VkDeviceSize stagingBufferSize;
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
	VkDeviceSize surfaceIndices16Size;
	VkDeviceSize colormappedIndices16Size;
	VkDeviceSize coloredIndices16Size;
	VkDeviceSize indices16Size;
	VkDeviceSize surfaceIndices32Size;
	VkDeviceSize colormappedIndices32Size;
	VkDeviceSize coloredIndices32Size;
	VkDeviceSize indices32Size;
	VkDeviceSize colorsSize;
	VkDeviceSize floorSize;
	ovrQuatf orientation;
	ovrPosef pose;

	void Create(AppState& appState, VkCommandBufferAllocateInfo& commandBufferAllocateInfo, VkCommandBuffer& setupCommandBuffer, VkCommandBufferBeginInfo& commandBufferBeginInfo, VkSubmitInfo& setupSubmitInfo, Instance& instance, VkFenceCreateInfo& fenceCreateInfo, struct android_app* app);
	void CreateShader(AppState& appState, struct android_app* app, const char* filename, VkShaderModule* shaderModule);
	void ClearSizes();
	void Reset();
};
