#pragma once

#include "CachedBuffers.h"
#include "CachedTextures.h"
#include "DescriptorResources.h"
#include "DescriptorResourcesLists.h"

struct PerFrame
{
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkFence fence;
	VkImage colorImage;
	VkImage depthImage;
	VkImage resolveImage;
	VkDeviceMemory colorMemory;
	VkDeviceMemory depthMemory;
	VkImageView colorView;
	VkImageView depthView;
	VkImageView resolveView;
	VkFramebuffer framebuffer;
	Buffer* matrices;
	CachedBuffers cachedVertices;
	CachedBuffers cachedSortedVertices;
	CachedBuffers cachedAttributes;
    CachedBuffers cachedSortedAttributes;
	CachedBuffers cachedIndices8;
	CachedBuffers cachedIndices16;
	CachedBuffers cachedSortedIndices16;
	CachedBuffers cachedIndices32;
	CachedBuffers cachedSortedIndices32;
	CachedBuffers cachedColors;
	CachedBuffers stagingBuffers;
	CachedTextures colormaps;
	int colormapCount;
    int paletteChangedFrame;
	Buffer* vertices;
	Buffer* sortedVertices;
	Buffer* attributes;
	Buffer* sortedAttributes;
	Buffer* indices8;
	Buffer* indices16;
	Buffer* sortedIndices16;
	Buffer* indices32;
	Buffer* sortedIndices32;
	Buffer* colors;
	Texture* sky;
	Texture* skyRGBA;
	Texture* statusBar;
	VkDeviceSize particleBase;
	DescriptorResources skyResources;
	DescriptorResources skyRGBAResources;
	DescriptorResources host_colormapResources;
	DescriptorResources sceneMatricesResources;
	DescriptorResources sceneMatricesAndPaletteResources;
	DescriptorResources sceneMatricesAndNeutralPaletteResources;
	DescriptorResources sceneMatricesAndColormapResources;
    DescriptorResources sortedAttributesResources;
	DescriptorResourcesLists colormapResources;
	DescriptorResources floorResources;
	DescriptorResources controllerResources;
	DescriptorResources handResources;
	DescriptorResources statusBarResources;
	VkDeviceSize controllersVertexBase;
	VkDeviceSize handsVertexBase;
	VkDeviceSize statusBarVertexBase;
	VkDeviceSize skyVertexBase;
	VkDeviceSize coloredVertexBase;
	VkDeviceSize cutoutVertexBase;
	VkDeviceSize controllersAttributeBase;
	VkDeviceSize handsAttributeBase;
	VkDeviceSize statusBarAttributeBase;
	VkDeviceSize skyAttributeBase;
	VkDeviceSize aliasAttributeBase;
	VkDeviceSize controllersIndexBase;
	VkDeviceSize handsIndexBase;
	VkDeviceSize statusBarIndexBase;
	VkDeviceSize coloredIndex8Base;
	VkDeviceSize coloredIndex16Base;
	VkDeviceSize cutoutIndex8Base;
	VkDeviceSize cutoutIndex16Base;
	VkDeviceSize cutoutIndex32Base;
    Buffer* previousSortedAttributes;

	static float GammaCorrect(float component);
	static unsigned char AveragePixels(std::vector<unsigned char>& pixdata);
	static void GenerateMipmaps(Buffer* stagingBuffer, VkDeviceSize offset, LoadedSharedMemoryTexture* loadedTexture);
	void LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer);
	void LoadNonStagedResources(AppState& appState);
	void FillColormapTextures(AppState& appState, LoadedAlias& loaded);
	void FillAliasFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedIndexBuffer* first, VkBufferCopy& bufferCopy, Buffer*& previousBuffer) const;
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, uint32_t swapchainImageIndex);
	void Reset(AppState& appState);
	static void SetPushConstants(const LoadedAliasColoredLights& alias, float pushConstants[]);
	static void SetTintPushConstants(float pushConstants[], size_t offset = 0);
	void Render(AppState& appState, uint32_t swapchainImageIndex);
	void DestroyFramebuffer(AppState& appState) const;
	void Destroy(AppState& appState);
};
