#pragma once

#include "CachedBuffers.h"
#include "CachedTextures.h"
#include "PipelineDescriptorResources.h"
#include "UpdatablePipelineDescriptorResources.h"
#include "vid_ovr.h"
#include "d_lists.h"
#include "LoadedSurface.h"
#include "LoadedSurfaceRotated.h"
#include "LoadedAlias.h"
#include "LoadedTurbulent.h"

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
	Texture* palette;
	Texture* host_colormap;
	int hostClearCount;
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
	VkDeviceSize vertexTransformBase;
	VkDeviceSize controllerAttributeBase;
	VkDeviceSize coloredIndex16Base;
	VkDeviceSize controllerIndex16Base;
	VkCommandBuffer commandBuffer;
	VkFence fence;
	bool submitted;

	void Reset(AppState& appState);
	static void GetIndices16StagingBufferSize(AppState& appState, dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	static void GetIndices16StagingBufferSize(AppState& appState, dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	static void GetIndices32StagingBufferSize(AppState& appState, dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	static void GetIndices32StagingBufferSize(AppState& appState, dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	static void GetAliasIndices16StagingBufferSize(AppState& appState, dalias_t& alias, LoadedAlias& loaded, VkDeviceSize& size);
	static void GetAliasIndices32StagingBufferSize(AppState& appState, dalias_t& alias, LoadedAlias& loaded, VkDeviceSize& size);
	static void GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	static void GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	static void GetStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dalias_t& alias, LoadedAlias& loaded, VkDeviceSize& size) const;
	VkDeviceSize GetStagingBufferSize(AppState& appState);
	void LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer);
	void FillColormapTextures(AppState& appState, LoadedColormappedTexture& loadedTexture, dalias_t& alias);
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const;
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryAliasIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const;
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryBuffer* first, VkBufferCopy& bufferCopy) const;
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer);
	static void SetPushConstants(const LoadedSurface& loaded, float pushConstants[]);
	static void SetPushConstants(const LoadedSurfaceRotated& loaded, float pushConstants[]);
	static void SetPushConstants(const LoadedTurbulent& loaded, float pushConstants[]);
	static void SetPushConstants(const LoadedAlias& alias, float pushConstants[]);
	void Render(AppState& appState);
};
