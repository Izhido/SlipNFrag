#pragma once

#include <vulkan/vulkan.h>
#include "PipelineWithLoaded.h"
#include "CachedSharedMemoryBuffers.h"
#include "CachedIndexBuffers.h"
#include "AliasVertices.h"
#include "LoadedSurface2Textures.h"
#include "LoadedSurfaceRotated2Textures.h"
#include "LoadedSprite.h"
#include "LoadedAlias.h"
#include "LoadedSky.h"
#include "CachedLightmaps.h"
#include "CachedSharedMemoryTextures.h"
#include "SurfaceTexture.h"
#include "Skybox.h"
#include <common/xr_linear.h>
#include "SortedSurfaceLightmap.h"
#include "UsedInSharedMemory.h"
#include "PerFrame.h"
#include "SortedSurfaces.h"

struct AppState;

struct Scene
{
	bool created;
	VkDescriptorSetLayout singleBufferLayout;
	VkDescriptorSetLayout doubleBufferLayout;
	VkDescriptorSetLayout twoBuffersAndImageLayout;
	VkDescriptorSetLayout singleImageLayout;
	PipelineWithLoaded<LoadedSurface> surfaces;
	PipelineWithLoaded<LoadedSurface2Textures> surfacesRGBA;
	PipelineWithLoaded<LoadedSurface> surfacesRGBANoGlow;
	PipelineWithLoaded<LoadedSurfaceRotated> surfacesRotated;
	PipelineWithLoaded<LoadedSurfaceRotated2Textures> surfacesRotatedRGBA;
	PipelineWithLoaded<LoadedSurfaceRotated> surfacesRotatedRGBANoGlow;
	PipelineWithLoaded<LoadedSurface> fences;
	PipelineWithLoaded<LoadedSurface2Textures> fencesRGBA;
	PipelineWithLoaded<LoadedSurface> fencesRGBANoGlow;
	PipelineWithLoaded<LoadedSurfaceRotated> fencesRotated;
	PipelineWithLoaded<LoadedSurfaceRotated2Textures> fencesRotatedRGBA;
	PipelineWithLoaded<LoadedSurfaceRotated> fencesRotatedRGBANoGlow;
	PipelineWithLoaded<LoadedTurbulent> turbulent;
	PipelineWithLoaded<LoadedTurbulent> turbulentRGBA;
	PipelineWithLoaded<LoadedSurface> turbulentLit;
	PipelineWithLoaded<LoadedSurface> turbulentLitRGBA;
	PipelineWithLoaded<LoadedSprite> sprites;
	PipelineWithLoaded<LoadedAlias> alias;
	PipelineWithLoaded<LoadedAlias> viewmodel;
	Pipeline particle;
	Pipeline colored;
	Pipeline sky;
	Pipeline skyRGBA;
	Pipeline floor;
	Pipeline floorStrip;
	int hostClearCount;
	CachedSharedMemoryBuffers buffers;
	CachedIndexBuffers indexBuffers;
	std::unordered_map<void*, std::vector<float>> surfaceVertexCache;
	std::unordered_map<void*, AliasVertices> aliasVertexCache;
	std::unordered_map<void*, IndexBuffer> aliasIndexCache;
	int lastParticle;
	int lastColoredIndex8;
	int lastColoredIndex16;
	int lastColoredIndex32;
	int lastSky;
	int lastSkyRGBA;
	LoadedSky loadedSky;
	LoadedSky loadedSkyRGBA;
	std::unordered_set<LightmapTexture*> lightmapTexturesInUse;
	std::vector<LightmapTexture*> lightmapTextures;
	LightmapTexture* deletedLightmapTextures;
	CachedLightmaps lightmaps;
	std::vector<CachedSharedMemoryTextures> surfaceTextures;
	std::vector<CachedSharedMemoryTextures> surfaceRGBATextures;
	CachedSharedMemoryTextures textures;
	std::unordered_map<void*, SurfaceTexture> surfaceTextureCache;
	std::unordered_map<void*, SharedMemoryTexture*> spriteCache;
	std::unordered_map<void*, SharedMemoryTexture*> aliasTextureCache;
	Texture floorTexture;
	Texture controllerTexture;
	std::vector<VkSampler> samplers;
	VkSampler lightmapSampler;
	std::unordered_map<uint32_t, std::list<UsedInSharedMemory>> latestMemory;
	SharedMemoryBuffer* latestIndexBuffer8;
	VkDeviceSize usedInLatestIndexBuffer8;
	SharedMemoryBuffer* latestIndexBuffer16;
	VkDeviceSize usedInLatestIndexBuffer16;
	SharedMemoryBuffer* latestIndexBuffer32;
	VkDeviceSize usedInLatestIndexBuffer32;
	DescriptorSets* latestTextureDescriptorSets;
	Skybox* previousSkyboxes;
	Skybox* skybox;
	VkDeviceSize floorVerticesSize;
	VkDeviceSize controllerVerticesSize;
	VkDeviceSize texturedVerticesSize;
	VkDeviceSize particlePositionsSize;
	VkDeviceSize coloredVerticesSize;
	VkDeviceSize verticesSize;
	VkDeviceSize floorAttributesSize;
	VkDeviceSize controllerAttributesSize;
	VkDeviceSize texturedAttributesSize;
	VkDeviceSize colormappedLightsSize;
	VkDeviceSize attributesSize;
	VkDeviceSize particleColorsSize;
	VkDeviceSize coloredColorsSize;
	VkDeviceSize colorsSize;
	VkDeviceSize floorIndicesSize;
	VkDeviceSize controllerIndicesSize;
	VkDeviceSize coloredIndices8Size;
	VkDeviceSize coloredIndices16Size;
	VkDeviceSize indices8Size;
	VkDeviceSize indices16Size;
	VkDeviceSize indices32Size;
	VkDeviceSize sortedVerticesSize;
	VkDeviceSize sortedAttributesSize;
	VkDeviceSize sortedIndicesCount;
	VkDeviceSize sortedIndices16Size;
	VkDeviceSize sortedIndices32Size;
	VkDeviceSize sortedSurfaceRGBAVerticesBase;
	VkDeviceSize sortedSurfaceRGBAAttributesBase;
	VkDeviceSize sortedSurfaceRGBAIndicesBase;
	VkDeviceSize sortedSurfaceRGBANoGlowVerticesBase;
	VkDeviceSize sortedSurfaceRGBANoGlowAttributesBase;
	VkDeviceSize sortedSurfaceRGBANoGlowIndicesBase;
	VkDeviceSize sortedSurfaceRotatedVerticesBase;
	VkDeviceSize sortedSurfaceRotatedAttributesBase;
	VkDeviceSize sortedSurfaceRotatedIndicesBase;
	VkDeviceSize sortedSurfaceRotatedRGBAVerticesBase;
	VkDeviceSize sortedSurfaceRotatedRGBAAttributesBase;
	VkDeviceSize sortedSurfaceRotatedRGBAIndicesBase;
	VkDeviceSize sortedSurfaceRotatedRGBANoGlowVerticesBase;
	VkDeviceSize sortedSurfaceRotatedRGBANoGlowAttributesBase;
	VkDeviceSize sortedSurfaceRotatedRGBANoGlowIndicesBase;
	VkDeviceSize sortedFenceVerticesBase;
	VkDeviceSize sortedFenceAttributesBase;
	VkDeviceSize sortedFenceIndicesBase;
	VkDeviceSize sortedFenceRGBAVerticesBase;
	VkDeviceSize sortedFenceRGBAAttributesBase;
	VkDeviceSize sortedFenceRGBAIndicesBase;
	VkDeviceSize sortedFenceRGBANoGlowVerticesBase;
	VkDeviceSize sortedFenceRGBANoGlowAttributesBase;
	VkDeviceSize sortedFenceRGBANoGlowIndicesBase;
	VkDeviceSize sortedFenceRotatedVerticesBase;
	VkDeviceSize sortedFenceRotatedAttributesBase;
	VkDeviceSize sortedFenceRotatedIndicesBase;
	VkDeviceSize sortedFenceRotatedRGBAVerticesBase;
	VkDeviceSize sortedFenceRotatedRGBAAttributesBase;
	VkDeviceSize sortedFenceRotatedRGBAIndicesBase;
	VkDeviceSize sortedFenceRotatedRGBANoGlowVerticesBase;
	VkDeviceSize sortedFenceRotatedRGBANoGlowAttributesBase;
	VkDeviceSize sortedFenceRotatedRGBANoGlowIndicesBase;
	VkDeviceSize sortedTurbulentVerticesBase;
	VkDeviceSize sortedTurbulentAttributesBase;
	VkDeviceSize sortedTurbulentIndicesBase;
	VkDeviceSize sortedTurbulentRGBAVerticesBase;
	VkDeviceSize sortedTurbulentRGBAAttributesBase;
	VkDeviceSize sortedTurbulentRGBAIndicesBase;
	VkDeviceSize sortedTurbulentLitVerticesBase;
	VkDeviceSize sortedTurbulentLitAttributesBase;
	VkDeviceSize sortedTurbulentLitIndicesBase;
	VkDeviceSize sortedTurbulentLitRGBAVerticesBase;
	VkDeviceSize sortedTurbulentLitRGBAAttributesBase;
	VkDeviceSize sortedTurbulentLitRGBAIndicesBase;
	VkDeviceSize paletteSize;
	VkDeviceSize colormapSize;
	StagingBuffer stagingBuffer;
	void* previousTexture;
	void* previousGlowTexture;
	void* previousApverts;
	SharedMemoryBuffer* previousVertexBuffer;
	SharedMemoryBuffer* previousTexCoordsBuffer;
	SharedMemoryTexture* previousSharedMemoryTexture;
	int previousSharedMemoryTextureIndex;
	SharedMemoryTexture* previousGlowSharedMemoryTexture;
	int previousGlowSharedMemoryTextureIndex;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	SortedSurfaces sorted;

	static void CopyImage(AppState& appState, unsigned char* source, uint32_t* target, int width, int height);
	static void AddBorder(AppState& appState, std::vector<uint32_t>& target);
	void Create(AppState& appState, VkCommandBufferAllocateInfo& commandBufferAllocateInfo, VkCommandBuffer& setupCommandBuffer, VkCommandBufferBeginInfo& commandBufferBeginInfo, VkSubmitInfo& setupSubmitInfo, struct android_app* app);
	static void CreateShader(AppState& appState, struct android_app* app, const char* filename, VkShaderModule* shaderModule);
	void Initialize();
	void AddToBufferBarrier(VkBuffer buffer);
	static VkDeviceSize GetAllocatedFor(int width, int height);
	void GetStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacewithglow_t& surface, LoadedSurface2Textures& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacerotatedwithglow_t& surface, LoadedSurfaceRotated2Textures& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dspritedata_t& sprite, LoadedSprite& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dalias_t& alias, LoadedAlias& loaded, Texture* host_colormap, VkDeviceSize& size);
	VkDeviceSize GetStagingBufferSize(AppState& appState, PerFrame& perFrame);
	void Reset();
};
