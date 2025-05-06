#pragma once

#include <vulkan/vulkan.h>
#include "PipelineWithSorted.h"
#include "PipelineWithLoaded.h"
#include "CachedAliasBuffers.h"
#include "CachedIndexBuffers.h"
#include "PerSurfaceData.h"
#include "AliasVertices.h"
#include "LoadedSurfaceRotatedColoredLights.h"
#include "LoadedSurfaceRotated2Textures.h"
#include "LoadedSurfaceRotated2TexturesColoredLights.h"
#include "LoadedTurbulentRotated.h"
#include "LoadedSprite.h"
#include "LoadedAlias.h"
#include "LoadedSky.h"
#include "LightmapBuffersPerTexture.h"
#include "LightmapRGBBuffersPerTexture.h"
#include "LightmapsToDelete.h"
#include "LightmapsRGBToDelete.h"
#include "CachedSharedMemoryTextures.h"
#include "SurfaceTexture.h"
#include "Skybox.h"
#include <common/xr_linear.h>
#include "UsedInSharedMemory.h"
#include "PerFrame.h"
#include "SortedSurfaces.h"
#include "LightmapChain.h"
#include "LightmapRGBChain.h"

struct AppState;

struct Scene
{
	bool created;
    VkDescriptorSetLayout singleStorageBufferLayout;
	VkDescriptorSetLayout singleFragmentStorageBufferLayout;
	VkDescriptorSetLayout singleBufferLayout;
	VkDescriptorSetLayout doubleBufferLayout;
	VkDescriptorSetLayout twoBuffersAndImageLayout;
	VkDescriptorSetLayout singleImageLayout;
	int frameCount;
	PipelineWithSorted<LoadedSurface, SortedSurfaceTexturesWithLightmaps> surfaces;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceTexturesWithLightmaps> surfacesColoredLights;
	PipelineWithSorted<LoadedSurface2Textures, SortedSurfaceTexturePairsWithLightmaps> surfacesRGBA;
	PipelineWithSorted<LoadedSurface2TexturesColoredLights, SortedSurfaceTexturePairsWithLightmaps> surfacesRGBAColoredLights;
	PipelineWithSorted<LoadedSurface, SortedSurfaceTexturesWithLightmaps> surfacesRGBANoGlow;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceTexturesWithLightmaps> surfacesRGBANoGlowColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceTexturesWithLightmaps> surfacesRotated;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceTexturesWithLightmaps> surfacesRotatedColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated2Textures, SortedSurfaceTexturePairsWithLightmaps> surfacesRotatedRGBA;
	PipelineWithSorted<LoadedSurfaceRotated2TexturesColoredLights, SortedSurfaceTexturePairsWithLightmaps> surfacesRotatedRGBAColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceTexturesWithLightmaps> surfacesRotatedRGBANoGlow;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceTexturesWithLightmaps> surfacesRotatedRGBANoGlowColoredLights;
	PipelineWithSorted<LoadedSurface, SortedSurfaceTexturesWithLightmaps> fences;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceTexturesWithLightmaps> fencesColoredLights;
	PipelineWithSorted<LoadedSurface2Textures, SortedSurfaceTexturePairsWithLightmaps> fencesRGBA;
	PipelineWithSorted<LoadedSurface2TexturesColoredLights, SortedSurfaceTexturePairsWithLightmaps> fencesRGBAColoredLights;
	PipelineWithSorted<LoadedSurface, SortedSurfaceTexturesWithLightmaps> fencesRGBANoGlow;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceTexturesWithLightmaps> fencesRGBANoGlowColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceTexturesWithLightmaps> fencesRotated;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceTexturesWithLightmaps> fencesRotatedColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated2Textures, SortedSurfaceTexturePairsWithLightmaps> fencesRotatedRGBA;
	PipelineWithSorted<LoadedSurfaceRotated2TexturesColoredLights, SortedSurfaceTexturePairsWithLightmaps> fencesRotatedRGBAColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceTexturesWithLightmaps> fencesRotatedRGBANoGlow;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceTexturesWithLightmaps> fencesRotatedRGBANoGlowColoredLights;
	PipelineWithSorted<LoadedTurbulent, SortedSurfaceTextures> turbulent;
	PipelineWithSorted<LoadedTurbulent, SortedSurfaceTextures> turbulentRGBA;
	PipelineWithSorted<LoadedSurface, SortedSurfaceTexturesWithLightmaps> turbulentLit;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceTexturesWithLightmaps> turbulentColoredLights;
	PipelineWithSorted<LoadedSurface, SortedSurfaceTexturesWithLightmaps> turbulentRGBALit;
	PipelineWithSorted<LoadedSurfaceColoredLights, SortedSurfaceTexturesWithLightmaps> turbulentRGBAColoredLights;
	PipelineWithSorted<LoadedTurbulentRotated, SortedSurfaceTextures> turbulentRotated;
	PipelineWithSorted<LoadedTurbulentRotated, SortedSurfaceTextures> turbulentRotatedRGBA;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceTexturesWithLightmaps> turbulentRotatedLit;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceTexturesWithLightmaps> turbulentRotatedColoredLights;
	PipelineWithSorted<LoadedSurfaceRotated, SortedSurfaceTexturesWithLightmaps> turbulentRotatedRGBALit;
	PipelineWithSorted<LoadedSurfaceRotatedColoredLights, SortedSurfaceTexturesWithLightmaps> turbulentRotatedRGBAColoredLights;
	PipelineWithSorted<LoadedSprite, SortedSurfaceTextures> sprites;
	PipelineWithLoaded<LoadedAlias> alias;
	PipelineWithLoaded<LoadedAlias> aliasAlpha;
	PipelineWithLoaded<LoadedAliasColoredLights> aliasColoredLights;
	PipelineWithLoaded<LoadedAliasColoredLights> aliasAlphaColoredLights;
	PipelineWithLoaded<LoadedAlias> aliasHoley;
	PipelineWithLoaded<LoadedAlias> aliasHoleyAlpha;
	PipelineWithLoaded<LoadedAliasColoredLights> aliasHoleyColoredLights;
	PipelineWithLoaded<LoadedAliasColoredLights> aliasHoleyAlphaColoredLights;
	PipelineWithLoaded<LoadedAlias> viewmodels;
	PipelineWithLoaded<LoadedAliasColoredLights> viewmodelsColoredLights;
	PipelineWithLoaded<LoadedAlias> viewmodelsHoley;
	PipelineWithLoaded<LoadedAliasColoredLights> viewmodelsHoleyColoredLights;
	Pipeline particles;
	Pipeline colored;
	Pipeline sky;
	Pipeline skyRGBA;
	Pipeline controllers;
	Pipeline floor;
    std::vector<VkBuffer> paletteBuffers;
    std::vector<VkBuffer> neutralPaletteBuffers;
    VkDeviceSize paletteBufferSize;
    VkDeviceMemory paletteMemory;
    Texture colormap;
	int hostClearCount;
	CachedAliasBuffers aliasBuffers;
	CachedIndexBuffers indexBuffers;
	std::unordered_map<void*, PerSurfaceData> perSurfaceCache;
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
	std::unordered_map<void*, LightmapBuffersPerTexture> lightmapBuffers;
	LightmapsToDelete lightmapsToDelete;
	std::vector<LightmapChain> lightmapChains;
	std::unordered_map<void*, size_t> lightmapChainTexturesInUse;
	std::unordered_map<void*, LightmapRGBBuffersPerTexture> lightmapRGBBuffers;
	LightmapsRGBToDelete lightmapsRGBToDelete;
	std::vector<LightmapRGBChain> lightmapRGBChains;
	std::unordered_map<void*, size_t> lightmapRGBChainTexturesInUse;
	std::vector<CachedSharedMemoryTextures> surfaceTextures;
	std::vector<CachedSharedMemoryTextures> surfaceRGBATextures;
	CachedSharedMemoryTextures textures;
	std::unordered_map<void*, SurfaceTexture> surfaceTextureCache;
	std::unordered_map<void*, SharedMemoryTexture*> spriteCache;
	std::unordered_map<void*, SharedMemoryTexture*> aliasTextureCache;
	Texture floorTexture;
	Texture controllerTexture;
	VkSampler sampler;
	VkSampler lightmapSampler;
	std::vector<std::list<UsedInSharedMemory>> latestMemory;
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
	VkDeviceSize skyVerticesSize;
	VkDeviceSize coloredVerticesSize;
	VkDeviceSize verticesSize;
	VkDeviceSize floorAttributesSize;
	VkDeviceSize controllerAttributesSize;
	VkDeviceSize skyAttributesSize;
	VkDeviceSize aliasAttributesSize;
	VkDeviceSize attributesSize;
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
	void* previousApverts;
	SharedMemoryBuffer* previousVertexBuffer;
	SharedMemoryBuffer* previousTexCoordsBuffer;
	SharedMemoryTexture* previousSharedMemoryTexture;
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	unsigned int paletteData[256];
	std::vector<unsigned char> screenData;
	std::vector<unsigned char> consoleData;

