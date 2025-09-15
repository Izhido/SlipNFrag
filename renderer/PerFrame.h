#pragma once

#include "CachedBuffers.h"
#include "CachedSharedMemoryBuffers.h"
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
	CachedSharedMemoryBuffers cachedVertices;
	CachedBuffers cachedSortedVertices;
	CachedSharedMemoryBuffers cachedAttributes;
    CachedBuffers cachedSortedAttributes;
	CachedSharedMemoryBuffers cachedIndices8;
	CachedSharedMemoryBuffers cachedIndices16;
	CachedBuffers cachedSortedIndices16;
	CachedSharedMemoryBuffers cachedIndices32;
	CachedBuffers cachedSortedIndices32;
	CachedSharedMemoryBuffers cachedColors;
	CachedBuffers stagingBuffers;
	CachedTextures colormaps;
	int colormapCount;
    int paletteChangedFrame;
	SharedMemoryBuffer* vertices;
	Buffer* sortedVertices;
	SharedMemoryBuffer* attributes;
	Buffer* sortedAttributes;
	SharedMemoryBuffer* indices8;
	SharedMemoryBuffer* indices16;
	Buffer* sortedIndices16;
	SharedMemoryBuffer* indices32;
	Buffer* sortedIndices32;
	SharedMemoryBuffer* colors;
	Texture* sky;
	Texture* skyRGBA;
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
	VkDeviceSize controllerVertexBase;
	VkDeviceSize skyVertexBase;
	VkDeviceSize controllerAttributeBase;
	VkDeviceSize skyAttributeBase;
	VkDeviceSize aliasAttributeBase;
	VkDeviceSize controllerIndexBase;
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
	void FillAliasFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const;
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, uint32_t swapchainImageIndex);
	void Reset(AppState& appState);
	static void SetPushConstants(const LoadedAliasColoredLights& alias, float pushConstants[]);
	static void SetTintPushConstants(float pushConstants[], size_t offset = 0);
	void Render(AppState& appState, uint32_t swapchainImageIndex);
	void DestroyFramebuffer(AppState& appState) const;
	void Destroy(AppState& appState);
};
