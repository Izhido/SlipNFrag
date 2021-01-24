#pragma once

#include "CachedBuffers.h"
#include "CachedTextures.h"
#include "PipelineDescriptorResources.h"
#include "UpdatablePipelineDescriptorResources.h"
#include "CachedPipelineDescriptorResources.h"

struct View;

struct PerImage
{
	CachedBuffers sceneMatricesStagingBuffers;
	CachedBuffers vertices;
	CachedBuffers attributes;
	CachedBuffers indices16;
	CachedBuffers indices32;
	CachedBuffers colors;
	CachedBuffers stagingBuffers;
	CachedTextures turbulent;
	CachedTextures colormaps;
	int colormapCount;
	int resetDescriptorSetsCount;
	int paletteOffset;
	int host_colormapOffset;
	int skyOffset;
	int paletteChanged;
	Texture* palette;
	Texture* host_colormap;
	int hostClearCount;
	Texture* sky;
	PipelineDescriptorResources host_colormapResources;
	PipelineDescriptorResources sceneMatricesResources;
	PipelineDescriptorResources sceneMatricesAndPaletteResources;
	UpdatablePipelineDescriptorResources texturedResources;
	CachedPipelineDescriptorResources spriteResources;
	UpdatablePipelineDescriptorResources colormapResources;
	CachedPipelineDescriptorResources aliasResources;
	CachedPipelineDescriptorResources viewmodelResources;
	PipelineDescriptorResources skyResources;
	PipelineDescriptorResources floorResources;
	VkDeviceSize texturedVertexBase;
	VkDeviceSize coloredVertexBase;
	VkDeviceSize texturedAttributeBase;
	VkDeviceSize colormappedAttributeBase;
	VkDeviceSize vertexTransformBase;
	VkDeviceSize colormappedIndex16Base;
	VkDeviceSize coloredIndex16Base;
	VkDeviceSize coloredIndex32Base;
	VkCommandBuffer commandBuffer;
	VkFence fence;
	bool submitted;

	void Reset(AppState& appState);
	void GetStagingBufferSize(AppState& appState, View& view, VkDeviceSize& stagingBufferSize, VkDeviceSize& floorSize);
	void LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer, VkDeviceSize stagingBufferSize, int floorSize);
	void FillTextures(AppState& appState, Buffer* stagingBuffer);
	void Render(AppState& appState, VkDescriptorPoolSize poolSizes[], VkDescriptorPoolCreateInfo& descriptorPoolCreateInfo, VkWriteDescriptorSet writes[], VkDescriptorSetAllocateInfo& descriptorSetAllocateInfo, VkDescriptorImageInfo textureInfo[]);
};
