#include "PerImage.h"
#include "AppState.h"
#include "VulkanCallWrappers.h"
#include "VrApi_Helpers.h"
#include "Constants.h"

void PerImage::Reset(AppState& appState)
{
	cachedVertices.Reset(appState);
	cachedAttributes.Reset(appState);
	cachedIndices16.Reset(appState);
	cachedIndices32.Reset(appState);
	cachedColors.Reset(appState);
	stagingBuffers.Reset(appState);
	turbulent.Reset(appState);
	colormaps.Reset(appState);
	colormapCount = 0;
	vertices = nullptr;
	attributes = nullptr;
	indices16 = nullptr;
	indices32 = nullptr;
	colors = nullptr;
}

void PerImage::GetSurfaceVertexStagingBufferSize(AppState& appState, dsurface_t& surface, LoadedSharedMemoryBuffer& loadedBuffer, VkDeviceSize& stagingBufferSize)
{
	auto vertexes = surface.vertexes;
	if (appState.Scene.previousSurfaceVertexes != vertexes)
	{
		auto entry = appState.Scene.surfaceVerticesPerModel.find(vertexes);
		if (entry == appState.Scene.surfaceVerticesPerModel.end())
		{
			loadedBuffer.size = surface.vertex_count * 3 * sizeof(float);
			loadedBuffer.buffer = new SharedMemoryBuffer();
			loadedBuffer.buffer->CreateVertexBuffer(appState, loadedBuffer.size);
			appState.Scene.surfaceVerticesPerModel.insert({ vertexes, loadedBuffer.buffer });
			stagingBufferSize += loadedBuffer.size;
			appState.Scene.vertexBuffers.MoveToFront(loadedBuffer.buffer);
			loadedBuffer.source = surface.vertexes;
			loadedBuffer.next = nullptr;
			if (appState.Scene.currentVertexListToCreate == nullptr)
			{
				appState.Scene.firstVertexListToCreate = &loadedBuffer;
			}
			else
			{
				appState.Scene.currentVertexListToCreate->next = &loadedBuffer;
			}
			appState.Scene.currentVertexListToCreate = &loadedBuffer;
		}
		else
		{
			loadedBuffer.size = 0;
			loadedBuffer.buffer = entry->second;
		}
		appState.Scene.previousSurfaceVertexes = vertexes;
		appState.Scene.previousSurfaceBuffer = loadedBuffer.buffer;
	}
	else
	{
		loadedBuffer.size = 0;
		loadedBuffer.buffer = appState.Scene.previousSurfaceBuffer;
	}
}

void PerImage::GetTurbulentVertexStagingBufferSize(AppState& appState, dturbulent_t& turbulent, LoadedSharedMemoryBuffer& loadedBuffer, VkDeviceSize& stagingBufferSize)
{
	auto vertexes = turbulent.vertexes;
	if (appState.Scene.previousSurfaceVertexes != vertexes)
	{
		auto entry = appState.Scene.surfaceVerticesPerModel.find(vertexes);
		if (entry == appState.Scene.surfaceVerticesPerModel.end())
		{
			loadedBuffer.size = turbulent.vertex_count * 3 * sizeof(float);
			loadedBuffer.buffer = new SharedMemoryBuffer();
			loadedBuffer.buffer->CreateVertexBuffer(appState, loadedBuffer.size);
			appState.Scene.surfaceVerticesPerModel.insert({ vertexes, loadedBuffer.buffer });
			stagingBufferSize += loadedBuffer.size;
			appState.Scene.vertexBuffers.MoveToFront(loadedBuffer.buffer);
			loadedBuffer.source = turbulent.vertexes;
			loadedBuffer.next = nullptr;
			if (appState.Scene.currentVertexListToCreate == nullptr)
			{
				appState.Scene.firstVertexListToCreate = &loadedBuffer;
			}
			else
			{
				appState.Scene.currentVertexListToCreate->next = &loadedBuffer;
			}
			appState.Scene.currentVertexListToCreate = &loadedBuffer;
		}
		else
		{
			loadedBuffer.size = 0;
			loadedBuffer.buffer = entry->second;
		}
		appState.Scene.previousSurfaceVertexes = vertexes;
		appState.Scene.previousSurfaceBuffer = loadedBuffer.buffer;
	}
	else
	{
		loadedBuffer.size = 0;
		loadedBuffer.buffer = appState.Scene.previousSurfaceBuffer;
	}
}

void PerImage::SetupLoadedLightmap(AppState& appState, dsurface_t& surface, LoadedLightmap& lightmap, VkDeviceSize& stagingBufferSize)
{
	lightmap.size = surface.lightmap_size * sizeof(float);
	stagingBufferSize += lightmap.size;
	lightmap.source = surface.lightmap;
	lightmap.next = nullptr;
	if (appState.Scene.currentLightmapToCreate == nullptr)
	{
		appState.Scene.firstLightmapToCreate = &lightmap;
	}
	else
	{
		appState.Scene.currentLightmapToCreate->next = &lightmap;
	}
	appState.Scene.currentLightmapToCreate = &lightmap;
}

void PerImage::SetupLoadedSharedMemoryTexture(AppState& appState, LoadedSharedMemoryTexture& sharedMemoryTexture)
{
	sharedMemoryTexture.next = nullptr;
	if (appState.Scene.currentSharedMemoryTextureToCreate == nullptr)
	{
		appState.Scene.firstSharedMemoryTextureToCreate = &sharedMemoryTexture;
	}
	else
	{
		appState.Scene.currentSharedMemoryTextureToCreate->next = &sharedMemoryTexture;
	}
	appState.Scene.currentSharedMemoryTextureToCreate = &sharedMemoryTexture;
}

void PerImage::GetSurfaceLightmapStagingBufferSize(AppState& appState, View& view, dsurface_t& surface, LoadedLightmap& loadedLightmap, VkDeviceSize& stagingBufferSize)
{
	auto entry = appState.Scene.lightmaps.find({ surface.surface, surface.entity });
	if (entry == appState.Scene.lightmaps.end())
	{
		auto lightmap = new Lightmap();
		lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		lightmap->key.first = surface.surface;
		lightmap->key.second = surface.entity;
		appState.Scene.lightmaps.insert({ lightmap->key, lightmap });
		loadedLightmap.lightmap = lightmap;
		SetupLoadedLightmap(appState, surface, loadedLightmap, stagingBufferSize);
	}
	else if (surface.created)
	{
		auto first = entry->second;
		auto available = first->unusedCount >= view.framebuffer.swapChainLength;
		if (first->next == nullptr || available)
		{
			if (available)
			{
				first->unusedCount = 0;
				loadedLightmap.lightmap = first;
			}
			else
			{
				auto lightmap = new Lightmap();
				lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				lightmap->key.first = surface.surface;
				lightmap->key.second = surface.entity;
				lightmap->next = first;
				entry->second = lightmap;
				loadedLightmap.lightmap = lightmap;
			}
		}
		else
		{
			auto found = false;
			for (auto previous = first, lightmap = first->next; lightmap != nullptr; previous = lightmap, lightmap = lightmap->next)
			{
				if (lightmap->unusedCount >= view.framebuffer.swapChainLength)
				{
					found = true;
					lightmap->unusedCount = 0;
					previous->next = lightmap->next;
					lightmap->next = first;
					entry->second = lightmap;
					loadedLightmap.lightmap = lightmap;
					break;
				}
			}
			if (!found)
			{
				auto lightmap = new Lightmap();
				lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				lightmap->key.first = surface.surface;
				lightmap->key.second = surface.entity;
				lightmap->next = entry->second;
				entry->second = lightmap;
				loadedLightmap.lightmap = lightmap;
			}
		}
		SetupLoadedLightmap(appState, surface, loadedLightmap, stagingBufferSize);
		surface.created = 0;
	}
	else
	{
		auto lightmap = entry->second;
		lightmap->unusedCount = 0;
		loadedLightmap.lightmap = lightmap;
		loadedLightmap.size = 0;
	}
}

void PerImage::GetTurbulentStagingBufferSize(AppState& appState, View& view, dturbulent_t& turbulent, LoadedTexture& loadedTexture, VkDeviceSize& stagingBufferSize)
{
	auto entry = appState.Scene.turbulentPerKey.find(turbulent.texture);
	if (hostClearCount == host_clearcount && entry != appState.Scene.turbulentPerKey.end())
	{
		loadedTexture.size = 0;
		loadedTexture.texture = entry->second;
		entry->second->unusedCount = 0;
	}
	else if (hostClearCount == host_clearcount)
	{
		Texture* texture = nullptr;
		for (auto t = &this->turbulent.oldTextures; *t != nullptr; t = &(*t)->next)
		{
			if ((*t)->key == turbulent.texture)
			{
				texture = *t;
				*t = (*t)->next;
				break;
			}
		}
		if (texture == nullptr)
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
			texture = new Texture();
			texture->Create(appState, turbulent.width, turbulent.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			texture->key = turbulent.texture;
			loadedTexture.size = turbulent.size;
			stagingBufferSize += loadedTexture.size;
		}
		else
		{
			loadedTexture.size = 0;
		}
		loadedTexture.texture = texture;
		appState.Scene.turbulentPerKey.insert({ texture->key, texture });
		this->turbulent.MoveToFront(texture);
	}
	else
	{
		auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
		auto texture = new Texture();
		texture->Create(appState, turbulent.width, turbulent.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		texture->key = turbulent.texture;
		loadedTexture.size = turbulent.size;
		stagingBufferSize += loadedTexture.size;
		this->turbulent.MoveToFront(texture);
		loadedTexture.texture = texture;
		appState.Scene.turbulentPerKey.insert({ texture->key, texture });
	}
}

void PerImage::GetAliasColormapStagingBufferSize(AppState& appState, int lastAlias, std::vector<dalias_t>& aliasList, std::vector<LoadedColormappedTexture>& textures, VkDeviceSize& stagingBufferSize)
{
	for (auto i = 0; i <= lastAlias; i++)
	{
		auto& alias = aliasList[i];
		if (alias.is_host_colormap)
		{
			textures[i].colormap.size = 0;
			textures[i].colormap.texture = host_colormap;
		}
		else
		{
			textures[i].colormap.size = 16384;
			stagingBufferSize += textures[i].colormap.size;
		}
	}
}

void PerImage::GetAliasStagingBufferSize(AppState& appState, int lastAlias, std::vector<dalias_t>& aliasList, std::vector<LoadedColormappedTexture>& textures, VkDeviceSize& stagingBufferSize)
{
	for (auto i = 0; i <= lastAlias; i++)
	{
		auto& alias = aliasList[i];
		auto entry = appState.Scene.aliasPerKey.find(alias.data);
		if (entry == appState.Scene.aliasPerKey.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(alias.width, alias.height)))) + 1;
			auto texture = new SharedMemoryTexture();
			texture->Create(appState, alias.width, alias.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			textures[i].texture.size = alias.size;
			stagingBufferSize += textures[i].texture.size;
			appState.Scene.textures.MoveToFront(texture);
			appState.Scene.aliasTextureCount++;
			appState.Scene.aliasPerKey.insert({ alias.data, texture });
			textures[i].texture.texture = texture;
		}
		else
		{
			textures[i].texture.size = 0;
			textures[i].texture.texture = entry->second;
		}
	}
	GetAliasColormapStagingBufferSize(appState, lastAlias, aliasList, textures, stagingBufferSize);
}

