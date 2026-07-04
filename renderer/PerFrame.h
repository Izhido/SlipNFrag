#pragma once

#include "CachedBuffers.h"
#include "CachedTextures.h"
#include "DescriptorResources.h"
#include "PushConstants.h"
#include "AliasColoredLightsPushConstants.h"

struct PerFrame
{
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkFence fence;
	VkImage colorImage;
	VkImage depthImage;
	VkImage resolveImage;
	VmaAllocation colorAllocation;
	VmaAllocation depthAllocation;
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
	DescriptorResources hostColormapResources;
	DescriptorResources sceneMatricesResources;
	DescriptorResources sceneMatricesAndPaletteResources;
	DescriptorResources aliasResources;
	DescriptorResources sceneMatricesAndNeutralPaletteResources;
	DescriptorResources aliasNeutralPaletteResources;
	DescriptorResources sceneMatricesAndColormapResources;
    DescriptorResources sortedAttributesResources;
	DescriptorResources floorResources;
	DescriptorResources controllerResources;
	DescriptorResources handResources;
	DescriptorResources statusBarResources;
	VkDeviceSize controllersVertexBase;
	VkDeviceSize handsVertexBase;
	VkDeviceSize statusBarVertexBase;
	VkDeviceSize skyVertexBase;
	VkDeviceSize skyboxVertexBase;
	VkDeviceSize coloredVertexBase;
	VkDeviceSize cutoutVertexBase;
	VkDeviceSize controllersAttributeBase;
	VkDeviceSize handsAttributeBase;
	VkDeviceSize statusBarAttributeBase;
	VkDeviceSize skyAttributeBase;
	VkDeviceSize skyboxAttributeBase;
	VkDeviceSize aliasLightBase;
    VkDeviceSize aliasAttributeBase;
	VkDeviceSize controllersIndexBase;
	VkDeviceSize handsIndexBase;
	VkDeviceSize statusBarIndexBase;
	VkDeviceSize skyboxIndexBase;
	VkDeviceSize coloredIndex8Base;
	VkDeviceSize coloredIndex16Base;
	VkDeviceSize cutoutIndex8Base;
	VkDeviceSize cutoutIndex16Base;
	VkDeviceSize cutoutIndex32Base;
    Buffer* previousSortedAttributes;

	static float GammaCorrect(float component);
	static unsigned char AveragePixels(std::vector<unsigned char>& pixdata);
	static void GenerateMipmaps(Buffer* stagingBuffer, VkDeviceSize offset, struct LoadedSharedMemoryTexture* loadedTexture, std::vector<unsigned char>& pixdata);
	void LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer);
	void LoadNonStagedResources(AppState& appState);
	void FillAliasFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, struct LoadedIndexBuffer* first, VkBufferCopy& bufferCopy, Buffer*& previousBuffer) const;
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, uint32_t swapchainImageIndex);
	void Reset(AppState& appState);
	static void SetTintPushConstants(const AppState& appState, PushConstants& pushConstants);
	static void SetTintPushConstants(const AppState& appState, AliasColoredLightsPushConstants& pushConstants);
	static void SetTintAndTimePushConstants(const AppState& appState, PushConstants& pushConstants);
	void Render(AppState& appState, uint32_t swapchainImageIndex);
	void DestroyFramebuffer(AppState& appState) const;
	void Destroy(AppState& appState);
};
