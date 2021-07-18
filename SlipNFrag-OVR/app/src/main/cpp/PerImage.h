#pragma once

#include "CachedBuffers.h"
#include "CachedTextures.h"
#include "PipelineDescriptorResources.h"
#include "UpdatablePipelineDescriptorResources.h"
#include "vid_ovr.h"
#include "d_lists.h"
#include "LoadedSurface.h"
#include "LoadedTurbulent.h"
#include "LoadedSharedMemoryTexCoordsBuffer.h"
#include "LoadedTexture.h"
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
	void GetStagingBufferSize(AppState& appState, View& view, dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, View& view, dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size);
	void GetAliasVerticesStagingBufferSize(AppState& appState, dalias_t& alias, LoadedSharedMemoryBuffer& loadedVertices, LoadedSharedMemoryTexCoordsBuffer& loadedTexCoords, VkDeviceSize& stagingBufferSize);
	void GetAliasColormapStagingBufferSize(AppState& appState, int lastAlias, std::vector<dalias_t>& aliasList, std::vector<LoadedColormappedTexture>& textures, VkDeviceSize& stagingBufferSize);
	void GetAliasStagingBufferSize(AppState& appState, int lastAlias, std::vector<dalias_t>& aliasList, std::vector<LoadedColormappedTexture>& textures, VkDeviceSize& stagingBufferSize);
	void GetViewmodelStagingBufferSize(AppState& appState, int lastViewmodel, std::vector<dalias_t>& viewmodelList, std::vector<LoadedColormappedTexture>& textures, VkDeviceSize& stagingBufferSize);
	VkDeviceSize GetStagingBufferSize(AppState& appState, View& view);
	void LoadStagingBuffer(AppState& appState, int matrixIndex, Buffer* stagingBuffer);
	void FillAliasTextures(AppState& appState, LoadedColormappedTexture& loadedTexture, dalias_t& alias);
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer);
	void LoadRemainingBuffers(AppState& appState);
	void SetPushConstants(LoadedSurface& loaded, float pushConstants[]);
	void SetPushConstants(LoadedTurbulent& loaded, float pushConstants[]);
	void SetPushConstants(dalias_t& alias, float pushConstants[]);
	void Render(AppState& appState);
};
