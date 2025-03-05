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
	CachedBuffers cachedHostVisibleVertices;
	CachedSharedMemoryBuffers cachedAttributes;
    CachedSharedMemoryBuffers cachedStorageAttributes;
	CachedSharedMemoryBuffers cachedIndices8;
	CachedSharedMemoryBuffers cachedIndices16;
	CachedSharedMemoryBuffers cachedIndices32;
	CachedSharedMemoryBuffers cachedColors;
	CachedBuffers stagingBuffers;
	CachedTextures colormaps;
	int colormapCount;
    int paletteChangedFrame;
	SharedMemoryBuffer* vertices;
	Buffer* sortedVertices;
	SharedMemoryBuffer* attributes;
	SharedMemoryBuffer* storageAttributes;
	SharedMemoryBuffer* indices8;
	SharedMemoryBuffer* indices16;
	SharedMemoryBuffer* indices32;
	SharedMemoryBuffer* colors;
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
    SharedMemoryBuffer* previousStorageAttributes;

	static float GammaCorrect(float component);
	static byte AveragePixels(std::vector<byte>& pixdata);
	static void GenerateMipmaps(Buffer* stagingBuffer, VkDeviceSize offset, LoadedSharedMemoryTexture* loadedTexture);
	void LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer);
	void LoadNonStagedResources(AppState& appState);
	void FillColormapTextures(AppState& appState, LoadedAlias& loaded);
	void FillAliasFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const;
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, uint32_t swapchainImageIndex);
	void Reset(AppState& appState);
	static void SetPushConstants(const LoadedAlias& alias, float pushConstants[]);
	static void SetTintPushConstants(float pushConstants[]);
	void Render(AppState& appState, uint32_t swapchainImageIndex);
};