void PerImage::GetViewmodelStagingBufferSize(AppState& appState, int lastViewmodel, std::vector<dalias_t>& viewmodelList, std::vector<LoadedColormappedTexture>& textures, VkDeviceSize& stagingBufferSize)
{
	for (auto i = 0; i <= lastViewmodel; i++)
	{
		auto& viewmodel = viewmodelList[i];
		auto entry = appState.Scene.viewmodelsPerKey.find(viewmodel.data);
		if (entry == appState.Scene.viewmodelsPerKey.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(viewmodel.width, viewmodel.height)))) + 1;
			auto texture = new SharedMemoryTexture();
			texture->Create(appState, viewmodel.width, viewmodel.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			textures[i].texture.size = viewmodel.size;
			stagingBufferSize += textures[i].texture.size;
			appState.Scene.textures.MoveToFront(texture);
			appState.Scene.viewmodelTextureCount++;
			appState.Scene.viewmodelsPerKey.insert({ viewmodel.data, texture });
			textures[i].texture.texture = texture;
		}
		else
		{
			textures[i].texture.size = 0;
			textures[i].texture.texture = entry->second;
		}
	}
	GetAliasColormapStagingBufferSize(appState, lastViewmodel, viewmodelList, textures, stagingBufferSize);
}

VkDeviceSize PerImage::GetStagingBufferSize(AppState& appState, View& view)
{
	appState.Scene.firstVertexListToCreate = nullptr;
	appState.Scene.currentVertexListToCreate = nullptr;
	appState.Scene.previousSurfaceVertexes = nullptr;
	appState.Scene.previousSurfaceBuffer = nullptr;
	auto size = appState.Scene.matrices.size;
	if (d_lists.last_surface16 >= appState.Scene.surfaceVertex16List.size())
	{
		appState.Scene.surfaceVertex16List.resize(d_lists.last_surface16 + 1);
	}
	for (auto i = 0; i <= d_lists.last_surface16; i++)
	{
		auto& surface = d_lists.surfaces16[i];
		GetSurfaceVertexStagingBufferSize(appState, surface, appState.Scene.surfaceVertex16List[i], size);
	}
	if (d_lists.last_surface32 >= appState.Scene.surfaceVertex32List.size())
	{
		appState.Scene.surfaceVertex32List.resize(d_lists.last_surface32 + 1);
	}
	for (auto i = 0; i <= d_lists.last_surface32; i++)
	{
		auto& surface = d_lists.surfaces32[i];
		GetSurfaceVertexStagingBufferSize(appState, surface, appState.Scene.surfaceVertex32List[i], size);
	}
	if (d_lists.last_fence16 >= appState.Scene.fenceVertex16List.size())
	{
		appState.Scene.fenceVertex16List.resize(d_lists.last_fence16 + 1);
	}
	for (auto i = 0; i <= d_lists.last_fence16; i++)
	{
		auto& fence = d_lists.fences16[i];
		GetSurfaceVertexStagingBufferSize(appState, fence, appState.Scene.fenceVertex16List[i], size);
	}
	if (d_lists.last_fence32 >= appState.Scene.fenceVertex32List.size())
	{
		appState.Scene.fenceVertex32List.resize(d_lists.last_fence32 + 1);
	}
	for (auto i = 0; i <= d_lists.last_fence32; i++)
	{
		auto& fence = d_lists.fences32[i];
		GetSurfaceVertexStagingBufferSize(appState, fence, appState.Scene.fenceVertex32List[i], size);
	}
	if (d_lists.last_turbulent16 >= appState.Scene.turbulentVertex16List.size())
	{
		appState.Scene.turbulentVertex16List.resize(d_lists.last_turbulent16 + 1);
	}
	for (auto i = 0; i <= d_lists.last_turbulent16; i++)
	{
		auto& turbulent = d_lists.turbulent16[i];
		GetTurbulentVertexStagingBufferSize(appState, turbulent, appState.Scene.turbulentVertex16List[i], size);
	}
	if (d_lists.last_turbulent32 >= appState.Scene.turbulentVertex32List.size())
	{
		appState.Scene.turbulentVertex32List.resize(d_lists.last_turbulent32 + 1);
	}
	for (auto i = 0; i <= d_lists.last_turbulent32; i++)
	{
		auto& turbulent = d_lists.turbulent32[i];
		GetTurbulentVertexStagingBufferSize(appState, turbulent, appState.Scene.turbulentVertex32List[i], size);
	}
	paletteSize = 0;
	if (palette == nullptr || paletteChanged != pal_changed)
	{
		if (palette == nullptr)
		{
			palette = new Texture();
			palette->Create(appState, 256, 1, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		paletteSize = 1024;
		size += paletteSize;
		paletteChanged = pal_changed;
	}
	host_colormapSize = 0;
	if (::host_colormap.size() > 0 && host_colormap == nullptr)
	{
		host_colormap = new Texture();
		host_colormap->Create(appState, 256, 64, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		host_colormapSize = 16384;
		size += host_colormapSize;
	}
	appState.Scene.firstLightmapToCreate = nullptr;
	appState.Scene.currentLightmapToCreate = nullptr;
	if (d_lists.last_surface16 >= appState.Scene.surfaceLightmap16List.size())
	{
		appState.Scene.surfaceLightmap16List.resize(d_lists.last_surface16 + 1);
	}
	for (auto i = 0; i <= d_lists.last_surface16; i++)
	{
		auto& surface = d_lists.surfaces16[i];
		GetSurfaceLightmapStagingBufferSize(appState, view, surface, appState.Scene.surfaceLightmap16List[i], size);
	}
	if (d_lists.last_surface32 >= appState.Scene.surfaceLightmap32List.size())
	{
		appState.Scene.surfaceLightmap32List.resize(d_lists.last_surface32 + 1);
	}
	for (auto i = 0; i <= d_lists.last_surface32; i++)
	{
		auto& surface = d_lists.surfaces32[i];
		GetSurfaceLightmapStagingBufferSize(appState, view, surface, appState.Scene.surfaceLightmap32List[i], size);
	}
	if (d_lists.last_fence16 >= appState.Scene.fenceLightmap16List.size())
	{
		appState.Scene.fenceLightmap16List.resize(d_lists.last_fence16 + 1);
	}
	for (auto i = 0; i <= d_lists.last_fence16; i++)
	{
		auto& fence = d_lists.fences16[i];
		GetSurfaceLightmapStagingBufferSize(appState, view, fence, appState.Scene.fenceLightmap16List[i], size);
	}
	if (d_lists.last_fence32 >= appState.Scene.fenceLightmap32List.size())
	{
		appState.Scene.fenceLightmap32List.resize(d_lists.last_fence32 + 1);
	}
	for (auto i = 0; i <= d_lists.last_fence32; i++)
	{
		auto& fence = d_lists.fences32[i];
		GetSurfaceLightmapStagingBufferSize(appState, view, fence, appState.Scene.fenceLightmap32List[i], size);
	}
	appState.Scene.firstSharedMemoryTextureToCreate = nullptr;
	appState.Scene.currentSharedMemoryTextureToCreate = nullptr;
	if (d_lists.last_surface16 >= appState.Scene.surfaceTexture16List.size())
	{
		appState.Scene.surfaceTexture16List.resize(d_lists.last_surface16 + 1);
	}
	for (auto i = 0; i <= d_lists.last_surface16; i++)
	{
		auto& surface = d_lists.surfaces16[i];
		auto& surfaceFromList = appState.Scene.surfaceTexture16List[i];
		auto entry = appState.Scene.surfaceTexturesPerKey.find(surface.texture);
		if (entry == appState.Scene.surfaceTexturesPerKey.end())
		{
			SharedMemoryTexture* texture;
			auto mipCount = (int)(std::floor(std::log2(std::max(surface.texture_width, surface.texture_height)))) + 1;
			texture = new SharedMemoryTexture();
			texture->Create(appState, surface.texture_width, surface.texture_height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			surfaceFromList.size = surface.texture_size;
			size += surfaceFromList.size;
			appState.Scene.textures.MoveToFront(texture);
			appState.Scene.surfaceTextureCount++;
			appState.Scene.surfaceTexturesPerKey.insert({ surface.texture, texture });
			surfaceFromList.texture = texture;
			surfaceFromList.source = surface.texture;
			SetupLoadedSharedMemoryTexture(appState, surfaceFromList);
		}
		else
		{
			surfaceFromList.size = 0;
			surfaceFromList.texture = entry->second;
		}
	}
	if (d_lists.last_surface32 >= appState.Scene.surfaceTexture32List.size())
	{
		appState.Scene.surfaceTexture32List.resize(d_lists.last_surface32 + 1);
	}
	for (auto i = 0; i <= d_lists.last_surface32; i++)
	{
		auto& surface = d_lists.surfaces32[i];
		auto& surfaceFromList = appState.Scene.surfaceTexture32List[i];
		auto entry = appState.Scene.surfaceTexturesPerKey.find(surface.texture);
		if (entry == appState.Scene.surfaceTexturesPerKey.end())
		{
			SharedMemoryTexture* texture;
			auto mipCount = (int)(std::floor(std::log2(std::max(surface.texture_width, surface.texture_height)))) + 1;
			texture = new SharedMemoryTexture();
			texture->Create(appState, surface.texture_width, surface.texture_height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			surfaceFromList.size = surface.texture_size;
			size += surfaceFromList.size;
			appState.Scene.textures.MoveToFront(texture);
			appState.Scene.surfaceTextureCount++;
			appState.Scene.surfaceTexturesPerKey.insert({ surface.texture, texture });
			surfaceFromList.texture = texture;
			surfaceFromList.source = surface.texture;
			SetupLoadedSharedMemoryTexture(appState, surfaceFromList);
		}
		else
		{
			surfaceFromList.size = 0;
			surfaceFromList.texture = entry->second;
		}
	}
	if (d_lists.last_fence16 >= appState.Scene.fenceTexture16List.size())
	{
		appState.Scene.fenceTexture16List.resize(d_lists.last_fence16 + 1);
	}
	for (auto i = 0; i <= d_lists.last_fence16; i++)
	{
		auto& fence = d_lists.fences16[i];
		auto& fenceFromList = appState.Scene.fenceTexture16List[i];
		auto entry = appState.Scene.fenceTexturesPerKey.find(fence.texture);
		if (entry == appState.Scene.fenceTexturesPerKey.end())
		{
			SharedMemoryTexture* texture;
			auto mipCount = (int)(std::floor(std::log2(std::max(fence.texture_width, fence.texture_height)))) + 1;
			texture = new SharedMemoryTexture();
			texture->Create(appState, fence.texture_width, fence.texture_height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			fenceFromList.size = fence.texture_size;
			size += fenceFromList.size;
			appState.Scene.textures.MoveToFront(texture);
			appState.Scene.fenceTextureCount++;
			appState.Scene.fenceTexturesPerKey.insert({ fence.texture, texture });
			fenceFromList.texture = texture;
			fenceFromList.source = fence.texture;
			SetupLoadedSharedMemoryTexture(appState, fenceFromList);
		}
		else
		{
			fenceFromList.size = 0;
			fenceFromList.texture = entry->second;
		}
	}
	if (d_lists.last_fence32 >= appState.Scene.fenceTexture32List.size())
	{
		appState.Scene.fenceTexture32List.resize(d_lists.last_fence32 + 1);
	}
	for (auto i = 0; i <= d_lists.last_fence32; i++)
	{
		auto& fence = d_lists.fences32[i];
		auto& fenceFromList = appState.Scene.fenceTexture32List[i];
		auto entry = appState.Scene.fenceTexturesPerKey.find(fence.texture);
		if (entry == appState.Scene.fenceTexturesPerKey.end())
		{
			SharedMemoryTexture* texture;
			auto mipCount = (int)(std::floor(std::log2(std::max(fence.texture_width, fence.texture_height)))) + 1;
			texture = new SharedMemoryTexture();
			texture->Create(appState, fence.texture_width, fence.texture_height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			fenceFromList.size = fence.texture_size;
			size += fenceFromList.size;
			appState.Scene.textures.MoveToFront(texture);
			appState.Scene.fenceTextureCount++;
			appState.Scene.fenceTexturesPerKey.insert({ fence.texture, texture });
			fenceFromList.texture = texture;
			fenceFromList.source = fence.texture;
			SetupLoadedSharedMemoryTexture(appState, fenceFromList);
		}
		else
		{
			fenceFromList.size = 0;
			fenceFromList.texture = entry->second;
		}
	}
	if (d_lists.last_sprite >= appState.Scene.spriteList.size())
	{
		appState.Scene.spriteList.resize(d_lists.last_sprite + 1);
	}
	for (auto i = 0; i <= d_lists.last_sprite; i++)
	{
		auto& sprite = d_lists.sprites[i];
		auto& spriteFromList = appState.Scene.spriteList[i];
		auto entry = appState.Scene.spritesPerKey.find(sprite.data);
		if (entry == appState.Scene.spritesPerKey.end())
		{
			SharedMemoryTexture* texture;
			auto mipCount = (int)(std::floor(std::log2(std::max(sprite.width, sprite.height)))) + 1;
			texture = new SharedMemoryTexture();
			texture->Create(appState, sprite.width, sprite.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			spriteFromList.size = sprite.size;
			size += spriteFromList.size;
			appState.Scene.textures.MoveToFront(texture);
			appState.Scene.spriteTextureCount++;
			appState.Scene.spritesPerKey.insert({ sprite.data, texture });
			spriteFromList.texture = texture;
			spriteFromList.source = sprite.data;
			SetupLoadedSharedMemoryTexture(appState, spriteFromList);
		}
		else
		{
			spriteFromList.size = 0;
			spriteFromList.texture = entry->second;
		}
	}
	appState.Scene.turbulentPerKey.clear();
	if (d_lists.last_turbulent16 >= appState.Scene.turbulent16List.size())
	{
		appState.Scene.turbulent16List.resize(d_lists.last_turbulent16 + 1);
	}
	for (auto i = 0; i <= d_lists.last_turbulent16; i++)
	{
		auto& turbulent = d_lists.turbulent16[i];
		GetTurbulentStagingBufferSize(appState, view, turbulent, appState.Scene.turbulent16List[i], size);
	}
	if (d_lists.last_turbulent32 >= appState.Scene.turbulent32List.size())
	{
		appState.Scene.turbulent32List.resize(d_lists.last_turbulent32 + 1);
	}
	for (auto i = 0; i <= d_lists.last_turbulent32; i++)
	{
		auto& turbulent = d_lists.turbulent32[i];
		GetTurbulentStagingBufferSize(appState, view, turbulent, appState.Scene.turbulent32List[i], size);
	}
	if (d_lists.last_alias16 >= 0)
	{
		if (d_lists.last_alias16 >= appState.Scene.alias16List.size())
		{
			appState.Scene.alias16List.resize(d_lists.last_alias16 + 1);
		}
		GetAliasStagingBufferSize(appState, d_lists.last_alias16, d_lists.alias16, appState.Scene.alias16List, size);
	}
	if (d_lists.last_alias32 >= 0)
	{
		if (d_lists.last_alias32 >= appState.Scene.alias32List.size())
		{
			appState.Scene.alias32List.resize(d_lists.last_alias32 + 1);
		}
		GetAliasStagingBufferSize(appState, d_lists.last_alias32, d_lists.alias32, appState.Scene.alias32List, size);
	}
	if (d_lists.last_viewmodel16 >= 0)
	{
		if (d_lists.last_viewmodel16 >= appState.Scene.viewmodel16List.size())
		{
			appState.Scene.viewmodel16List.resize(d_lists.last_viewmodel16 + 1);
		}
		GetViewmodelStagingBufferSize(appState, d_lists.last_viewmodel16, d_lists.viewmodels16, appState.Scene.viewmodel16List, size);
	}
	if (d_lists.last_viewmodel32)
	{
		if (d_lists.last_viewmodel32 >= appState.Scene.viewmodel32List.size())
		{
			appState.Scene.viewmodel32List.resize(d_lists.last_viewmodel32 + 1);
		}
		GetViewmodelStagingBufferSize(appState, d_lists.last_viewmodel32, d_lists.viewmodels32, appState.Scene.viewmodel32List, size);
	}
	skySize = 0;
	if (d_lists.last_sky >= 0)
	{
		if (sky == nullptr)
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(128, 128)))) + 1;
			sky = new Texture();
			sky->Create(appState, 128, 128, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		skySize = 16384;
		size += skySize;
	}
	return size;
}

void PerImage::LoadStagingBuffer(AppState& appState, int matrixIndex, Buffer* stagingBuffer)
{
	VkDeviceSize offset = 0;
	size_t matricesSize = appState.Scene.numBuffers * sizeof(ovrMatrix4f);
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, &appState.ViewMatrices[matrixIndex], matricesSize);
	offset += matricesSize;
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, &appState.ProjectionMatrices[matrixIndex], matricesSize);
	offset += matricesSize;
	auto loadedBuffer = appState.Scene.firstVertexListToCreate;
	while (loadedBuffer != nullptr)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, loadedBuffer->source, loadedBuffer->size);
		offset += loadedBuffer->size;
		loadedBuffer = loadedBuffer->next;
	}
	if (paletteSize > 0)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_8to24table, paletteSize);
		offset += paletteSize;
	}
	if (host_colormapSize > 0)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, ::host_colormap.data(), host_colormapSize);
		offset += host_colormapSize;
	}
	auto lightmap = appState.Scene.firstLightmapToCreate;
	while (lightmap != nullptr)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, lightmap->source, lightmap->size);
		offset += lightmap->size;
		lightmap = lightmap->next;
	}
	auto sharedMemoryTexture = appState.Scene.firstSharedMemoryTextureToCreate;
	while (sharedMemoryTexture != nullptr)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, sharedMemoryTexture->source, sharedMemoryTexture->size);
		offset += sharedMemoryTexture->size;
		sharedMemoryTexture = sharedMemoryTexture->next;
	}
	for (auto i = 0; i <= d_lists.last_turbulent16; i++)
	{
		auto size = appState.Scene.turbulent16List[i].size;
		if (size > 0)
		{
			auto& turbulent = d_lists.turbulent16[i];
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, turbulent.data, turbulent.size);
			offset += size;
		}
	}
	for (auto i = 0; i <= d_lists.last_turbulent32; i++)
	{
		auto size = appState.Scene.turbulent32List[i].size;
		if (size > 0)
		{
			auto& turbulent = d_lists.turbulent32[i];
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, turbulent.data, turbulent.size);
			offset += size;
		}
	}
	for (auto i = 0; i <= d_lists.last_alias16; i++)
	{
		auto& alias = d_lists.alias16[i];
		auto size = appState.Scene.alias16List[i].texture.size;
		if (size > 0)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.data, alias.size);
			offset += size;
		}
		if (!alias.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.colormap.data(), 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= d_lists.last_alias32; i++)
	{
		auto& alias = d_lists.alias32[i];
		auto size = appState.Scene.alias32List[i].texture.size;
		if (size > 0)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.data, alias.size);
			offset += size;
		}
		if (!alias.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.colormap.data(), 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= d_lists.last_viewmodel16; i++)
	{
		auto& viewmodel = d_lists.viewmodels16[i];
		auto size = appState.Scene.viewmodel16List[i].texture.size;
		if (size > 0)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.data, viewmodel.size);
			offset += size;
		}
		if (!viewmodel.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.colormap.data(), 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= d_lists.last_viewmodel32; i++)
	{
		auto& viewmodel = d_lists.viewmodels32[i];
		auto size = appState.Scene.viewmodel32List[i].texture.size;
		if (size > 0)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.data, viewmodel.size);
			offset += size;
		}
		if (!viewmodel.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.colormap.data(), 16384);
			offset += 16384;
		}
	}
	if (skySize > 0)
	{
		auto source = r_skysource;
		auto target = ((unsigned char*)stagingBuffer->mapped) + offset;
		for (auto i = 0; i < 128; i++)
		{
			memcpy(target, source, 128);
			source += 256;
			target += 128;
		}
	}
}

