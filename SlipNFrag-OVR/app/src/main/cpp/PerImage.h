#pragma once

#include "CachedBuffers.h"
#include "CachedTextures.h"
#include "PipelineDescriptorResources.h"
#include "UpdatablePipelineDescriptorResources.h"
#include "CachedPipelineDescriptorResources.h"
#include "vid_ovr.h"
#include "d_lists.h"
#include "LoadedSharedMemoryBuffer.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"
#include "LoadedTexture.h"
#include "LoadedLightmap.h"
#include "LoadedSharedMemoryTexture.h"
#include "LoadedColormappedTexture.h"

struct View;

struct PerImage
{
	CachedBuffers cachedVertices;
	CachedBuffers cachedAttributes;
	CachedBuffers cachedIndices16;
	CachedBuffers cachedIndices32;
	CachedBuffers cachedColors;
	CachedBuffers stagingBuffers;
	CachedTextures colormaps;
	int colormapCount;
	int resetDescriptorSetsCount;
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
	CachedPipelineDescriptorResources turbulentResources;
	UpdatablePipelineDescriptorResources colormapResources;
	CachedPipelineDescriptorResources aliasResources;
	CachedPipelineDescriptorResources viewmodelResources;
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
	VkDeviceSize surfaceIndex16Base;
	VkDeviceSize colormappedIndex16Base;
	VkDeviceSize colormappedIndex32Base;
	VkDeviceSize coloredIndex16Base;
	VkDeviceSize controllerIndex16Base;
	VkDeviceSize coloredIndex32Base;
	VkCommandBuffer commandBuffer;
	VkFence fence;
	bool submitted;

	void Reset(AppState& appState);
	void GetSurfaceVerticesStagingBufferSize(AppState& appState, dsurface_t& surface, LoadedSharedMemoryBuffer& buffer, VkDeviceSize& stagingBufferSize);
	void GetTurbulentVerticesStagingBufferSize(AppState& appState, dturbulent_t& turbulent, LoadedSharedMemoryBuffer& buffer, VkDeviceSize& stagingBufferSize);
	void GetAliasVerticesStagingBufferSize(AppState& appState, dalias_t& alias, LoadedSharedMemoryBuffer& vertexBuffer, LoadedSharedMemoryTexCoordsBuffer& texCoordsBuffer, VkDeviceSize& stagingBufferSize);
	void GetSurfaceLightmapStagingBufferSize(AppState& appState, View& view, dsurface_t& surface, LoadedLightmap& lightmap, VkDeviceSize& stagingBufferSize);
	void GetTurbulentStagingBufferSize(AppState& appState, int lastTurbulent, std::vector<dturbulent_t>& turbulentList, std::vector<LoadedSharedMemoryTexture>& textures, VkDeviceSize& stagingBufferSize);
	void GetAliasColormapStagingBufferSize(AppState& appState, int lastAlias, std::vector<dalias_t>& aliasList, std::vector<LoadedColormappedTexture>& textures, VkDeviceSize& stagingBufferSize);
	void GetAliasStagingBufferSize(AppState& appState, int lastAlias, std::vector<dalias_t>& aliasList, std::vector<LoadedColormappedTexture>& textures, VkDeviceSize& stagingBufferSize);
	void GetViewmodelStagingBufferSize(AppState& appState, int lastViewmodel, std::vector<dalias_t>& viewmodelList, std::vector<LoadedColormappedTexture>& textures, VkDeviceSize& stagingBufferSize);
	void SetupLoadedLightmap(AppState& appState, dsurface_t& surface, LoadedLightmap& loadedLightmap, VkDeviceSize& stagingBufferSize);
	void SetupLoadedSharedMemoryTexture(AppState& appState, LoadedSharedMemoryTexture& sharedMemoryTexture);
	VkDeviceSize GetStagingBufferSize(AppState& appState, View& view);
	void LoadStagingBuffer(AppState& appState, int matrixIndex, Buffer* stagingBuffer);
	void FillAliasTextures(AppState& appState, LoadedColormappedTexture& loadedTexture, dalias_t& alias);
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer);
	void LoadRemainingBuffers(AppState& appState);
	void SetPushConstants(dsurface_t& surface, float pushConstants[]);
	void SetPushConstants(dturbulent_t& turbulent, float pushConstants[]);
	void SetPushConstants(dalias_t& alias, float pushConstants[]);
	VkDescriptorSet GetDescriptorSet(AppState& appState, SharedMemoryTexture* texture, CachedPipelineDescriptorResources& resources, VkDescriptorImageInfo textureInfo[], VkWriteDescriptorSet writes[]);
	VkDescriptorSet GetDescriptorSet(AppState& appState, Texture* texture, CachedPipelineDescriptorResources& resources, VkDescriptorImageInfo textureInfo[], VkWriteDescriptorSet writes[]);
	void Render(AppState& appState);
};
