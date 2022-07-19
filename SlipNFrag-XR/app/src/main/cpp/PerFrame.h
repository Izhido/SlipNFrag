#pragma once

#include "CachedBuffers.h"
#include "CachedTextures.h"
#include "UpdatablePipelineDescriptorResources.h"

struct PerFrame
{
	VkImage colorImage;
	VkImage depthImage;
	VkImage resolveImage;
	VkDeviceMemory colorMemory;
	VkDeviceMemory depthMemory;
	VkImageView colorView;
	VkImageView depthView;
	VkImageView resolveView;
	VkFramebuffer framebuffer;
	VkCommandBuffer commandBuffer;
	VkSubmitInfo submitInfo;
	CachedBuffers cachedVertices;
	CachedBuffers cachedAttributes;
	CachedBuffers cachedIndices8;
	CachedBuffers cachedIndices16;
	CachedBuffers cachedIndices32;
	CachedBuffers cachedColors;
	CachedBuffers stagingBuffers;
	CachedTextures colormaps;
	int colormapCount;
	int paletteChanged;
	Buffer* palette;
	Texture* host_colormap;
	Buffer matrices;
	Buffer* vertices;
	Buffer* attributes;
	Buffer* indices8;
	Buffer* indices16;
	Buffer* indices32;
	Buffer* colors;
	Texture* sky;
	Texture* skyRGBA;
	PipelineDescriptorResources skyResources;
	PipelineDescriptorResources skyRGBAResources;
	PipelineDescriptorResources host_colormapResources;
	PipelineDescriptorResources sceneMatricesResources;
	PipelineDescriptorResources sceneMatricesAndPaletteResources;
	PipelineDescriptorResources sceneMatricesAndColormapResources;
	UpdatablePipelineDescriptorResources colormapResources;
	PipelineDescriptorResources floorResources;
	PipelineDescriptorResources controllerResources;
	VkDeviceSize controllerVertexBase;
	VkDeviceSize texturedVertexBase;
	VkDeviceSize particlePositionBase;
	VkDeviceSize coloredVertexBase;
	VkDeviceSize controllerAttributeBase;
	VkDeviceSize texturedAttributeBase;
	VkDeviceSize colormappedAttributeBase;
	VkDeviceSize coloredColorBase;
	VkDeviceSize controllerIndexBase;
	VkDeviceSize coloredIndex8Base;
	VkDeviceSize coloredIndex16Base;

	static float GammaCorrect(float component);
	static byte AveragePixels(std::vector<byte>& pixdata);
	static void GenerateMipmaps(Buffer* stagingBuffer, VkDeviceSize offset, LoadedSharedMemoryTexture* loadedTexture);
	void LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer);
	void FillColormapTextures(AppState& appState, LoadedAlias& loaded);
	void FillAliasFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const;
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryBuffer* first, VkBufferCopy& bufferCopy) const;
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer);
	void Reset(AppState& appState);
	static void SetPushConstants(const LoadedAlias& alias, float pushConstants[]);
	void Render(AppState& appState);
};