void PerImage::FillAliasTextures(AppState& appState, LoadedColormappedTexture& loadedTexture, dalias_t& alias)
{
	if (loadedTexture.texture.size > 0)
	{
		loadedTexture.texture.texture->FillMipmapped(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += loadedTexture.texture.size;
	}
	if (!alias.is_host_colormap)
	{
		Texture* texture;
		if (colormaps.oldTextures != nullptr)
		{
			texture = colormaps.oldTextures;
			colormaps.oldTextures = texture->next;
		}
		else
		{
			texture = new Texture();
			texture->Create(appState, 256, 64, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		texture->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += 16384;
		colormaps.MoveToFront(texture);
		loadedTexture.colormap.texture = texture;
		colormapCount++;
	}
}

void PerImage::FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer)
{
	appState.Scene.stagingBuffer.lastBarrier = -1;
	VkBufferCopy bufferCopy { };
	bufferCopy.size = appState.Scene.matrices.size;
	VC(appState.Device.vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, appState.Scene.matrices.buffer, 1, &bufferCopy));
	VkBufferMemoryBarrier barrier { };
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.buffer = appState.Scene.matrices.buffer;
	barrier.size = VK_WHOLE_SIZE;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
	VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr));
	bufferCopy.srcOffset += appState.Scene.matrices.size;
	auto loadedBuffer = appState.Scene.firstVertexListToCreate;
	while (loadedBuffer != nullptr)
	{
		appState.Scene.stagingBuffer.lastBarrier++;
		if (appState.Scene.stagingBuffer.bufferBarriers.size() <= appState.Scene.stagingBuffer.lastBarrier)
		{
			appState.Scene.stagingBuffer.bufferBarriers.emplace_back();
		}
		bufferCopy.size = loadedBuffer->size;
		VC(appState.Device.vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, loadedBuffer->buffer->buffer, 1, &bufferCopy));
		auto& barrier = appState.Scene.stagingBuffer.bufferBarriers[appState.Scene.stagingBuffer.lastBarrier];
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.buffer = loadedBuffer->buffer->buffer;
		barrier.size = VK_WHOLE_SIZE;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		bufferCopy.srcOffset += loadedBuffer->size;
		loadedBuffer = loadedBuffer->next;
	}
	if (appState.Scene.stagingBuffer.lastBarrier >= 0)
	{
		VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, appState.Scene.stagingBuffer.lastBarrier + 1, appState.Scene.stagingBuffer.bufferBarriers.data(), 0, nullptr));
	}
	appState.Scene.stagingBuffer.buffer = stagingBuffer;
	appState.Scene.stagingBuffer.offset = bufferCopy.srcOffset;
	appState.Scene.stagingBuffer.commandBuffer = commandBuffer;
	appState.Scene.stagingBuffer.lastBarrier = -1;
	if (paletteSize > 0)
	{
		palette->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += paletteSize;
	}
	if (host_colormapSize > 0)
	{
		host_colormap->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += host_colormapSize;
	}
	auto lightmap = appState.Scene.firstLightmapToCreate;
	while (lightmap != nullptr)
	{
		lightmap->lightmap->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += lightmap->size;
		lightmap = lightmap->next;
	}
	auto sharedMemoryTexture = appState.Scene.firstSharedMemoryTextureToCreate;
	while (sharedMemoryTexture != nullptr)
	{
		sharedMemoryTexture->texture->FillMipmapped(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += sharedMemoryTexture->size;
		sharedMemoryTexture = sharedMemoryTexture->next;
	}
	for (auto i = 0; i <= d_lists.last_turbulent16; i++)
	{
		auto size = appState.Scene.turbulent16List[i].size;
		if (size > 0)
		{
			appState.Scene.turbulent16List[i].texture->FillMipmapped(appState, appState.Scene.stagingBuffer);
			appState.Scene.stagingBuffer.offset += size;
		}
	}
	for (auto i = 0; i <= d_lists.last_turbulent32; i++)
	{
		auto size = appState.Scene.turbulent32List[i].size;
		if (size > 0)
		{
			appState.Scene.turbulent32List[i].texture->FillMipmapped(appState, appState.Scene.stagingBuffer);
			appState.Scene.stagingBuffer.offset += size;
		}
	}
	for (auto i = 0; i <= d_lists.last_alias16; i++)
	{
		auto& alias = d_lists.alias16[i];
		FillAliasTextures(appState, appState.Scene.alias16List[i], alias);
	}
	for (auto i = 0; i <= d_lists.last_alias32; i++)
	{
		auto& alias = d_lists.alias32[i];
		FillAliasTextures(appState, appState.Scene.alias32List[i], alias);
	}
	for (auto i = 0; i <= d_lists.last_viewmodel16; i++)
	{
		auto& viewmodel = d_lists.viewmodels16[i];
		FillAliasTextures(appState, appState.Scene.viewmodel16List[i], viewmodel);
	}
	for (auto i = 0; i <= d_lists.last_viewmodel32; i++)
	{
		auto& viewmodel = d_lists.viewmodels32[i];
		FillAliasTextures(appState, appState.Scene.viewmodel32List[i], viewmodel);
	}
	if (skySize > 0)
	{
		sky->FillMipmapped(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += skySize;
	}
	if (appState.Scene.stagingBuffer.lastBarrier >= 0)
	{
		VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, appState.Scene.stagingBuffer.lastBarrier + 1, appState.Scene.stagingBuffer.imageBarriers.data()));
	}
}

