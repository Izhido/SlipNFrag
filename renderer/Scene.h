#pragma once

#include <vulkan/vulkan.h>
#include "PipelineWithSorted.h"
#include "PipelineWithLoaded.h"
#include "CachedSharedMemoryBuffers.h"
#include "CachedIndexBuffers.h"
#include "AliasVertices.h"
#include "LoadedSurfaceRotatedColoredLights.h"
#include "LoadedSurfaceRotated2Textures.h"
#include "LoadedSurfaceRotated2TexturesColoredLights.h"
#include "LoadedTurbulentRotated.h"
#include "LoadedSprite.h"
#include "LoadedAlias.h"
#include "LoadedSky.h"
#include "CachedLightmaps.h"
#include "CachedLightmapsRGBA.h"
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
    VkDescriptorSetLayout singleStorageBufferLayout;
	VkDescriptorSetLayout singleBufferLayout;
	VkDescriptorSetLayout doubleBufferLayout;
	VkDescriptorSetLayout twoBuffersAndImageLayout;
	VkDescriptorSetLayout singleImageLayout;
	PipelineWithSorted<LoadedSurface, SortedSurfaceLightmap> surfaces;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceLightmap> surfacesColoredLights;
	PipelineWithSorted<LoadedSurface2Textures, SortedSurface2TexturesLightmap> surfacesRGBA;
	PipelineWithSorted<LoadedSurface2TexturesColoredLights, SortedSurface2TexturesLightmap> surfacesRGBAColoredLights;
	PipelineWithSorted<LoadedSurface, SortedSurfaceLightmap> surfacesRGBANoGlow;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceLightmap> surfacesRGBANoGlowColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceLightmap> surfacesRotated;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceLightmap> surfacesRotatedColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated2Textures, SortedSurface2TexturesLightmap> surfacesRotatedRGBA;
	PipelineWithSorted<LoadedSurfaceRotated2TexturesColoredLights, SortedSurface2TexturesLightmap> surfacesRotatedRGBAColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceLightmap> surfacesRotatedRGBANoGlow;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceLightmap> surfacesRotatedRGBANoGlowColoredLights;
	PipelineWithSorted<LoadedSurface, SortedSurfaceLightmap> fences;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceLightmap> fencesColoredLights;
	PipelineWithSorted<LoadedSurface2Textures, SortedSurface2TexturesLightmap> fencesRGBA;
	PipelineWithSorted<LoadedSurface2TexturesColoredLights, SortedSurface2TexturesLightmap> fencesRGBAColoredLights;
	PipelineWithSorted<LoadedSurface, SortedSurfaceLightmap> fencesRGBANoGlow;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceLightmap> fencesRGBANoGlowColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceLightmap> fencesRotated;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceLightmap> fencesRotatedColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated2Textures, SortedSurface2TexturesLightmap> fencesRotatedRGBA;
	PipelineWithSorted<LoadedSurfaceRotated2TexturesColoredLights, SortedSurface2TexturesLightmap> fencesRotatedRGBAColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceLightmap> fencesRotatedRGBANoGlow;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceLightmap> fencesRotatedRGBANoGlowColoredLights;
	PipelineWithSorted<LoadedTurbulent, SortedSurfaceTexture> turbulent;
	PipelineWithSorted<LoadedTurbulent, SortedSurfaceTexture> turbulentRGBA;
	PipelineWithSorted<LoadedSurface, SortedSurfaceLightmap> turbulentLit;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceLightmap> turbulentColoredLights;
	PipelineWithSorted<LoadedSurface, SortedSurfaceLightmap> turbulentRGBALit;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceLightmap> turbulentRGBAColoredLights;
	PipelineWithSorted<LoadedTurbulentRotated, SortedSurfaceTexture> turbulentRotated;
	PipelineWithSorted<LoadedTurbulentRotated, SortedSurfaceTexture> turbulentRotatedRGBA;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceLightmap> turbulentRotatedLit;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceLightmap> turbulentRotatedColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceLightmap> turbulentRotatedRGBALit;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceLightmap> turbulentRotatedRGBAColoredLights;
	PipelineWithLoaded<LoadedSprite> sprites;
	PipelineWithLoaded<LoadedAlias> alias;
	PipelineWithLoaded<LoadedAlias> viewmodels;
	Pipeline particle;
	Pipeline colored;
	Pipeline sky;
	Pipeline skyRGBA;
	Pipeline floor;
	Pipeline floorStrip;
	int hostClearCount;
	SharedMemoryBuffer* palette;
	SharedMemoryBuffer* neutralPalette;
	SharedMemoryBuffer* oldPalettes;
	int paletteChangedFrame;
	CachedSharedMemoryBuffers buffers;
	CachedIndexBuffers indexBuffers;
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
	std::unordered_set<LightmapRGBATexture*> lightmapRGBATexturesInUse;
	std::vector<LightmapRGBATexture*> lightmapRGBATextures;
	LightmapRGBATexture* deletedLightmapRGBATextures;
	CachedLightmapsRGBA lightmapsRGBA;
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
	unsigned int paletteData[256];
	std::vector<unsigned char> screenData;
	std::vector<unsigned char> consoleData;

	static void CopyImage(AppState& appState, unsigned char* source, uint32_t* target, int width, int height);
	static void AddBorder(AppState& appState, std::vector<uint32_t>& target);
	void Create(AppState& appState, VkCommandBuffer& setupCommandBuffer, VkCommandBufferBeginInfo& commandBufferBeginInfo, VkSubmitInfo& setupSubmitInfo);
	static void CreateShader(AppState& appState, const char* filename, VkShaderModule* shaderModule);
	void Initialize();
	void AddSampler(AppState& appState, uint32_t mipCount);
	void AddToVertexInputBarriers(VkBuffer buffer, VkAccessFlags flags);
    void AddToVertexShaderBarriers(VkBuffer buffer, VkAccessFlags flags);
	static VkDeviceSize GetAllocatedFor(int width, int height);
	void GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedLightmap& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedLightmapRGBA& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurfaceColoredLights& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacewithglow_t& surface, LoadedSurface2Textures& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacewithglow_t& surface, LoadedSurface2TexturesColoredLights& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurface_t& surface, LoadedSurfaceColoredLights& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotatedColoredLights& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacerotatedwithglow_t& surface, LoadedSurfaceRotated2Textures& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacerotatedwithglow_t& surface, LoadedSurfaceRotated2TexturesColoredLights& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotatedColoredLights& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dturbulentrotated_t& turbulent, LoadedTurbulentRotated& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dspritedata_t& sprite, LoadedSprite& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dalias_t& alias, LoadedAlias& loaded, Texture* host_colormap, VkDeviceSize& size);
	VkDeviceSize GetStagingBufferSize(AppState& appState, PerFrame& perFrame);
	void Reset();
};
