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
	PipelineWithSorted<LoadedSurface, SortedSurfaceLightmapsWithTextures> surfaces;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceLightmapsWithTextures> surfacesColoredLights;
	PipelineWithSorted<LoadedSurface2Textures, SortedSurfaceLightmapsWith2Textures> surfacesRGBA;
	PipelineWithSorted<LoadedSurface2TexturesColoredLights, SortedSurfaceLightmapsWith2Textures> surfacesRGBAColoredLights;
	PipelineWithSorted<LoadedSurface, SortedSurfaceLightmapsWithTextures> surfacesRGBANoGlow;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceLightmapsWithTextures> surfacesRGBANoGlowColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceLightmapsWithTextures> surfacesRotated;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceLightmapsWithTextures> surfacesRotatedColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated2Textures, SortedSurfaceLightmapsWith2Textures> surfacesRotatedRGBA;
	PipelineWithSorted<LoadedSurfaceRotated2TexturesColoredLights, SortedSurfaceLightmapsWith2Textures> surfacesRotatedRGBAColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceLightmapsWithTextures> surfacesRotatedRGBANoGlow;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceLightmapsWithTextures> surfacesRotatedRGBANoGlowColoredLights;
	PipelineWithSorted<LoadedSurface, SortedSurfaceLightmapsWithTextures> fences;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceLightmapsWithTextures> fencesColoredLights;
	PipelineWithSorted<LoadedSurface2Textures, SortedSurfaceLightmapsWith2Textures> fencesRGBA;
	PipelineWithSorted<LoadedSurface2TexturesColoredLights, SortedSurfaceLightmapsWith2Textures> fencesRGBAColoredLights;
	PipelineWithSorted<LoadedSurface, SortedSurfaceLightmapsWithTextures> fencesRGBANoGlow;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceLightmapsWithTextures> fencesRGBANoGlowColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceLightmapsWithTextures> fencesRotated;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceLightmapsWithTextures> fencesRotatedColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated2Textures, SortedSurfaceLightmapsWith2Textures> fencesRotatedRGBA;
	PipelineWithSorted<LoadedSurfaceRotated2TexturesColoredLights, SortedSurfaceLightmapsWith2Textures> fencesRotatedRGBAColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceLightmapsWithTextures> fencesRotatedRGBANoGlow;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceLightmapsWithTextures> fencesRotatedRGBANoGlowColoredLights;
	PipelineWithSorted<LoadedTurbulent, SortedSurfaceTextures> turbulent;
	PipelineWithSorted<LoadedTurbulent, SortedSurfaceTextures> turbulentRGBA;
	PipelineWithSorted<LoadedSurface, SortedSurfaceLightmapsWithTextures> turbulentLit;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceLightmapsWithTextures> turbulentColoredLights;
	PipelineWithSorted<LoadedSurface, SortedSurfaceLightmapsWithTextures> turbulentRGBALit;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceLightmapsWithTextures> turbulentRGBAColoredLights;
	PipelineWithSorted<LoadedTurbulentRotated, SortedSurfaceTextures> turbulentRotated;
	PipelineWithSorted<LoadedTurbulentRotated, SortedSurfaceTextures> turbulentRotatedRGBA;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceLightmapsWithTextures> turbulentRotatedLit;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceLightmapsWithTextures> turbulentRotatedColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceLightmapsWithTextures> turbulentRotatedRGBALit;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceLightmapsWithTextures> turbulentRotatedRGBAColoredLights;
	PipelineWithLoaded<LoadedSprite> sprites;
	PipelineWithLoaded<LoadedAlias> alias;
	PipelineWithLoaded<LoadedAlias> viewmodels;
	Pipeline particle;
	Pipeline colored;
	Pipeline sky;
	Pipeline skyRGBA;
	Pipeline floor;
	Pipeline floorStrip;
    std::vector<VkBuffer> matricesBuffers;
    VkDeviceSize matricesBufferSize;
    VkDeviceMemory matricesMemory;
    std::vector<VkBuffer> paletteBuffers;
    std::vector<VkBuffer> neutralPaletteBuffers;
    VkDeviceSize paletteBufferSize;
    VkDeviceMemory paletteMemory;
    Texture colormap;
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
	std::vector<std::vector<LightmapTexture*>> lightmapTextures;
	LightmapTexture* deletedLightmapTextures;
	CachedLightmaps lightmaps;
	std::unordered_set<LightmapRGBATexture*> lightmapRGBATexturesInUse;
	std::vector<std::vector<LightmapRGBATexture*>> lightmapRGBATextures;
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
    VkDeviceSize sortedVerticesCount;
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
	unsigned int paletteData[256];
	std::vector<unsigned char> screenData;
	std::vector<unsigned char> consoleData;

	static void CopyImage(AppState& appState, unsigned char* source, uint32_t* target, int width, int height);
	static void AddBorder(AppState& appState, std::vector<uint32_t>& target);
	void Create(AppState& appState);
	static void CreateShader(AppState& appState, const char* filename, VkShaderModule* shaderModule);
	void Initialize();
	void AddSampler(AppState& appState, uint32_t mipCount);
	void AddToVertexInputBarriers(VkBuffer buffer, VkAccessFlags flags);
    void AddToVertexShaderBarriers(VkBuffer buffer, VkAccessFlags flags);
	static VkDeviceSize GetAllocatedFor(int width, int height);
    void CacheVertices(LoadedTurbulent& loaded);
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