void PerImage::LoadAliasBuffers(AppState& appState, int lastAlias, std::vector<dalias_t>& aliasList, std::vector<int>& aliasVertices, std::vector<int>& aliasTexCoords)
{
	VkDeviceSize verticesOffset = 0;
	VkDeviceSize texCoordsOffset = 0;
	appState.Scene.newVertices.clear();
	appState.Scene.newTexCoords.clear();
	for (auto i = 0; i <= lastAlias; i++)
	{
		auto& alias = aliasList[i];
		auto verticesEntry = appState.Scene.colormappedVerticesPerKey.find(alias.vertices);
		if (verticesEntry == appState.Scene.colormappedVerticesPerKey.end())
		{
			auto lastIndex = appState.Scene.colormappedBufferList.size();
			appState.Scene.colormappedBufferList.push_back({ verticesOffset });
			appState.Scene.colormappedVerticesPerKey.insert({ alias.vertices, lastIndex });
			appState.Scene.newVertices.push_back(i);
			aliasVertices[i] = lastIndex;
			verticesOffset += alias.vertex_count * 2 * 4 * sizeof(float);
		}
		else
		{
			aliasVertices[i] = verticesEntry->second;
		}
		auto texCoordsEntry = appState.Scene.colormappedTexCoordsPerKey.find(alias.texture_coordinates);
		if (texCoordsEntry == appState.Scene.colormappedTexCoordsPerKey.end())
		{
			auto lastIndex = appState.Scene.colormappedBufferList.size();
			appState.Scene.colormappedBufferList.push_back({ texCoordsOffset });
			appState.Scene.colormappedTexCoordsPerKey.insert({ alias.texture_coordinates, lastIndex });
			appState.Scene.newTexCoords.push_back(i);
			aliasTexCoords[i] = lastIndex;
			texCoordsOffset += alias.vertex_count * 2 * 2 * sizeof(float);
		}
		else
		{
			aliasTexCoords[i] = texCoordsEntry->second;
		}
	}
	if (verticesOffset > 0)
	{
		Buffer* buffer;
		if (appState.Scene.latestColormappedBuffer == nullptr || appState.Scene.usedInLatestColormappedBuffer + verticesOffset > MEMORY_BLOCK_SIZE)
		{
			VkDeviceSize size = MEMORY_BLOCK_SIZE;
			if (size < verticesOffset)
			{
				size = verticesOffset;
			}
			buffer = new Buffer();
			buffer->CreateHostVisibleVertexBuffer(appState, size);
			appState.Scene.colormappedBuffers.MoveToFront(buffer);
			appState.Scene.latestColormappedBuffer = buffer;
			appState.Scene.usedInLatestColormappedBuffer = 0;
		}
		else
		{
			buffer = appState.Scene.latestColormappedBuffer;
		}
		VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, appState.Scene.usedInLatestColormappedBuffer, verticesOffset, 0, &buffer->mapped));
		auto mapped = (float*)buffer->mapped;
		for (auto i : appState.Scene.newVertices)
		{
			auto& alias = aliasList[i];
			auto vertexFromModel = alias.vertices;
			for (auto j = 0; j < alias.vertex_count; j++)
			{
				auto x = (float)(vertexFromModel->v[0]);
				auto y = (float)(vertexFromModel->v[1]);
				auto z = (float)(vertexFromModel->v[2]);
				*mapped++ = x;
				*mapped++ = z;
				*mapped++ = -y;
				*mapped++ = 1;
				*mapped++ = x;
				*mapped++ = z;
				*mapped++ = -y;
				*mapped++ = 1;
				vertexFromModel++;
			}
			auto index = aliasVertices[i];
			appState.Scene.colormappedBufferList[index].buffer = buffer;
			appState.Scene.colormappedBufferList[index].offset += appState.Scene.usedInLatestColormappedBuffer;
		}
		VC(appState.Device.vkUnmapMemory(appState.Device.device, buffer->memory));
		buffer->mapped = nullptr;
		buffer->SubmitVertexBuffer(appState, commandBuffer);
		appState.Scene.usedInLatestColormappedBuffer += verticesOffset;
	}
	if (texCoordsOffset > 0)
	{
		Buffer* buffer;
		if (appState.Scene.latestColormappedBuffer == nullptr || appState.Scene.usedInLatestColormappedBuffer + texCoordsOffset > MEMORY_BLOCK_SIZE)
		{
			VkDeviceSize size = MEMORY_BLOCK_SIZE;
			if (size < texCoordsOffset)
			{
				size = texCoordsOffset;
			}
			buffer = new Buffer();
			buffer->CreateHostVisibleVertexBuffer(appState, size);
			appState.Scene.colormappedBuffers.MoveToFront(buffer);
			appState.Scene.latestColormappedBuffer = buffer;
			appState.Scene.usedInLatestColormappedBuffer = 0;
		}
		else
		{
			buffer = appState.Scene.latestColormappedBuffer;
		}
		VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, appState.Scene.usedInLatestColormappedBuffer, texCoordsOffset, 0, &buffer->mapped));
		auto mapped = (float*)buffer->mapped;
		for (auto i : appState.Scene.newTexCoords)
		{
			auto& alias = aliasList[i];
			auto texCoords = alias.texture_coordinates;
			for (auto j = 0; j < alias.vertex_count; j++)
			{
				auto s = (float)(texCoords->s >> 16);
				auto t = (float)(texCoords->t >> 16);
				s /= alias.width;
				t /= alias.height;
				*mapped++ = s;
				*mapped++ = t;
				*mapped++ = s + 0.5;
				*mapped++ = t;
				texCoords++;
			}
			auto index = aliasTexCoords[i];
			appState.Scene.colormappedBufferList[index].buffer = buffer;
			appState.Scene.colormappedBufferList[index].offset += appState.Scene.usedInLatestColormappedBuffer;
		}
		VC(appState.Device.vkUnmapMemory(appState.Device.device, buffer->memory));
		buffer->mapped = nullptr;
		buffer->SubmitVertexBuffer(appState, commandBuffer);
		appState.Scene.usedInLatestColormappedBuffer += texCoordsOffset;
	}
}

