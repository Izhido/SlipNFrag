#pragma once

#include "CachedBuffers.h"
#include "CachedTextures.h"
#include "PipelineDescriptorResources.h"
#include "UpdatablePipelineDescriptorResources.h"
#include "SurfaceRotatedPushConstants.h"
#include "TurbulentLitPushConstants.h"
#include "TurbulentRotatedLitPushConstants.h"

struct PerImage
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
	Buffer* indices16;
	Buffer* indices32;
	Buffer* colors;
	Texture* sky;
	PipelineDescriptorResources host_colormapResources;
	PipelineDescriptorResources sceneMatricesResources;
	PipelineDescriptorResources sceneMatricesAndPaletteResources;
	PipelineDescriptorResources sceneMatricesAndColormapResources;
	UpdatablePipelineDescriptorResources colormapResources;
	PipelineDescriptorResources skyResources;
	PipelineDescriptorResources floorResources;
	PipelineDescriptorResources controllerResources;
	VkDeviceSize texturedVertexBase;
	VkDeviceSize coloredVertexBase;
	VkDeviceSize controllerVertexBase;
	VkDeviceSize texturedAttributeBase;
	VkDeviceSize colormappedAttributeBase;
	VkDeviceSize controllerAttributeBase;
	VkDeviceSize coloredIndex16Base;
	VkDeviceSize controllerIndex16Base;
	VkCommandBuffer commandBuffer;
	VkSubmitInfo submitInfo;

	void Reset(AppState& appState);
	VkDeviceSize GetStagingBufferSize(AppState& appState);
	static float GammaCorrect(float component);
	void LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer);
	void FillColormapTextures(AppState& appState, LoadedAlias& loaded);
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const;
	void FillAliasFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryWithOffsetBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const;
	void FillTexturePositionsFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryTexturePositionsBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const;
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryBuffer* first, VkBufferCopy& bufferCopy) const;
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer);
	static void SetPushConstants(const LoadedSurfaceRotated& loaded, SurfaceRotatedPushConstants& pushConstants);
	static void SetPushConstants(const LoadedAlias& alias, float pushConstants[]);
	void Render(AppState& appState);
};
