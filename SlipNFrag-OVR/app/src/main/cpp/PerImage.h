#pragma once

#include "CachedBuffers.h"
#include "CachedTextures.h"
#include "PipelineDescriptorResources.h"
#include "UpdatablePipelineDescriptorResources.h"
#include "CachedPipelineDescriptorResources.h"
#include "vid_ovr.h"
#include "d_lists.h"
#include "LoadedTexture.h"
#include "LoadedLightmap.h"
#include "LoadedSharedMemoryTexture.h"
#include "LoadedColormappedTexture.h"

struct View;

struct PerImage
{
	CachedBuffers sceneMatricesStagingBuffers;
	CachedBuffers cachedVertices;
	CachedBuffers cachedAttributes;
	CachedBuffers cachedIndices16;
	CachedBuffers cachedIndices32;
	CachedBuffers cachedColors;
	CachedBuffers stagingBuffers;
	CachedTextures turbulent;
	CachedTextures colormaps;
	int colormapCount;
	int resetDescriptorSetsCount;
	VkDeviceSize paletteSize;
	VkDeviceSize host_colormapSize;
	VkDeviceSize skySize;
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
	CachedPipelineDescriptorResources surfaceTextureResources;
	CachedPipelineDescriptorResources fenceTextureResources;
	CachedPipelineDescriptorResources spriteResources;
	UpdatablePipelineDescriptorResources turbulentResources;
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
	VkDeviceSize surfaceIndex16Base;
	VkDeviceSize colormappedIndex16Base;
	VkDeviceSize colormappedIndex32Base;
	VkDeviceSize coloredIndex16Base;
	VkDeviceSize coloredIndex32Base;
	VkCommandBuffer commandBuffer;
	VkFence fence;
	bool submitted;

	void Reset(AppState& appState);
	void LoadSurfaceVertexes(AppState& appState, VkBufferMemoryBarrier& bufferMemoryBarrier, void* vertexes, int vertexCount);
	void LoadAliasBuffers(AppState& appState, int lastAlias, std::vector<dalias_t>& aliasList, std::vector<int>& aliasVertices, std::vector<int>& aliasTexCoords, VkBufferMemoryBarrier& bufferMemoryBarrier);
	void LoadBuffers(AppState& appState, VkBufferMemoryBarrier& bufferMemoryBarrier);
	void GetSurfaceStagingBufferSize(AppState& appState, View& view, dsurface_t& surface, LoadedLightmap& lightmap, VkDeviceSize& stagingBufferSize);
	void GetTurbulentStagingBufferSize(AppState& appState, View& view, dturbulent_t& turbulent, LoadedTexture& loadedTexture, VkDeviceSize& stagingBufferSize);
	void GetAliasColormapStagingBufferSize(AppState& appState, int lastAlias, std::vector<dalias_t>& aliasList, std::vector<LoadedColormappedTexture>& textures, VkDeviceSize& stagingBufferSize);
	void GetAliasStagingBufferSize(AppState& appState, int lastAlias, std::vector<dalias_t>& aliasList, std::vector<LoadedColormappedTexture>& textures, VkDeviceSize& stagingBufferSize);
	void GetViewmodelStagingBufferSize(AppState& appState, int lastViewmodel, std::vector<dalias_t>& viewmodelList, std::vector<LoadedColormappedTexture>& textures, VkDeviceSize& stagingBufferSize);
	void SetupLoadedLightmap(AppState& appState, dsurface_t& surface, LoadedLightmap& loadedLightmap, VkDeviceSize& stagingBufferSize);
	void SetupLoadedSharedMemoryTexture(AppState& appState, LoadedSharedMemoryTexture& sharedMemoryTexture);
	void GetStagingBufferSize(AppState& appState, View& view, VkDeviceSize& stagingBufferSize, VkDeviceSize& floorSize);
	void LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer, VkDeviceSize stagingBufferSize, VkDeviceSize floorSize);
	void FillAliasTextures(AppState& appState, Buffer* stagingBuffer, LoadedColormappedTexture& loadedTexture, dalias_t& alias, VkDeviceSize& offset);
	void FillTextures(AppState& appState, Buffer* stagingBuffer, VkDeviceSize floorSize);
	void SetPushConstants(dsurface_t& surface, float pushConstants[]);
	void SetPushConstants(dturbulent_t& turbulent, float pushConstants[]);
	void SetPushConstants(dalias_t& alias, float pushConstants[]);
	VkDescriptorSet GetDescriptorSet(AppState& appState, SharedMemoryTexture* texture, CachedPipelineDescriptorResources& resources, VkDescriptorImageInfo textureInfo[], VkWriteDescriptorSet writes[]);
	void Render(AppState& appState);
};