void PerImage::LoadRemainingBuffers(AppState& appState)
{
	VkDeviceSize floorVerticesSize;
	if (appState.Mode != AppWorldMode)
	{
		floorVerticesSize = 3 * 4 * sizeof(float);
	}
	else
	{
		floorVerticesSize = 0;
	}
	VkDeviceSize texturedVerticesSize = (d_lists.last_textured_vertex + 1) * sizeof(float);
	VkDeviceSize coloredVerticesSize = (d_lists.last_colored_vertex + 1) * sizeof(float);
	appState.Scene.controllerVerticesSize = 0;
	if (appState.Mode != AppWorldMode || key_dest == key_console)
	{
		if (appState.LeftController.TrackingResult == ovrSuccess)
		{
			appState.Scene.controllerVerticesSize += 2 * 8 * 3 * sizeof(float);
		}
		if (appState.RightController.TrackingResult == ovrSuccess)
		{
			appState.Scene.controllerVerticesSize += 2 * 8 * 3 * sizeof(float);
		}
	}
	appState.Scene.verticesSize = floorVerticesSize + texturedVerticesSize + coloredVerticesSize + appState.Scene.controllerVerticesSize;
	if (appState.Scene.verticesSize > 0)
	{
		vertices = cachedVertices.GetVertexBuffer(appState, appState.Scene.verticesSize);
		if (floorVerticesSize > 0)
		{
			auto mapped = (float*)vertices->mapped;
			*mapped++ = -0.5;
			*mapped++ = appState.Scene.pose.Position.y;
			*mapped++ = -0.5;
			*mapped++ = 0.5;
			*mapped++ = appState.Scene.pose.Position.y;
			*mapped++ = -0.5;
			*mapped++ = 0.5;
			*mapped++ = appState.Scene.pose.Position.y;
			*mapped++ = 0.5;
			*mapped++ = -0.5;
			*mapped++ = appState.Scene.pose.Position.y;
			*mapped++ = 0.5;
		}
		texturedVertexBase = floorVerticesSize;
		memcpy((unsigned char*)vertices->mapped + texturedVertexBase, d_lists.textured_vertices.data(), texturedVerticesSize);
		coloredVertexBase = texturedVertexBase + texturedVerticesSize;
		memcpy((unsigned char*)vertices->mapped + coloredVertexBase, d_lists.colored_vertices.data(), coloredVerticesSize);
		controllerVertexBase = coloredVertexBase + coloredVerticesSize;
		if (appState.Scene.controllerVerticesSize > 0)
		{
			auto mapped = (float*)vertices->mapped + controllerVertexBase / sizeof(float);
			if (appState.LeftController.TrackingResult == ovrSuccess)
			{
				mapped = appState.LeftController.WriteVertices(mapped);
			}
			if (appState.RightController.TrackingResult == ovrSuccess)
			{
				mapped = appState.RightController.WriteVertices(mapped);
			}
		}
		vertices->SubmitVertexBuffer(appState, commandBuffer);
	}
	if (d_lists.last_alias16 >= 0)
	{
		if (d_lists.last_alias16 >= appState.Scene.aliasVertices16List.size())
		{
			appState.Scene.aliasVertices16List.resize(d_lists.last_alias16 + 1);
			appState.Scene.aliasTexCoords16List.resize(d_lists.last_alias16 + 1);
		}
		LoadAliasBuffers(appState, d_lists.last_alias16, d_lists.alias16, appState.Scene.aliasVertices16List, appState.Scene.aliasTexCoords16List);
	}
	if (d_lists.last_alias32 >= 0)
	{
		if (d_lists.last_alias32 >= appState.Scene.aliasVertices32List.size())
		{
			appState.Scene.aliasVertices32List.resize(d_lists.last_alias32 + 1);
			appState.Scene.aliasTexCoords32List.resize(d_lists.last_alias32 + 1);
		}
		LoadAliasBuffers(appState, d_lists.last_alias32, d_lists.alias32, appState.Scene.aliasVertices32List, appState.Scene.aliasTexCoords32List);
	}
	if (d_lists.last_viewmodel16 >= 0)
	{
		if (d_lists.last_viewmodel16 >= appState.Scene.viewmodelVertices16List.size())
		{
			appState.Scene.viewmodelVertices16List.resize(d_lists.last_viewmodel16 + 1);
			appState.Scene.viewmodelTexCoords16List.resize(d_lists.last_viewmodel16 + 1);
		}
		LoadAliasBuffers(appState, d_lists.last_viewmodel16, d_lists.viewmodels16, appState.Scene.viewmodelVertices16List, appState.Scene.viewmodelTexCoords16List);
	}
	if (d_lists.last_viewmodel32 >= 0)
	{
		if (d_lists.last_viewmodel32 >= appState.Scene.viewmodelVertices32List.size())
		{
			appState.Scene.viewmodelVertices32List.resize(d_lists.last_viewmodel32 + 1);
			appState.Scene.viewmodelTexCoords32List.resize(d_lists.last_viewmodel32 + 1);
		}
		LoadAliasBuffers(appState, d_lists.last_viewmodel32, d_lists.viewmodels32, appState.Scene.viewmodelVertices32List, appState.Scene.viewmodelTexCoords32List);
	}
	VkDeviceSize floorAttributesSize;
	if (appState.Mode != AppWorldMode)
	{
		floorAttributesSize = 2 * 4 * sizeof(float);
	}
	else
	{
		floorAttributesSize = 0;
	}
	VkDeviceSize texturedAttributesSize = (d_lists.last_textured_attribute + 1) * sizeof(float);
	VkDeviceSize colormappedLightsSize = (d_lists.last_colormapped_attribute + 1) * sizeof(float);
	VkDeviceSize vertexTransformSize = 16 * sizeof(float);
	VkDeviceSize controllerAttributesSize = 0;
	if (appState.Scene.controllerVerticesSize > 0)
	{
		if (appState.LeftController.TrackingResult == ovrSuccess)
		{
			controllerAttributesSize += 2 * 8 * 2 * sizeof(float);
		}
		if (appState.RightController.TrackingResult == ovrSuccess)
		{
			controllerAttributesSize += 2 * 8 * 2 * sizeof(float);
		}
	}
	VkDeviceSize attributesSize = floorAttributesSize + texturedAttributesSize + colormappedLightsSize + vertexTransformSize + controllerAttributesSize;
	attributes = cachedAttributes.GetVertexBuffer(appState, attributesSize);
	if (floorAttributesSize > 0)
	{
		auto mapped = (float*)attributes->mapped;
		*mapped++ = 0;
		*mapped++ = 0;
		*mapped++ = 1;
		*mapped++ = 0;
		*mapped++ = 1;
		*mapped++ = 1;
		*mapped++ = 0;
		*mapped++ = 1;
	}
	texturedAttributeBase = floorAttributesSize;
	memcpy((unsigned char*)attributes->mapped + texturedAttributeBase, d_lists.textured_attributes.data(), texturedAttributesSize);
	colormappedAttributeBase = texturedAttributeBase + texturedAttributesSize;
	memcpy((unsigned char*)attributes->mapped + colormappedAttributeBase, d_lists.colormapped_attributes.data(), colormappedLightsSize);
	vertexTransformBase = colormappedAttributeBase + colormappedLightsSize;
	auto mapped = (float*)attributes->mapped + vertexTransformBase / sizeof(float);
	*mapped++ = appState.Scale;
	*mapped++ = 0;
	*mapped++ = 0;
	*mapped++ = 0;
	*mapped++ = 0;
	*mapped++ = appState.Scale;
	*mapped++ = 0;
	*mapped++ = 0;
	*mapped++ = 0;
	*mapped++ = 0;
	*mapped++ = appState.Scale;
	*mapped++ = 0;
	*mapped++ = -r_refdef.vieworg[0] * appState.Scale;
	*mapped++ = -r_refdef.vieworg[2] * appState.Scale;
	*mapped++ = r_refdef.vieworg[1] * appState.Scale;
	*mapped++ = 1;
	controllerAttributeBase = vertexTransformBase + 16 * sizeof(float);
	if (controllerAttributesSize > 0)
	{
		if (appState.LeftController.TrackingResult == ovrSuccess)
		{
			mapped = appState.LeftController.WriteAttributes(mapped);
		}
		if (appState.RightController.TrackingResult == ovrSuccess)
		{
			mapped = appState.RightController.WriteAttributes(mapped);
		}
	}
	attributes->SubmitVertexBuffer(appState, commandBuffer);
	VkDeviceSize floorIndicesSize;
	if (appState.Mode != AppWorldMode)
	{
		floorIndicesSize = 6 * sizeof(uint16_t);
	}
	else
	{
		floorIndicesSize = 0;
	}
	VkDeviceSize surfaceIndices16Size = (d_lists.last_surface_index16 + 1) * sizeof(uint16_t);
	VkDeviceSize colormappedIndices16Size = (d_lists.last_colormapped_index16 + 1) * sizeof(uint16_t);
	VkDeviceSize coloredIndices16Size = (d_lists.last_colored_index16 + 1) * sizeof(uint16_t);
	VkDeviceSize controllerIndices16Size = 0;
	if (appState.Scene.controllerVerticesSize > 0)
	{
		if (appState.LeftController.TrackingResult == ovrSuccess)
		{
			controllerIndices16Size += 2 * 36 * sizeof(uint16_t);
		}
		if (appState.RightController.TrackingResult == ovrSuccess)
		{
			controllerIndices16Size += 2 * 36 * sizeof(uint16_t);
		}
	}
	VkDeviceSize indices16Size = floorIndicesSize + surfaceIndices16Size + colormappedIndices16Size + coloredIndices16Size + controllerIndices16Size;
	if (indices16Size > 0)
	{
		indices16 = cachedIndices16.GetIndexBuffer(appState, indices16Size);
		if (floorIndicesSize > 0)
		{
			auto mapped = (uint16_t*)indices16->mapped;
			*mapped++ = 0;
			*mapped++ = 1;
			*mapped++ = 2;
			*mapped++ = 2;
			*mapped++ = 3;
			*mapped++ = 0;
		}
		surfaceIndex16Base = floorIndicesSize;
		memcpy((unsigned char*)indices16->mapped + surfaceIndex16Base, d_lists.surface_indices16.data(), surfaceIndices16Size);
		colormappedIndex16Base = surfaceIndex16Base + surfaceIndices16Size;
		memcpy((unsigned char*)indices16->mapped + colormappedIndex16Base, d_lists.colormapped_indices16.data(), colormappedIndices16Size);
		coloredIndex16Base = colormappedIndex16Base + colormappedIndices16Size;
		memcpy((unsigned char*)indices16->mapped + coloredIndex16Base, d_lists.colored_indices16.data(), coloredIndices16Size);
		controllerIndex16Base = coloredIndex16Base + coloredIndices16Size;
		if (controllerIndices16Size > 0)
		{
			auto mapped = (uint16_t*)indices16->mapped + controllerIndex16Base / sizeof(uint16_t);
			auto offset = 0;
			if (appState.LeftController.TrackingResult == ovrSuccess)
			{
				mapped = appState.LeftController.WriteIndices(mapped, offset);
				offset += 8;
				mapped = appState.LeftController.WriteIndices(mapped, offset);
				offset += 8;
			}
			if (appState.RightController.TrackingResult == ovrSuccess)
			{
				mapped = appState.RightController.WriteIndices(mapped, offset);
				offset += 8;
				mapped = appState.RightController.WriteIndices(mapped, offset);
			}
		}
		indices16->SubmitIndexBuffer(appState, commandBuffer);
	}
	VkDeviceSize surfaceIndices32Size = (d_lists.last_surface_index32 + 1) * sizeof(uint32_t);
	VkDeviceSize colormappedIndices32Size = (d_lists.last_colormapped_index32 + 1) * sizeof(uint32_t);
	VkDeviceSize coloredIndices32Size = (d_lists.last_colored_index32 + 1) * sizeof(uint32_t);
	VkDeviceSize indices32Size = surfaceIndices32Size + colormappedIndices32Size + coloredIndices32Size;
	if (indices32Size > 0)
	{
		indices32 = cachedIndices32.GetIndexBuffer(appState, indices32Size);
		memcpy(indices32->mapped, d_lists.surface_indices32.data(), surfaceIndices32Size);
		colormappedIndex32Base = surfaceIndices32Size;
		memcpy((unsigned char*)indices32->mapped + colormappedIndex32Base, d_lists.colormapped_indices32.data(), colormappedIndices32Size);
		coloredIndex32Base = colormappedIndex32Base + colormappedIndices32Size;
		memcpy((unsigned char*)indices32->mapped + coloredIndex32Base, d_lists.colored_indices32.data(), coloredIndices32Size);
		indices32->SubmitIndexBuffer(appState, commandBuffer);
	}
	VkDeviceSize colorsSize = (d_lists.last_colored_attribute + 1) * sizeof(float);
	if (colorsSize > 0)
	{
		colors = cachedColors.GetVertexBuffer(appState, colorsSize);
		memcpy(colors->mapped, d_lists.colored_attributes.data(), colorsSize);
		colors->SubmitVertexBuffer(appState, commandBuffer);
	}
}

void PerImage::SetPushConstants(dsurface_t& surface, float pushConstants[])
{
	pushConstants[0] = surface.origin_x;
	pushConstants[1] = surface.origin_y;
	pushConstants[2] = surface.origin_z;
	pushConstants[3] = surface.yaw * M_PI / 180;
	pushConstants[4] = surface.pitch * M_PI / 180;
	pushConstants[5] = -surface.roll * M_PI / 180;
	pushConstants[6] = surface.vecs[0][0];
	pushConstants[7] = surface.vecs[0][1];
	pushConstants[8] = surface.vecs[0][2];
	pushConstants[9] = surface.vecs[0][3];
	pushConstants[10] = surface.vecs[1][0];
	pushConstants[11] = surface.vecs[1][1];
	pushConstants[12] = surface.vecs[1][2];
	pushConstants[13] = surface.vecs[1][3];
	pushConstants[14] = surface.texturemins[0];
	pushConstants[15] = surface.texturemins[1];
	pushConstants[16] = surface.extents[0];
	pushConstants[17] = surface.extents[1];
	pushConstants[18] = surface.texture_width;
	pushConstants[19] = surface.texture_height;
}

void PerImage::SetPushConstants(dturbulent_t& turbulent, float pushConstants[])
{
	pushConstants[0] = turbulent.origin_x;
	pushConstants[1] = turbulent.origin_y;
	pushConstants[2] = turbulent.origin_z;
	pushConstants[3] = turbulent.yaw * M_PI / 180;
	pushConstants[4] = turbulent.pitch * M_PI / 180;
	pushConstants[5] = -turbulent.roll * M_PI / 180;
	pushConstants[6] = turbulent.vecs[0][0];
	pushConstants[7] = turbulent.vecs[0][1];
	pushConstants[8] = turbulent.vecs[0][2];
	pushConstants[9] = turbulent.vecs[0][3];
	pushConstants[10] = turbulent.vecs[1][0];
	pushConstants[11] = turbulent.vecs[1][1];
	pushConstants[12] = turbulent.vecs[1][2];
	pushConstants[13] = turbulent.vecs[1][3];
	pushConstants[14] = turbulent.width;
	pushConstants[15] = turbulent.height;
}