	static void CopyImage(AppState& appState, unsigned char* source, uint32_t* target, int width, int height);
	static void AddBorder(AppState& appState, std::vector<uint32_t>& target);
	void Create(AppState& appState);
	static void CreateShader(AppState& appState, const char* filename, VkShaderModule* shaderModule);
	void Initialize();
	void AddToVertexInputBarriers(VkBuffer buffer, VkAccessFlags flags);
    void AddToVertexShaderBarriers(VkBuffer buffer, VkAccessFlags flags);
	static VkDeviceSize GetAllocatedFor(int width, int height);
	static uint32_t GetLayerCountFor(int width, int height);
    static void CacheVertices(PerSurfaceData& perSurface, LoadedTurbulent& loaded);
	void GetStagingBufferSize(AppState& appState, const dsurface_t& surface, PerSurfaceData& perSurface, LoadedLightmap& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurface_t& surface, PerSurfaceData& perSurface, LoadedLightmapRGB& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, PerSurfaceData& perSurface, LoadedTurbulent& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dturbulent_t& turbulent, PerSurfaceData& perSurface, LoadedTurbulent& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurface_t& surface, PerSurfaceData& perSurface, LoadedSurface& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurface_t& surface, PerSurfaceData& perSurface, LoadedSurfaceColoredLights& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacewithglow_t& surface, PerSurfaceData& perSurface, LoadedSurface2Textures& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacewithglow_t& surface, PerSurfaceData& perSurface, LoadedSurface2TexturesColoredLights& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurface_t& surface, PerSurfaceData& perSurface, LoadedSurface& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurface_t& surface, PerSurfaceData& perSurface, LoadedSurfaceColoredLights& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, PerSurfaceData& perSurface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, PerSurfaceData& perSurface, LoadedSurfaceRotatedColoredLights& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacerotatedwithglow_t& surface, PerSurfaceData& perSurface, LoadedSurfaceRotated2Textures& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dsurfacerotatedwithglow_t& surface, PerSurfaceData& perSurface, LoadedSurfaceRotated2TexturesColoredLights& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurfacerotated_t& surface, PerSurfaceData& perSurface, LoadedSurfaceRotated& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurfacerotated_t& surface, PerSurfaceData& perSurface, LoadedSurfaceRotatedColoredLights& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dturbulentrotated_t& turbulent, PerSurfaceData& perSurface, LoadedTurbulentRotated& loaded, VkDeviceSize& size);
	void GetStagingBufferSizeRGBANoGlow(AppState& appState, const dturbulentrotated_t& surface, PerSurfaceData& perSurface, LoadedTurbulentRotated& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dspritedata_t& sprite, LoadedSprite& loaded, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const dalias_t& alias, LoadedAlias& loaded, Texture* host_colormap, VkDeviceSize& size);
	void GetStagingBufferSize(AppState& appState, const daliascoloredlights_t& alias, LoadedAliasColoredLights& loaded, VkDeviceSize& size);
	VkDeviceSize GetStagingBufferSize(AppState& appState, PerFrame& perFrame);
	void Reset(AppState& appState);
};
