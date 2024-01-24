#pragma once

#include "CachedBuffers.h"
#include "CachedTextures.h"
#include "DescriptorResources.h"
#include "DescriptorResourcesLists.h"

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
	CachedBuffers cachedVertices;
	CachedBuffers cachedAttributes;
    CachedBuffers cachedStorageAttributes;
	CachedBuffers cachedIndices8;
	CachedBuffers cachedIndices16;
	CachedBuffers cachedIndices32;
	CachedBuffers cachedColors;
	CachedBuffers stagingBuffers;
	CachedTextures colormaps;
	int colormapCount;
    int paletteChangedFrame;
	Texture* colormap;
	Buffer* vertices;
	Buffer* attributes;
    Buffer* storageAttributes;
	Buffer* indices8;
	Buffer* indices16;
	Buffer* indices32;
	Buffer* colors;
	Texture* sky;
	Texture* skyRGBA;
	DescriptorResources skyResources;
	DescriptorResources skyRGBAResources;
	DescriptorResources host_colormapResources;
	DescriptorResources sceneMatricesResources;
	DescriptorResources sceneMatricesAndPaletteResources;
	DescriptorResources sceneMatricesAndNeutralPaletteResources;
	DescriptorResources sceneMatricesAndColormapResources;
    DescriptorResources storageAttributesResources;
	DescriptorResourcesLists colormapResources;
	DescriptorResources floorResources;
	DescriptorResources controllerResources;
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
    Buffer* previousStorageAttributes;

	static float GammaCorrect(float component);
	static byte AveragePixels(std::vector<byte>& pixdata);
	static void GenerateMipmaps(Buffer* stagingBuffer, VkDeviceSize offset, LoadedSharedMemoryTexture* loadedTexture);
	void LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer);
	void FillColormapTextures(AppState& appState, LoadedAlias& loaded);
	void FillAliasFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, VkCommandBuffer commandBuffer, LoadedIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const;
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex);
	void Reset(AppState& appState);
	static void SetPushConstants(const LoadedAlias& alias, float pushConstants[]);
	static void SetTintPushConstants(float pushConstants[]);
	void Render(AppState& appState, VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex);
};