void PerImage::SetPushConstants(dalias_t& alias, float pushConstants[])
{
	pushConstants[0] = alias.transform[0][0];
	pushConstants[1] = alias.transform[2][0];
	pushConstants[2] = -alias.transform[1][0];
	pushConstants[4] = alias.transform[0][2];
	pushConstants[5] = alias.transform[2][2];
	pushConstants[6] = -alias.transform[1][2];
	pushConstants[8] = -alias.transform[0][1];
	pushConstants[9] = -alias.transform[2][1];
	pushConstants[10] = alias.transform[1][1];
	pushConstants[12] = alias.transform[0][3];
	pushConstants[13] = alias.transform[2][3];
	pushConstants[14] = -alias.transform[1][3];
}

VkDescriptorSet PerImage::GetDescriptorSet(AppState& appState, SharedMemoryTexture* texture, CachedPipelineDescriptorResources& resources, VkDescriptorImageInfo textureInfo[], VkWriteDescriptorSet writes[])
{
	auto entry = resources.cache.find(texture);
	if (entry != resources.cache.end())
	{
		return entry->second;
	}
	auto descriptorSet = resources.descriptorSets[resources.index];
	resources.index++;
	textureInfo[0].sampler = appState.Scene.textureSamplers[texture->mipCount];
	textureInfo[0].imageView = texture->view;
	writes[0].dstSet = descriptorSet;
	VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
	resources.cache.insert({ texture, descriptorSet });
	return descriptorSet;
}

