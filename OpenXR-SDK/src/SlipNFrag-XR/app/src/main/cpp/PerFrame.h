#pragma once

#include "CachedBuffers.h"
#include "CachedTextures.h"
#include "UpdatablePipelineDescriptorResources.h"
#include "SurfaceRotatedPushConstants.h"

struct PerFrame
{
	std::vector<int> inFlight;
	CachedBuffers cachedVertices;
	CachedBuffers cachedAttributes;
	CachedBuffers cachedIndices8;
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
	Buffer* indices8;
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

	void Reset(AppState& appState);
	static void SetPushConstants(const LoadedSurfaceRotated& loaded, SurfaceRotatedPushConstants& pushConstants);
	static void SetPushConstants(const LoadedAlias& alias, float pushConstants[]);
	void Render(AppState& appState, VkCommandBuffer commandBuffer);
};