void PerImage::Render(AppState& appState)
{
	if (d_lists.last_surface16 < 0 && d_lists.last_surface32 < 0 && d_lists.last_fence16 < 0 && d_lists.last_fence32 < 0 && appState.Scene.verticesSize == 0 && d_lists.last_alias16 < 0 && d_lists.last_alias32 < 0 && d_lists.last_viewmodel16 < 0 && d_lists.last_viewmodel32 < 0)
	{
		return;
	}
	VkDescriptorPoolSize poolSizes[3] { };
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { };
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.pPoolSizes = poolSizes;
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { };
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	VkDescriptorImageInfo textureInfo[2] { };
	textureInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	textureInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VkWriteDescriptorSet writes[3] { };
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].descriptorCount = 1;
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].descriptorCount = 1;
	writes[1].dstBinding = 1;
	writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[2].descriptorCount = 1;
	writes[2].dstBinding = 2;
	if (!host_colormapResources.created && host_colormap != nullptr)
	{
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = 1;
		textureInfo[0].sampler = appState.Scene.textureSamplers[host_colormap->mipCount];
		textureInfo[0].imageView = host_colormap->view;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo = textureInfo;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &host_colormapResources.descriptorPool));
		descriptorSetAllocateInfo.descriptorPool = host_colormapResources.descriptorPool;
		descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
		VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &host_colormapResources.descriptorSet));
		writes[0].dstSet = host_colormapResources.descriptorSet;
		VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
		host_colormapResources.created = true;
	}
	if (!sceneMatricesResources.created || !sceneMatricesAndPaletteResources.created || (!sceneMatricesAndColormapResources.created && host_colormap != nullptr))
	{
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 1;
		VkDescriptorBufferInfo bufferInfo { };
		bufferInfo.buffer = appState.Scene.matrices.buffer;
		bufferInfo.range = appState.Scene.matrices.size;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].pBufferInfo = &bufferInfo;
		if (!sceneMatricesResources.created)
		{
			descriptorPoolCreateInfo.poolSizeCount = 1;
			VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = sceneMatricesResources.descriptorPool;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleBufferLayout;
			VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &sceneMatricesResources.descriptorSet));
			writes[0].dstSet = sceneMatricesResources.descriptorSet;
			VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
			sceneMatricesResources.created = true;
		}
		if (!sceneMatricesAndPaletteResources.created || (!sceneMatricesAndColormapResources.created && host_colormap != nullptr))
		{
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[1].descriptorCount = 1;
			textureInfo[0].sampler = appState.Scene.textureSamplers[palette->mipCount];
			textureInfo[0].imageView = palette->view;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[1].pImageInfo = textureInfo;
			if (!sceneMatricesAndPaletteResources.created)
			{
				descriptorPoolCreateInfo.poolSizeCount = 2;
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesAndPaletteResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = sceneMatricesAndPaletteResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.bufferAndImageLayout;
				VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &sceneMatricesAndPaletteResources.descriptorSet));
				writes[0].dstSet = sceneMatricesAndPaletteResources.descriptorSet;
				writes[1].dstSet = sceneMatricesAndPaletteResources.descriptorSet;
				VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 2, writes, 0, nullptr));
				sceneMatricesAndPaletteResources.created = true;
			}
			if (!sceneMatricesAndColormapResources.created && host_colormap != nullptr)
			{
				poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolSizes[2].descriptorCount = 1;
				textureInfo[1].sampler = appState.Scene.textureSamplers[host_colormap->mipCount];
				textureInfo[1].imageView = host_colormap->view;
				writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writes[2].pImageInfo = textureInfo + 1;
				descriptorPoolCreateInfo.poolSizeCount = 3;
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesAndColormapResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = sceneMatricesAndColormapResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.bufferAndTwoImagesLayout;
				VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &sceneMatricesAndColormapResources.descriptorSet));
				writes[0].dstSet = sceneMatricesAndColormapResources.descriptorSet;
				writes[1].dstSet = sceneMatricesAndColormapResources.descriptorSet;
				writes[2].dstSet = sceneMatricesAndColormapResources.descriptorSet;
				VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 3, writes, 0, nullptr));
				sceneMatricesAndColormapResources.created = true;
			}
		}
	}
	if (appState.Mode == AppWorldMode)
	{
		float pushConstants[24];
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = 1;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo = textureInfo;
		if (resetDescriptorSetsCount != appState.Scene.resetDescriptorSetsCount || surfaceTextureResources.descriptorSets.size() < appState.Scene.surfaceTextureCount)
		{
			surfaceTextureResources.Delete(appState);
			auto toCreate = appState.Scene.surfaceTextureCount;
			if (toCreate > 0)
			{
				poolSizes[0].descriptorCount = toCreate;
				descriptorPoolCreateInfo.maxSets = toCreate;
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &surfaceTextureResources.descriptorPool));
				surfaceTextureResources.descriptorSetLayouts.resize(toCreate);
				surfaceTextureResources.descriptorSets.resize(toCreate);
				std::fill(surfaceTextureResources.descriptorSetLayouts.begin(), surfaceTextureResources.descriptorSetLayouts.end(), appState.Scene.singleImageLayout);
				descriptorSetAllocateInfo.descriptorPool = surfaceTextureResources.descriptorPool;
				descriptorSetAllocateInfo.descriptorSetCount = toCreate;
				descriptorSetAllocateInfo.pSetLayouts = surfaceTextureResources.descriptorSetLayouts.data();
				VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, surfaceTextureResources.descriptorSets.data()));
				surfaceTextureResources.created = true;
			}
		}
		if (d_lists.last_surface16 >= 0 || d_lists.last_surface32 >= 0)
		{
			VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipeline));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &vertexTransformBase));
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr));
		}
		if (d_lists.last_surface16 >= 0)
		{
			void* previousVertexes = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, surfaceIndex16Base, VK_INDEX_TYPE_UINT16));
			for (auto i = 0; i <= d_lists.last_surface16; i++)
			{
				auto& surface = d_lists.surfaces16[i];
				if (previousVertexes != surface.vertexes)
				{
					auto vertexBuffer = appState.Scene.surfaceVertex16List[i].buffer->buffer;
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &appState.NoOffset));
					previousVertexes = surface.vertexes;
				}
				SetPushConstants(surface, pushConstants);
				VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.surfaces.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 20 * sizeof(float), pushConstants));
				auto texture = appState.Scene.surfaceTexture16List[i].texture;
				if (previousTexture != texture)
				{
					auto descriptorSet = GetDescriptorSet(appState, texture, surfaceTextureResources, textureInfo, writes);
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
					previousTexture = texture;
				}
				auto lightmap = appState.Scene.surfaceLightmap16List[i].lightmap;
				VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 2, 1, &lightmap->descriptorSet, 0, nullptr));
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, surface.count, 1, surface.first_index, 0, 0));
			}
		}
		if (d_lists.last_surface32 >= 0)
		{
			void* previousVertexes = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32));
			for (auto i = 0; i <= d_lists.last_surface32; i++)
			{
				auto& surface = d_lists.surfaces32[i];
				if (previousVertexes != surface.vertexes)
				{
					auto vertexBuffer = appState.Scene.surfaceVertex32List[i].buffer->buffer;
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &appState.NoOffset));
					previousVertexes = surface.vertexes;
				}
				SetPushConstants(surface, pushConstants);
				VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.surfaces.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 20 * sizeof(float), pushConstants));
				auto texture = appState.Scene.surfaceTexture32List[i].texture;
				if (previousTexture != texture)
				{
					auto descriptorSet = GetDescriptorSet(appState, texture, surfaceTextureResources, textureInfo, writes);
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
					previousTexture = texture;
				}
				auto lightmap = appState.Scene.surfaceLightmap32List[i].lightmap;
				VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 2, 1, &lightmap->descriptorSet, 0, nullptr));
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, surface.count, 1, surface.first_index, 0, 0));
			}
		}
		if (resetDescriptorSetsCount != appState.Scene.resetDescriptorSetsCount || fenceTextureResources.descriptorSets.size() < appState.Scene.fenceTextureCount)
		{
			fenceTextureResources.Delete(appState);
			auto toCreate = appState.Scene.fenceTextureCount;
			if (toCreate > 0)
			{
				poolSizes[0].descriptorCount = toCreate;
				descriptorPoolCreateInfo.maxSets = toCreate;
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &fenceTextureResources.descriptorPool));
				fenceTextureResources.descriptorSetLayouts.resize(toCreate);
				fenceTextureResources.descriptorSets.resize(toCreate);
				std::fill(fenceTextureResources.descriptorSetLayouts.begin(), fenceTextureResources.descriptorSetLayouts.end(), appState.Scene.singleImageLayout);
				descriptorSetAllocateInfo.descriptorPool = fenceTextureResources.descriptorPool;
				descriptorSetAllocateInfo.descriptorSetCount = toCreate;
				descriptorSetAllocateInfo.pSetLayouts = fenceTextureResources.descriptorSetLayouts.data();
				VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, fenceTextureResources.descriptorSets.data()));
				fenceTextureResources.created = true;
			}
		}
		if (d_lists.last_fence16 >= 0 || d_lists.last_fence32 >= 0)
		{
			VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipeline));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &vertexTransformBase));
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr));
		}
		if (d_lists.last_fence16 >= 0)
		{
			void* previousVertexes = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, surfaceIndex16Base, VK_INDEX_TYPE_UINT16));
			for (auto i = 0; i <= d_lists.last_fence16; i++)
			{
				auto& fence = d_lists.fences16[i];
				if (previousVertexes != fence.vertexes)
				{
					auto vertexBuffer = appState.Scene.fenceVertex16List[i].buffer->buffer;
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &appState.NoOffset));
					previousVertexes = fence.vertexes;
				}
				SetPushConstants(fence, pushConstants);
				VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.fences.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 20 * sizeof(float), pushConstants));
				auto texture = appState.Scene.fenceTexture16List[i].texture;
				if (previousTexture != texture)
				{
					auto descriptorSet = GetDescriptorSet(appState, texture, fenceTextureResources, textureInfo, writes);
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
					previousTexture = texture;
				}
				auto lightmap = appState.Scene.fenceLightmap16List[i].lightmap;
				VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 2, 1, &lightmap->descriptorSet, 0, nullptr));
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, fence.count, 1, fence.first_index, 0, 0));
			}
		}
		if (d_lists.last_fence32 >= 0)
		{
			void* previousVertexes = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32));
			for (auto i = 0; i <= d_lists.last_fence32; i++)
			{
				auto& fence = d_lists.fences32[i];
				if (previousVertexes != fence.vertexes)
				{
					auto vertexBuffer = appState.Scene.fenceVertex32List[i].buffer->buffer;
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &appState.NoOffset));
					previousVertexes = fence.vertexes;
				}
				SetPushConstants(fence, pushConstants);
				VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.fences.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 20 * sizeof(float), pushConstants));
				auto texture = appState.Scene.fenceTexture32List[i].texture;
				if (previousTexture != texture)
				{
					auto descriptorSet = GetDescriptorSet(appState, texture, fenceTextureResources, textureInfo, writes);
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
					previousTexture = texture;
				}
				auto lightmap = appState.Scene.fenceLightmap32List[i].lightmap;
				VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 2, 1, &lightmap->descriptorSet, 0, nullptr));
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, fence.count, 1, fence.first_index, 0, 0));
			}
		}
		if (resetDescriptorSetsCount != appState.Scene.resetDescriptorSetsCount || spriteResources.descriptorSets.size() < appState.Scene.spriteTextureCount)
		{
			spriteResources.Delete(appState);
			auto toCreate = appState.Scene.spriteTextureCount;
			if (toCreate > 0)
			{
				poolSizes[0].descriptorCount = toCreate;
				descriptorPoolCreateInfo.maxSets = toCreate;
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &spriteResources.descriptorPool));
				spriteResources.descriptorSetLayouts.resize(toCreate);
				spriteResources.descriptorSets.resize(toCreate);
				std::fill(spriteResources.descriptorSetLayouts.begin(), spriteResources.descriptorSetLayouts.end(), appState.Scene.singleImageLayout);
				descriptorSetAllocateInfo.descriptorPool = spriteResources.descriptorPool;
				descriptorSetAllocateInfo.descriptorSetCount = toCreate;
				descriptorSetAllocateInfo.pSetLayouts = spriteResources.descriptorSetLayouts.data();
				VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, spriteResources.descriptorSets.data()));
				spriteResources.created = true;
			}
		}
		if (d_lists.last_sprite >= 0)
		{
			VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipeline));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &texturedVertexBase));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &texturedAttributeBase));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &vertexTransformBase));
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
			for (auto i = 0; i <= d_lists.last_sprite; i++)
			{
				auto& sprite = d_lists.sprites[i];
				auto texture = appState.Scene.spriteList[i].texture;
				auto descriptorSet = GetDescriptorSet(appState, texture, spriteResources, textureInfo, writes);
				VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
				VC(appState.Device.vkCmdDraw(commandBuffer, sprite.count, 1, sprite.first_vertex, 0));
			}
		}
		if (d_lists.last_turbulent16 < 0 && d_lists.last_turbulent32 < 0)
		{
			turbulentResources.Delete(appState);
		}
		else
		{
			auto size = turbulentResources.descriptorSets.size();
			auto required = d_lists.last_turbulent16 + 1 + d_lists.last_turbulent32 + 1;
			if (resetDescriptorSetsCount != appState.Scene.resetDescriptorSetsCount || size < required || size > required * 2)
			{
				auto toCreate = std::max(16, required + required / 4);
				if (toCreate != size)
				{
					turbulentResources.Delete(appState);
					poolSizes[0].descriptorCount = toCreate;
					descriptorPoolCreateInfo.maxSets = toCreate;
					VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &turbulentResources.descriptorPool));
					turbulentResources.descriptorSetLayouts.resize(toCreate);
					turbulentResources.descriptorSets.resize(toCreate);
					turbulentResources.bound.resize(toCreate);
					std::fill(turbulentResources.descriptorSetLayouts.begin(), turbulentResources.descriptorSetLayouts.end(), appState.Scene.singleImageLayout);
					descriptorSetAllocateInfo.descriptorPool = turbulentResources.descriptorPool;
					descriptorSetAllocateInfo.descriptorSetCount = toCreate;
					descriptorSetAllocateInfo.pSetLayouts = turbulentResources.descriptorSetLayouts.data();
					VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, turbulentResources.descriptorSets.data()));
					turbulentResources.created = true;
				}
			}
		}
		auto descriptorSetIndex = 0;
		if (d_lists.last_turbulent16 >= 0 || d_lists.last_turbulent32 >= 0)
		{
			VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipeline));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &vertexTransformBase));
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
			pushConstants[16] = (float)cl.time;
		}
		if (d_lists.last_turbulent16 >= 0)
		{
			void* previousVertexes = nullptr;
			Texture* previousTexture = nullptr;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, surfaceIndex16Base, VK_INDEX_TYPE_UINT16));
			for (auto i = 0; i <= d_lists.last_turbulent16; i++)
			{
				auto& turbulent = d_lists.turbulent16[i];
				if (previousVertexes != turbulent.vertexes)
				{
					auto vertexBuffer = appState.Scene.turbulentVertex16List[i].buffer->buffer;
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &appState.NoOffset));
					previousVertexes = turbulent.vertexes;
				}
				SetPushConstants(turbulent, pushConstants);
				VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.turbulent.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 17 * sizeof(float), pushConstants));
				auto texture = appState.Scene.turbulent16List[i].texture;
				if (previousTexture != texture)
				{
					if (turbulentResources.bound[descriptorSetIndex] != texture)
					{
						textureInfo[0].sampler = appState.Scene.textureSamplers[texture->mipCount];
						textureInfo[0].imageView = texture->view;
						writes[0].dstSet = turbulentResources.descriptorSets[descriptorSetIndex];
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						turbulentResources.bound[descriptorSetIndex] = texture;
					}
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 1, 1, &turbulentResources.descriptorSets[descriptorSetIndex], 0, nullptr));
					descriptorSetIndex++;
					previousTexture = texture;
				}
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, turbulent.count, 1, turbulent.first_index, 0, 0));
			}
		}
		if (d_lists.last_turbulent32 >= 0)
		{
			void* previousVertexes = nullptr;
			Texture* previousTexture = nullptr;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32));
			for (auto i = 0; i <= d_lists.last_turbulent32; i++)
			{
				auto& turbulent = d_lists.turbulent32[i];
				if (previousVertexes != turbulent.vertexes)
				{
					auto vertexBuffer = appState.Scene.turbulentVertex32List[i].buffer->buffer;
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &appState.NoOffset));
					previousVertexes = turbulent.vertexes;
				}
				SetPushConstants(turbulent, pushConstants);
				VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.turbulent.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 17 * sizeof(float), pushConstants));
				auto texture = appState.Scene.turbulent32List[i].texture;
				if (previousTexture != texture)
				{
					if (turbulentResources.bound[descriptorSetIndex] != texture)
					{
						textureInfo[0].sampler = appState.Scene.textureSamplers[texture->mipCount];
						textureInfo[0].imageView = texture->view;
						writes[0].dstSet = turbulentResources.descriptorSets[descriptorSetIndex];
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						turbulentResources.bound[descriptorSetIndex] = texture;
					}
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 1, 1, &turbulentResources.descriptorSets[descriptorSetIndex], 0, nullptr));
					descriptorSetIndex++;
					previousTexture = texture;
				}
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, turbulent.count, 1, turbulent.first_index, 0, 0));
			}
		}
		if (colormapCount == 0)
		{
			colormapResources.Delete(appState);
		}
		else
		{
			auto size = colormapResources.descriptorSets.size();
			auto required = colormapCount;
			if (size < required || size > required * 2)
			{
				auto toCreate = std::max(4, required + required / 4);
				if (toCreate != size)
				{
					colormapResources.Delete(appState);
					poolSizes[0].descriptorCount = toCreate;
					descriptorPoolCreateInfo.maxSets = toCreate;
					VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &colormapResources.descriptorPool));
					colormapResources.descriptorSetLayouts.resize(toCreate);
					colormapResources.descriptorSets.resize(toCreate);
					colormapResources.bound.resize(toCreate);
					std::fill(colormapResources.descriptorSetLayouts.begin(), colormapResources.descriptorSetLayouts.end(), appState.Scene.singleImageLayout);
					descriptorSetAllocateInfo.descriptorPool = colormapResources.descriptorPool;
					descriptorSetAllocateInfo.descriptorSetCount = toCreate;
					descriptorSetAllocateInfo.pSetLayouts = colormapResources.descriptorSetLayouts.data();
					VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, colormapResources.descriptorSets.data()));
					colormapResources.created = true;
				}
			}
		}
		descriptorSetIndex = 0;
		if (resetDescriptorSetsCount != appState.Scene.resetDescriptorSetsCount || aliasResources.descriptorSets.size() < appState.Scene.aliasTextureCount)
		{
			aliasResources.Delete(appState);
			auto toCreate = appState.Scene.aliasTextureCount;
			if (toCreate > 0)
			{
				poolSizes[0].descriptorCount = toCreate;
				descriptorPoolCreateInfo.maxSets = toCreate;
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &aliasResources.descriptorPool));
				aliasResources.descriptorSetLayouts.resize(toCreate);
				aliasResources.descriptorSets.resize(toCreate);
				std::fill(aliasResources.descriptorSetLayouts.begin(), aliasResources.descriptorSetLayouts.end(), appState.Scene.singleImageLayout);
				descriptorSetAllocateInfo.descriptorPool = aliasResources.descriptorPool;
				descriptorSetAllocateInfo.descriptorSetCount = toCreate;
				descriptorSetAllocateInfo.pSetLayouts = aliasResources.descriptorSetLayouts.data();
				VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, aliasResources.descriptorSets.data()));
				aliasResources.created = true;
			}
		}
		if (d_lists.last_alias16 >= 0 || d_lists.last_alias32 >= 0)
		{
			VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipeline));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 3, 1, &attributes->buffer, &vertexTransformBase));
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
		}
		if (d_lists.last_alias16 >= 0)
		{
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, colormappedIndex16Base, VK_INDEX_TYPE_UINT16));
			for (auto i = 0; i <= d_lists.last_alias16; i++)
			{
				auto& alias = d_lists.alias16[i];
				auto index = appState.Scene.aliasVertices16List[i];
				auto& vertices = appState.Scene.colormappedBufferList[index];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer->buffer, &vertices.offset));
				index = appState.Scene.aliasTexCoords16List[i];
				auto& texCoords = appState.Scene.colormappedBufferList[index];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords.buffer->buffer, &texCoords.offset));
				VkDeviceSize attributeOffset = colormappedAttributeBase + alias.first_attribute * sizeof(float);
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
				SetPushConstants(alias, pushConstants);
				VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.alias.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants));
				if (alias.is_host_colormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr));
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = appState.Scene.alias16List[i].colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo[0].sampler = appState.Scene.textureSamplers[colormap->mipCount];
						textureInfo[0].imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr));
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = appState.Scene.alias16List[i].texture.texture;
				auto descriptorSet = GetDescriptorSet(appState, texture, aliasResources, textureInfo, writes);
				if (previousTextureDescriptorSet != descriptorSet)
				{
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 2, 1, &descriptorSet, 0, nullptr));
					previousTextureDescriptorSet = descriptorSet;
				}
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, alias.count, 1, alias.first_index, 0, 0));
			}
		}
		if (d_lists.last_alias32 >= 0)
		{
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32));
			for (auto i = 0; i <= d_lists.last_alias32; i++)
			{
				auto& alias = d_lists.alias32[i];
				auto index = appState.Scene.aliasVertices32List[i];
				auto& vertices = appState.Scene.colormappedBufferList[index];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer->buffer, &vertices.offset));
				index = appState.Scene.aliasTexCoords32List[i];
				auto& texCoords = appState.Scene.colormappedBufferList[index];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords.buffer->buffer, &texCoords.offset));
				VkDeviceSize attributeOffset = colormappedAttributeBase + alias.first_attribute * sizeof(float);
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
				SetPushConstants(alias, pushConstants);
				VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.alias.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants));
				if (alias.is_host_colormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr));
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = appState.Scene.alias32List[i].colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo[0].sampler = appState.Scene.textureSamplers[colormap->mipCount];
						textureInfo[0].imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr));
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = appState.Scene.alias32List[i].texture.texture;
				auto descriptorSet = GetDescriptorSet(appState, texture, aliasResources, textureInfo, writes);
				if (previousTextureDescriptorSet != descriptorSet)
				{
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 2, 1, &descriptorSet, 0, nullptr));
					previousTextureDescriptorSet = descriptorSet;
				}
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, alias.count, 1, alias.first_index, 0, 0));
			}
		}
		if (resetDescriptorSetsCount != appState.Scene.resetDescriptorSetsCount || viewmodelResources.descriptorSets.size() < appState.Scene.viewmodelTextureCount)
		{
			viewmodelResources.Delete(appState);
			auto toCreate = appState.Scene.viewmodelTextureCount;
			if (toCreate > 0)
			{
				poolSizes[0].descriptorCount = toCreate;
				descriptorPoolCreateInfo.maxSets = toCreate;
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &viewmodelResources.descriptorPool));
				viewmodelResources.descriptorSetLayouts.resize(toCreate);
				viewmodelResources.descriptorSets.resize(toCreate);
				std::fill(viewmodelResources.descriptorSetLayouts.begin(), viewmodelResources.descriptorSetLayouts.end(), appState.Scene.singleImageLayout);
				descriptorSetAllocateInfo.descriptorPool = viewmodelResources.descriptorPool;
				descriptorSetAllocateInfo.descriptorSetCount = toCreate;
				descriptorSetAllocateInfo.pSetLayouts = viewmodelResources.descriptorSetLayouts.data();
				VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, viewmodelResources.descriptorSets.data()));
				viewmodelResources.created = true;
			}
		}
		if (d_lists.last_viewmodel16 >= 0 || d_lists.last_viewmodel32 >= 0)
		{
			VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipeline));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 3, 1, &attributes->buffer, &vertexTransformBase));
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			if (appState.NearViewModel)
			{
				pushConstants[16] = vpn[0] / appState.Scale;
				pushConstants[17] = vpn[2] / appState.Scale;
				pushConstants[18] = -vpn[1] / appState.Scale;
				pushConstants[19] = 0;
				pushConstants[20] = 1;
				pushConstants[21] = 1;
				pushConstants[22] = 1;
				pushConstants[23] = 1;
			}
			else
			{
				pushConstants[16] = 1 / appState.Scale;
				pushConstants[17] = 0;
				pushConstants[18] = 0;
				pushConstants[19] = 8;
				pushConstants[20] = 1;
				pushConstants[21] = 0;
				pushConstants[22] = 0;
				pushConstants[23] = 0.7 + 0.3 * sin(cl.time * M_PI);
			}
		}
		if (d_lists.last_viewmodel16 >= 0)
		{
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, colormappedIndex16Base, VK_INDEX_TYPE_UINT16));
			for (auto i = 0; i <= d_lists.last_viewmodel16; i++)
			{
				auto& viewmodel = d_lists.viewmodels16[i];
				auto index = appState.Scene.viewmodelVertices16List[i];
				auto& vertices = appState.Scene.colormappedBufferList[index];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer->buffer, &vertices.offset));
				index = appState.Scene.viewmodelTexCoords16List[i];
				auto& texCoords = appState.Scene.colormappedBufferList[index];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords.buffer->buffer, &texCoords.offset));
				VkDeviceSize attributeOffset = colormappedAttributeBase + viewmodel.first_attribute * sizeof(float);
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
				SetPushConstants(viewmodel, pushConstants);
				VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.viewmodel.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 24 * sizeof(float), pushConstants));
				if (viewmodel.is_host_colormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr));
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = appState.Scene.viewmodel16List[i].colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo[0].sampler = appState.Scene.textureSamplers[colormap->mipCount];
						textureInfo[0].imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr));
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = appState.Scene.viewmodel16List[i].texture.texture;
				auto descriptorSet = GetDescriptorSet(appState, texture, viewmodelResources, textureInfo, writes);
				if (previousTextureDescriptorSet != descriptorSet)
				{
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 2, 1, &descriptorSet, 0, nullptr));
					previousTextureDescriptorSet = descriptorSet;
				}
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, viewmodel.count, 1, viewmodel.first_index, 0, 0));
			}
		}
		if (d_lists.last_viewmodel32 >= 0)
		{
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32));
			for (auto i = 0; i <= d_lists.last_viewmodel32; i++)
			{
				auto& viewmodel = d_lists.viewmodels32[i];
				auto index = appState.Scene.viewmodelVertices32List[i];
				auto& vertices = appState.Scene.colormappedBufferList[index];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer->buffer, &vertices.offset));
				index = appState.Scene.viewmodelTexCoords32List[i];
				auto& texCoords = appState.Scene.colormappedBufferList[index];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords.buffer->buffer, &texCoords.offset));
				VkDeviceSize attributeOffset = colormappedAttributeBase + viewmodel.first_attribute * sizeof(float);
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
				SetPushConstants(viewmodel, pushConstants);
				VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.viewmodel.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 24 * sizeof(float), pushConstants));
				if (viewmodel.is_host_colormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr));
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = appState.Scene.viewmodel32List[i].colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo[0].sampler = appState.Scene.textureSamplers[colormap->mipCount];
						textureInfo[0].imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr));
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = appState.Scene.viewmodel32List[i].texture.texture;
				auto descriptorSet = GetDescriptorSet(appState, texture, viewmodelResources, textureInfo, writes);
				if (previousTextureDescriptorSet != descriptorSet)
				{
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 2, 1, &descriptorSet, 0, nullptr));
					previousTextureDescriptorSet = descriptorSet;
				}
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, viewmodel.count, 1, viewmodel.first_index, 0, 0));
			}
		}
		resetDescriptorSetsCount = appState.Scene.resetDescriptorSetsCount;
		if (d_lists.last_colored_index16 >= 0 || d_lists.last_colored_index32 >= 0)
		{
			VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.colored.pipeline));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &coloredVertexBase));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &vertexTransformBase));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 2, 1, &colors->buffer, &appState.NoOffset));
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.colored.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
			if (d_lists.last_colored_index16 >= 0)
			{
				VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, coloredIndex16Base, VK_INDEX_TYPE_UINT16));
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, d_lists.last_colored_index16 + 1, 1, 0, 0, 0));
			}
			if (d_lists.last_colored_index32 >= 0)
			{
				VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, coloredIndex32Base, VK_INDEX_TYPE_UINT32));
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, d_lists.last_colored_index32 + 1, 1, 0, 0, 0));
			}
		}
		if (d_lists.last_sky >= 0)
		{
			VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipeline));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &texturedVertexBase));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &texturedAttributeBase));
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			if (!skyResources.created)
			{
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &skyResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = skyResources.descriptorPool;
				descriptorSetAllocateInfo.descriptorSetCount = 1;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
				VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &skyResources.descriptorSet));
				textureInfo[0].sampler = appState.Scene.textureSamplers[sky->mipCount];
				textureInfo[0].imageView = sky->view;
				writes[0].dstSet = skyResources.descriptorSet;
				VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
				skyResources.created = true;
			}
			VkDescriptorSet descriptorSets[2];
			descriptorSets[0] = sceneMatricesAndPaletteResources.descriptorSet;
			descriptorSets[1] = skyResources.descriptorSet;
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipelineLayout, 0, 2, descriptorSets, 0, nullptr));
			auto matrix = ovrMatrix4f_CreateFromQuaternion(&appState.Scene.orientation);
			pushConstants[0] = -matrix.M[0][2];
			pushConstants[1] = matrix.M[2][2];
			pushConstants[2] = -matrix.M[1][2];
			pushConstants[3] = matrix.M[0][0];
			pushConstants[4] = -matrix.M[2][0];
			pushConstants[5] = matrix.M[1][0];
			pushConstants[6] = matrix.M[0][1];
			pushConstants[7] = -matrix.M[2][1];
			pushConstants[8] = matrix.M[1][1];
			pushConstants[9] = appState.EyeTextureWidth;
			pushConstants[10] = appState.EyeTextureHeight;
			pushConstants[11] = appState.EyeTextureMaxDimension;
			pushConstants[12] = skytime*skyspeed;
			VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.sky.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 13 * sizeof(float), &pushConstants));
			for (auto i = 0; i <= d_lists.last_sky; i++)
			{
				auto& sky = d_lists.sky[i];
				VC(appState.Device.vkCmdDraw(commandBuffer, sky.count, 1, sky.first_vertex, 0));
			}
		}
	}
	else
	{
		VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipeline));
		VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset));
		VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &appState.NoOffset));
		if (!floorResources.created)
		{
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &floorResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = floorResources.descriptorPool;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
			VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &floorResources.descriptorSet));
			textureInfo[0].sampler = appState.Scene.textureSamplers[appState.Scene.floorTexture.mipCount];
			textureInfo[0].imageView = appState.Scene.floorTexture.view;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].pImageInfo = textureInfo;
			writes[0].dstSet = floorResources.descriptorSet;
			VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
			floorResources.created = true;
		}
		VkDescriptorSet descriptorSets[2];
		descriptorSets[0] = sceneMatricesResources.descriptorSet;
		descriptorSets[1] = floorResources.descriptorSet;
		VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipelineLayout, 0, 2, descriptorSets, 0, nullptr));
		VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, 0, VK_INDEX_TYPE_UINT16));
		VC(appState.Device.vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0));
	}
	if (appState.Scene.controllerVerticesSize > 0)
	{
		VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipeline));
		VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &controllerVertexBase));
		VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &controllerAttributeBase));
		if (!controllerResources.created)
		{
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &controllerResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = controllerResources.descriptorPool;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
			VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &controllerResources.descriptorSet));
			textureInfo[0].sampler = appState.Scene.textureSamplers[appState.Scene.controllerTexture.mipCount];
			textureInfo[0].imageView = appState.Scene.controllerTexture.view;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].pImageInfo = textureInfo;
			writes[0].dstSet = controllerResources.descriptorSet;
			VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
			controllerResources.created = true;
		}
		VkDescriptorSet descriptorSets[2];
		descriptorSets[0] = sceneMatricesResources.descriptorSet;
		descriptorSets[1] = controllerResources.descriptorSet;
		VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipelineLayout, 0, 2, descriptorSets, 0, nullptr));
		VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, controllerIndex16Base, VK_INDEX_TYPE_UINT16));
		VkDeviceSize size = 0;
		if (appState.LeftController.TrackingResult == ovrSuccess)
		{
			size += 2 * 36;
		}
		if (appState.RightController.TrackingResult == ovrSuccess)
		{
			size += 2 * 36;
		}
		VC(appState.Device.vkCmdDrawIndexed(commandBuffer, size, 1, 0, 0, 0));
	}
}
