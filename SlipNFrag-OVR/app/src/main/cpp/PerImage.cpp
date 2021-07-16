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
	colormaps.Reset(appState);
	colormapCount = 0;
	vertices = nullptr;
	attributes = nullptr;
	indices16 = nullptr;
	indices32 = nullptr;
	colors = nullptr;
}

void PerImage::GetSurfaceVerticesStagingBufferSize(AppState& appState, dsurface_t& surface, LoadedSharedMemoryBuffer& loaded, VkDeviceSize& stagingBufferSize)
{
	auto vertexes = surface.vertexes;
	if (appState.Scene.previousVertexes != vertexes)
	{
		auto entry = appState.Scene.verticesPerKey.find(vertexes);
		if (entry == appState.Scene.verticesPerKey.end())
		{
			loaded.size = surface.vertex_count * 3 * sizeof(float);
			loaded.buffer = new SharedMemoryBuffer { };
			loaded.buffer->CreateVertexBuffer(appState, loaded.size);
			appState.Scene.verticesPerKey.insert({ vertexes, loaded.buffer });
			stagingBufferSize += loaded.size;
			appState.Scene.buffers.MoveToFront(loaded.buffer);
			loaded.source = surface.vertexes;
			appState.Scene.buffers.SetupVertices(appState, loaded);
		}
		else
		{
			loaded.size = 0;
			loaded.buffer = entry->second;
		}
		appState.Scene.previousVertexes = vertexes;
		appState.Scene.previousVertexBuffer = loaded.buffer;
	}
	else
	{
		loaded.size = 0;
		loaded.buffer = appState.Scene.previousVertexBuffer;
	}
}

void PerImage::GetTurbulentVerticesStagingBufferSize(AppState& appState, dturbulent_t& turbulent, LoadedSharedMemoryBuffer& loaded, VkDeviceSize& stagingBufferSize)
{
	auto vertexes = turbulent.vertexes;
	if (appState.Scene.previousVertexes != vertexes)
	{
		auto entry = appState.Scene.verticesPerKey.find(vertexes);
		if (entry == appState.Scene.verticesPerKey.end())
		{
			loaded.size = turbulent.vertex_count * 3 * sizeof(float);
			loaded.buffer = new SharedMemoryBuffer { };
			loaded.buffer->CreateVertexBuffer(appState, loaded.size);
			appState.Scene.verticesPerKey.insert({ vertexes, loaded.buffer });
			stagingBufferSize += loaded.size;
			appState.Scene.buffers.MoveToFront(loaded.buffer);
			loaded.source = turbulent.vertexes;
			appState.Scene.buffers.SetupVertices(appState, loaded);
		}
		else
		{
			loaded.size = 0;
			loaded.buffer = entry->second;
		}
		appState.Scene.previousVertexes = vertexes;
		appState.Scene.previousVertexBuffer = loaded.buffer;
	}
	else
	{
		loaded.size = 0;
		loaded.buffer = appState.Scene.previousVertexBuffer;
	}
}

void PerImage::GetAliasVerticesStagingBufferSize(AppState& appState, dalias_t& alias, LoadedSharedMemoryBuffer& loadedVertices, LoadedSharedMemoryTexCoordsBuffer& loadedTexCoords, VkDeviceSize& stagingBufferSize)
{
	auto vertices = alias.vertices;
	if (appState.Scene.previousVertexes != vertices)
	{
		auto entry = appState.Scene.verticesPerKey.find(vertices);
		if (entry == appState.Scene.verticesPerKey.end())
		{
			loadedVertices.size = alias.vertex_count * 2 * 4 * sizeof(float);
			loadedVertices.buffer = new SharedMemoryBuffer { };
			loadedVertices.buffer->CreateVertexBuffer(appState, loadedVertices.size);
			appState.Scene.verticesPerKey.insert({ vertices, loadedVertices.buffer });
			stagingBufferSize += loadedVertices.size;
			appState.Scene.buffers.MoveToFront(loadedVertices.buffer);
			loadedVertices.source = alias.vertices;
			appState.Scene.buffers.SetupAliasVertices(appState, loadedVertices);
			loadedTexCoords.size = alias.vertex_count * 2 * 2 * sizeof(float);
			loadedTexCoords.buffer = new SharedMemoryBuffer { };
			loadedTexCoords.buffer->CreateVertexBuffer(appState, loadedTexCoords.size);
			appState.Scene.texCoordsPerKey.insert({ vertices, loadedTexCoords.buffer });
			stagingBufferSize += loadedTexCoords.size;
			appState.Scene.buffers.MoveToFront(loadedTexCoords.buffer);
			loadedTexCoords.source = alias.texture_coordinates;
			loadedTexCoords.width = alias.width;
			loadedTexCoords.height = alias.height;
			appState.Scene.buffers.SetupAliasTexCoords(appState, loadedTexCoords);
		}
		else
		{
			loadedVertices.size = 0;
			loadedVertices.buffer = entry->second;
			loadedTexCoords.size = 0;
			loadedTexCoords.buffer = appState.Scene.texCoordsPerKey[vertices];
		}
		appState.Scene.previousVertexes = vertices;
		appState.Scene.previousVertexBuffer = loadedVertices.buffer;
		appState.Scene.previousTexCoordsBuffer = loadedTexCoords.buffer;
	}
	else
	{
		loadedVertices.size = 0;
		loadedVertices.buffer = appState.Scene.previousVertexBuffer;
		loadedTexCoords.size = 0;
		loadedTexCoords.buffer = appState.Scene.previousTexCoordsBuffer;
	}
}

void PerImage::GetSurfaceLightmapStagingBufferSize(AppState& appState, View& view, dsurface_t& surface, LoadedLightmap& loaded, VkDeviceSize& stagingBufferSize)
{
	auto entry = appState.Scene.lightmaps.lightmaps.find({ surface.surface, surface.entity });
	if (entry == appState.Scene.lightmaps.lightmaps.end())
	{
		auto lightmap = new Lightmap();
		lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		lightmap->key.first = surface.surface;
		lightmap->key.second = surface.entity;
		appState.Scene.lightmaps.lightmaps.insert({ lightmap->key, lightmap });
		loaded.lightmap = lightmap;
		loaded.size = surface.lightmap_size * sizeof(float);
		stagingBufferSize += loaded.size;
		loaded.source = surface.lightmap;
		appState.Scene.lightmaps.Setup(appState, loaded);
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
				loaded.lightmap = first;
			}
			else
			{
				auto lightmap = new Lightmap();
				lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				lightmap->key.first = surface.surface;
				lightmap->key.second = surface.entity;
				lightmap->next = first;
				entry->second = lightmap;
				loaded.lightmap = lightmap;
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
					loaded.lightmap = lightmap;
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
				loaded.lightmap = lightmap;
			}
		}
		loaded.size = surface.lightmap_size * sizeof(float);
		stagingBufferSize += loaded.size;
		loaded.source = surface.lightmap;
		appState.Scene.lightmaps.Setup(appState, loaded);
		surface.created = 0;
	}
	else
	{
		auto lightmap = entry->second;
		lightmap->unusedCount = 0;
		loaded.lightmap = lightmap;
		loaded.size = 0;
	}
}

void PerImage::GetSurfaceTextureStagingBufferSize(AppState& appState, int lastSurface, std::vector<dsurface_t>& surfaceList, std::vector<LoadedSharedMemoryTexture>& textures, VkDeviceSize& stagingBufferSize)
{
	if (lastSurface < 0)
	{
		return;
	}
	if (lastSurface >= textures.size())
	{
		textures.resize(lastSurface + 1);
	}
	for (auto i = 0; i <= lastSurface; i++)
	{
		auto& surface = surfaceList[i];
		auto& loaded = textures[i];
		auto entry = appState.Scene.surfaceTexturesPerKey.find(surface.texture);
		if (entry == appState.Scene.surfaceTexturesPerKey.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(surface.texture_width, surface.texture_height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, surface.texture_width, surface.texture_height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			loaded.size = surface.texture_size;
			stagingBufferSize += loaded.size;
			appState.Scene.textures.MoveToFront(texture);
			appState.Scene.surfaceTexturesPerKey.insert({ surface.texture, texture });
			loaded.texture = texture;
			loaded.source = surface.texture;
			appState.Scene.textures.Setup(appState, loaded);
		}
		else
		{
			loaded.size = 0;
			loaded.texture = entry->second;
		}
	}
}

void PerImage::GetFenceTextureStagingBufferSize(AppState& appState, int lastFence, std::vector<dsurface_t>& fenceList, std::vector<LoadedSharedMemoryTexture>& textures, VkDeviceSize& stagingBufferSize)
{
	if (lastFence < 0)
	{
		return;
	}
	if (lastFence >= textures.size())
	{
		textures.resize(lastFence + 1);
	}
	for (auto i = 0; i <= lastFence; i++)
	{
		auto& fence = fenceList[i];
		auto& loaded = textures[i];
		auto entry = appState.Scene.fenceTexturesPerKey.find(fence.texture);
		if (entry == appState.Scene.fenceTexturesPerKey.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(fence.texture_width, fence.texture_height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, fence.texture_width, fence.texture_height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			loaded.size = fence.texture_size;
			stagingBufferSize += loaded.size;
			appState.Scene.textures.MoveToFront(texture);
			appState.Scene.fenceTexturesPerKey.insert({ fence.texture, texture });
			loaded.texture = texture;
			loaded.source = fence.texture;
			appState.Scene.textures.Setup(appState, loaded);
		}
		else
		{
			loaded.size = 0;
			loaded.texture = entry->second;
		}
	}
}

void PerImage::GetTurbulentStagingBufferSize(AppState& appState, int lastTurbulent, std::vector<dturbulent_t>& turbulentList, std::vector<LoadedSharedMemoryTexture>& textures, VkDeviceSize& stagingBufferSize)
{
	if (lastTurbulent < 0)
	{
		return;
	}
	if (lastTurbulent >= textures.size())
	{
		textures.resize(lastTurbulent + 1);
	}
	for (auto i = 0; i <= lastTurbulent; i++)
	{
		auto& turbulent = turbulentList[i];
		auto& loaded = textures[i];
		auto entry = appState.Scene.turbulentPerKey.find(turbulent.texture);
		if (entry == appState.Scene.turbulentPerKey.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, turbulent.width, turbulent.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			loaded.size = turbulent.size;
			stagingBufferSize += loaded.size;
			appState.Scene.textures.MoveToFront(texture);
			appState.Scene.turbulentPerKey.insert({ turbulent.texture, texture });
			loaded.texture = texture;
		}
		else
		{
			loaded.size = 0;
			loaded.texture = entry->second;
		}
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
	if (lastAlias < 0)
	{
		return;
	}
	if (lastAlias >= textures.size())
	{
		textures.resize(lastAlias + 1);
	}
	for (auto i = 0; i <= lastAlias; i++)
	{
		auto& alias = aliasList[i];
		auto& loaded = textures[i];
		auto entry = appState.Scene.aliasTexturesPerKey.find(alias.data);
		if (entry == appState.Scene.aliasTexturesPerKey.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(alias.width, alias.height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, alias.width, alias.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			loaded.texture.size = alias.size;
			stagingBufferSize += loaded.texture.size;
			appState.Scene.textures.MoveToFront(texture);
			appState.Scene.aliasTexturesPerKey.insert({ alias.data, texture });
			loaded.texture.texture = texture;
		}
		else
		{
			loaded.texture.size = 0;
			loaded.texture.texture = entry->second;
		}
	}
	GetAliasColormapStagingBufferSize(appState, lastAlias, aliasList, textures, stagingBufferSize);
}

void PerImage::GetViewmodelStagingBufferSize(AppState& appState, int lastViewmodel, std::vector<dalias_t>& viewmodelList, std::vector<LoadedColormappedTexture>& textures, VkDeviceSize& stagingBufferSize)
{
	if (lastViewmodel < 0)
	{
		return;
	}
	if (lastViewmodel >= textures.size())
	{
		textures.resize(lastViewmodel + 1);
	}
	for (auto i = 0; i <= lastViewmodel; i++)
	{
		auto& viewmodel = viewmodelList[i];
		auto& loaded = textures[i];
		auto entry = appState.Scene.viewmodelTexturesPerKey.find(viewmodel.data);
		if (entry == appState.Scene.viewmodelTexturesPerKey.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(viewmodel.width, viewmodel.height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, viewmodel.width, viewmodel.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			loaded.texture.size = viewmodel.size;
			stagingBufferSize += loaded.texture.size;
			appState.Scene.textures.MoveToFront(texture);
			appState.Scene.viewmodelTexturesPerKey.insert({ viewmodel.data, texture });
			loaded.texture.texture = texture;
		}
		else
		{
			loaded.texture.size = 0;
			loaded.texture.texture = entry->second;
		}
	}
	GetAliasColormapStagingBufferSize(appState, lastViewmodel, viewmodelList, textures, stagingBufferSize);
}

VkDeviceSize PerImage::GetStagingBufferSize(AppState& appState, View& view)
{
	auto size = appState.Scene.matrices.size;
	if (d_lists.last_surface16 >= appState.Scene.surfaceVertex16List.size())
	{
		appState.Scene.surfaceVertex16List.resize(d_lists.last_surface16 + 1);
	}
	for (auto i = 0; i <= d_lists.last_surface16; i++)
	{
		auto& surface = d_lists.surfaces16[i];
		GetSurfaceVerticesStagingBufferSize(appState, surface, appState.Scene.surfaceVertex16List[i], size);
	}
	if (d_lists.last_surface32 >= appState.Scene.surfaceVertex32List.size())
	{
		appState.Scene.surfaceVertex32List.resize(d_lists.last_surface32 + 1);
	}
	for (auto i = 0; i <= d_lists.last_surface32; i++)
	{
		auto& surface = d_lists.surfaces32[i];
		GetSurfaceVerticesStagingBufferSize(appState, surface, appState.Scene.surfaceVertex32List[i], size);
	}
	if (d_lists.last_fence16 >= appState.Scene.fenceVertex16List.size())
	{
		appState.Scene.fenceVertex16List.resize(d_lists.last_fence16 + 1);
	}
	for (auto i = 0; i <= d_lists.last_fence16; i++)
	{
		auto& fence = d_lists.fences16[i];
		GetSurfaceVerticesStagingBufferSize(appState, fence, appState.Scene.fenceVertex16List[i], size);
	}
	if (d_lists.last_fence32 >= appState.Scene.fenceVertex32List.size())
	{
		appState.Scene.fenceVertex32List.resize(d_lists.last_fence32 + 1);
	}
	for (auto i = 0; i <= d_lists.last_fence32; i++)
	{
		auto& fence = d_lists.fences32[i];
		GetSurfaceVerticesStagingBufferSize(appState, fence, appState.Scene.fenceVertex32List[i], size);
	}
	if (d_lists.last_turbulent16 >= appState.Scene.turbulentVertex16List.size())
	{
		appState.Scene.turbulentVertex16List.resize(d_lists.last_turbulent16 + 1);
	}
	for (auto i = 0; i <= d_lists.last_turbulent16; i++)
	{
		auto& turbulent = d_lists.turbulent16[i];
		GetTurbulentVerticesStagingBufferSize(appState, turbulent, appState.Scene.turbulentVertex16List[i], size);
	}
	if (d_lists.last_turbulent32 >= appState.Scene.turbulentVertex32List.size())
	{
		appState.Scene.turbulentVertex32List.resize(d_lists.last_turbulent32 + 1);
	}
	for (auto i = 0; i <= d_lists.last_turbulent32; i++)
	{
		auto& turbulent = d_lists.turbulent32[i];
		GetTurbulentVerticesStagingBufferSize(appState, turbulent, appState.Scene.turbulentVertex32List[i], size);
	}
	if (d_lists.last_alias16 >= appState.Scene.aliasVertex16List.size())
	{
		appState.Scene.aliasVertex16List.resize(d_lists.last_alias16 + 1);
		appState.Scene.aliasTexCoords16List.resize(d_lists.last_alias16 + 1);
	}
	for (auto i = 0; i <= d_lists.last_alias16; i++)
	{
		auto& alias = d_lists.alias16[i];
		GetAliasVerticesStagingBufferSize(appState, alias, appState.Scene.aliasVertex16List[i], appState.Scene.aliasTexCoords16List[i], size);
	}
	if (d_lists.last_alias32 >= appState.Scene.aliasVertex32List.size())
	{
		appState.Scene.aliasVertex32List.resize(d_lists.last_alias32 + 1);
		appState.Scene.aliasTexCoords32List.resize(d_lists.last_alias32 + 1);
	}
	for (auto i = 0; i <= d_lists.last_alias32; i++)
	{
		auto& alias = d_lists.alias32[i];
		GetAliasVerticesStagingBufferSize(appState, alias, appState.Scene.aliasVertex32List[i], appState.Scene.aliasTexCoords32List[i], size);
	}
	if (d_lists.last_viewmodel16 >= appState.Scene.viewmodelVertex16List.size())
	{
		appState.Scene.viewmodelVertex16List.resize(d_lists.last_viewmodel16 + 1);
		appState.Scene.viewmodelTexCoords16List.resize(d_lists.last_viewmodel16 + 1);
	}
	for (auto i = 0; i <= d_lists.last_viewmodel16; i++)
	{
		auto& viewmodel = d_lists.viewmodels16[i];
		GetAliasVerticesStagingBufferSize(appState, viewmodel, appState.Scene.viewmodelVertex16List[i], appState.Scene.viewmodelTexCoords16List[i], size);
	}
	if (d_lists.last_viewmodel32 >= appState.Scene.viewmodelVertex32List.size())
	{
		appState.Scene.viewmodelVertex32List.resize(d_lists.last_viewmodel32 + 1);
		appState.Scene.viewmodelTexCoords32List.resize(d_lists.last_viewmodel32 + 1);
	}
	for (auto i = 0; i <= d_lists.last_viewmodel32; i++)
	{
		auto& viewmodel = d_lists.viewmodels32[i];
		GetAliasVerticesStagingBufferSize(appState, viewmodel, appState.Scene.viewmodelVertex32List[i], appState.Scene.viewmodelTexCoords32List[i], size);
	}
	appState.Scene.paletteSize = 0;
	if (palette == nullptr || paletteChanged != pal_changed)
	{
		if (palette == nullptr)
		{
			palette = new Texture();
			palette->Create(appState, 256, 1, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		appState.Scene.paletteSize = 1024;
		size += appState.Scene.paletteSize;
		paletteChanged = pal_changed;
	}
	appState.Scene.host_colormapSize = 0;
	if (::host_colormap.size() > 0 && host_colormap == nullptr)
	{
		host_colormap = new Texture();
		host_colormap->Create(appState, 256, 64, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		appState.Scene.host_colormapSize = 16384;
		size += appState.Scene.host_colormapSize;
	}
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
	GetSurfaceTextureStagingBufferSize(appState, d_lists.last_surface16, d_lists.surfaces16, appState.Scene.surfaceTexture16List, size);
	GetSurfaceTextureStagingBufferSize(appState, d_lists.last_surface32, d_lists.surfaces32, appState.Scene.surfaceTexture32List, size);
	GetFenceTextureStagingBufferSize(appState, d_lists.last_fence16, d_lists.fences16, appState.Scene.fenceTexture16List, size);
	GetFenceTextureStagingBufferSize(appState, d_lists.last_fence32, d_lists.fences32, appState.Scene.fenceTexture32List, size);
	if (d_lists.last_sprite >= appState.Scene.spriteList.size())
	{
		appState.Scene.spriteList.resize(d_lists.last_sprite + 1);
	}
	for (auto i = 0; i <= d_lists.last_sprite; i++)
	{
		auto& sprite = d_lists.sprites[i];
		auto& loaded = appState.Scene.spriteList[i];
		auto entry = appState.Scene.spritesPerKey.find(sprite.data);
		if (entry == appState.Scene.spritesPerKey.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(sprite.width, sprite.height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, sprite.width, sprite.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			loaded.size = sprite.size;
			size += loaded.size;
			appState.Scene.textures.MoveToFront(texture);
			appState.Scene.spritesPerKey.insert({ sprite.data, texture });
			loaded.texture = texture;
			loaded.source = sprite.data;
			appState.Scene.textures.Setup(appState, loaded);
		}
		else
		{
			loaded.size = 0;
			loaded.texture = entry->second;
		}
	}
	GetTurbulentStagingBufferSize(appState, d_lists.last_turbulent16, d_lists.turbulent16, appState.Scene.turbulent16List, size);
	GetTurbulentStagingBufferSize(appState, d_lists.last_turbulent32, d_lists.turbulent32, appState.Scene.turbulent32List, size);
	GetAliasStagingBufferSize(appState, d_lists.last_alias16, d_lists.alias16, appState.Scene.alias16List, size);
	GetAliasStagingBufferSize(appState, d_lists.last_alias32, d_lists.alias32, appState.Scene.alias32List, size);
	GetViewmodelStagingBufferSize(appState, d_lists.last_viewmodel16, d_lists.viewmodels16, appState.Scene.viewmodel16List, size);
	GetViewmodelStagingBufferSize(appState, d_lists.last_viewmodel32, d_lists.viewmodels32, appState.Scene.viewmodel32List, size);
	appState.Scene.skySize = 0;
	if (d_lists.last_sky >= 0)
	{
		if (sky == nullptr)
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(128, 128)))) + 1;
			sky = new Texture();
			sky->Create(appState, 128, 128, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		appState.Scene.skySize = 16384;
		size += appState.Scene.skySize;
	}
	return size;
}

void PerImage::LoadStagingBuffer(AppState& appState, int matrixIndex, Buffer* stagingBuffer)
{
	VkDeviceSize offset = 0;
	size_t matricesSize = appState.Scene.numBuffers * sizeof(ovrMatrix4f);
	memcpy(stagingBuffer->mapped, &appState.ViewMatrices[matrixIndex], matricesSize);
	offset += matricesSize;
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, &appState.ProjectionMatrices[matrixIndex], matricesSize);
	offset += matricesSize;
	auto loadedBuffer = appState.Scene.buffers.firstVertices;
	while (loadedBuffer != nullptr)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, loadedBuffer->source, loadedBuffer->size);
		offset += loadedBuffer->size;
		loadedBuffer = loadedBuffer->next;
	}
	loadedBuffer = appState.Scene.buffers.firstAliasVertices;
	while (loadedBuffer != nullptr)
	{
		auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto source = (trivertx_t*)loadedBuffer->source;
		for (auto i = 0; i < loadedBuffer->size; i += 2 * 4 * sizeof(float))
		{
			auto x = (float)(source->v[0]);
			auto y = (float)(source->v[1]);
			auto z = (float)(source->v[2]);
			*target++ = x;
			*target++ = z;
			*target++ = -y;
			*target++ = 1;
			*target++ = x;
			*target++ = z;
			*target++ = -y;
			*target++ = 1;
			source++;
		}
		offset += loadedBuffer->size;
		loadedBuffer = loadedBuffer->next;
	}
	auto loadedTexCoordsBuffer = appState.Scene.buffers.firstAliasTexCoords;
	while (loadedTexCoordsBuffer != nullptr)
	{
		auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto source = (stvert_t*)loadedTexCoordsBuffer->source;
		for (auto i = 0; i < loadedTexCoordsBuffer->size; i += 2 * 2 * sizeof(float))
		{
			auto s = (float)(source->s >> 16);
			auto t = (float)(source->t >> 16);
			s /= loadedTexCoordsBuffer->width;
			t /= loadedTexCoordsBuffer->height;
			*target++ = s;
			*target++ = t;
			*target++ = s + 0.5;
			*target++ = t;
			source++;
		}
		offset += loadedTexCoordsBuffer->size;
		loadedTexCoordsBuffer = loadedTexCoordsBuffer->next;
	}
	if (appState.Scene.paletteSize > 0)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_8to24table, appState.Scene.paletteSize);
		offset += appState.Scene.paletteSize;
	}
	if (appState.Scene.host_colormapSize > 0)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, ::host_colormap.data(), appState.Scene.host_colormapSize);
		offset += appState.Scene.host_colormapSize;
	}
	auto lightmap = appState.Scene.lightmaps.first;
	while (lightmap != nullptr)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, lightmap->source, lightmap->size);
		offset += lightmap->size;
		lightmap = lightmap->next;
	}
	auto sharedMemoryTexture = appState.Scene.textures.first;
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
	if (appState.Scene.skySize > 0)
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
	auto loadedBuffer = appState.Scene.buffers.firstVertices;
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
	loadedBuffer = appState.Scene.buffers.firstAliasVertices;
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
	auto loadedTexCoordsBuffer = appState.Scene.buffers.firstAliasTexCoords;
	while (loadedTexCoordsBuffer != nullptr)
	{
		appState.Scene.stagingBuffer.lastBarrier++;
		if (appState.Scene.stagingBuffer.bufferBarriers.size() <= appState.Scene.stagingBuffer.lastBarrier)
		{
			appState.Scene.stagingBuffer.bufferBarriers.emplace_back();
		}
		bufferCopy.size = loadedTexCoordsBuffer->size;
		VC(appState.Device.vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, loadedTexCoordsBuffer->buffer->buffer, 1, &bufferCopy));
		auto& barrier = appState.Scene.stagingBuffer.bufferBarriers[appState.Scene.stagingBuffer.lastBarrier];
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.buffer = loadedTexCoordsBuffer->buffer->buffer;
		barrier.size = VK_WHOLE_SIZE;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		bufferCopy.srcOffset += loadedTexCoordsBuffer->size;
		loadedTexCoordsBuffer = loadedTexCoordsBuffer->next;
	}
	if (appState.Scene.stagingBuffer.lastBarrier >= 0)
	{
		VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, appState.Scene.stagingBuffer.lastBarrier + 1, appState.Scene.stagingBuffer.bufferBarriers.data(), 0, nullptr));
	}
	appState.Scene.stagingBuffer.buffer = stagingBuffer;
	appState.Scene.stagingBuffer.offset = bufferCopy.srcOffset;
	appState.Scene.stagingBuffer.commandBuffer = commandBuffer;
	appState.Scene.stagingBuffer.lastBarrier = -1;
	if (appState.Scene.paletteSize > 0)
	{
		palette->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += appState.Scene.paletteSize;
	}
	if (appState.Scene.host_colormapSize > 0)
	{
		host_colormap->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += appState.Scene.host_colormapSize;
	}
	auto lightmap = appState.Scene.lightmaps.first;
	while (lightmap != nullptr)
	{
		lightmap->lightmap->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += lightmap->size;
		lightmap = lightmap->next;
	}
	auto sharedMemoryTexture = appState.Scene.textures.first;
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
	if (appState.Scene.skySize > 0)
	{
		sky->FillMipmapped(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += appState.Scene.skySize;
	}
	if (appState.Scene.stagingBuffer.lastBarrier >= 0)
	{
		VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, appState.Scene.stagingBuffer.lastBarrier + 1, appState.Scene.stagingBuffer.imageBarriers.data()));
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
	if (key_dest == key_console || key_dest == key_menu || appState.Mode != AppWorldMode)
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
	pushConstants[0] = surface.vecs[0][0];
	pushConstants[1] = surface.vecs[0][1];
	pushConstants[2] = surface.vecs[0][2];
	pushConstants[3] = surface.vecs[0][3];
	pushConstants[4] = surface.vecs[1][0];
	pushConstants[5] = surface.vecs[1][1];
	pushConstants[6] = surface.vecs[1][2];
	pushConstants[7] = surface.vecs[1][3];
	pushConstants[8] = surface.texturemins[0];
	pushConstants[9] = surface.texturemins[1];
	pushConstants[10] = surface.extents[0];
	pushConstants[11] = surface.extents[1];
	pushConstants[12] = surface.texture_width;
	pushConstants[13] = surface.texture_height;
	pushConstants[14] = surface.origin_x;
	pushConstants[15] = surface.origin_y;
	pushConstants[16] = surface.origin_z;
	pushConstants[17] = surface.yaw * M_PI / 180;
	pushConstants[18] = surface.pitch * M_PI / 180;
	pushConstants[19] = -surface.roll * M_PI / 180;
}

void PerImage::SetPushConstants(dturbulent_t& turbulent, float pushConstants[])
{
	pushConstants[0] = turbulent.vecs[0][0];
	pushConstants[1] = turbulent.vecs[0][1];
	pushConstants[2] = turbulent.vecs[0][2];
	pushConstants[3] = turbulent.vecs[0][3];
	pushConstants[4] = turbulent.vecs[1][0];
	pushConstants[5] = turbulent.vecs[1][1];
	pushConstants[6] = turbulent.vecs[1][2];
	pushConstants[7] = turbulent.vecs[1][3];
	pushConstants[8] = turbulent.width;
	pushConstants[9] = turbulent.height;
	pushConstants[10] = turbulent.origin_x;
	pushConstants[11] = turbulent.origin_y;
	pushConstants[12] = turbulent.origin_z;
	pushConstants[13] = turbulent.yaw * M_PI / 180;
	pushConstants[14] = turbulent.pitch * M_PI / 180;
	pushConstants[15] = -turbulent.roll * M_PI / 180;
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
		textureInfo[0].sampler = appState.Scene.samplers[host_colormap->mipCount];
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
			textureInfo[0].sampler = appState.Scene.samplers[palette->mipCount];
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
				textureInfo[1].sampler = appState.Scene.samplers[host_colormap->mipCount];
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
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr));
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
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr));
					previousTexture = texture;
				}
				auto lightmap = appState.Scene.surfaceLightmap32List[i].lightmap;
				VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 2, 1, &lightmap->descriptorSet, 0, nullptr));
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, surface.count, 1, surface.first_index, 0, 0));
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
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr));
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
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr));
					previousTexture = texture;
				}
				auto lightmap = appState.Scene.fenceLightmap32List[i].lightmap;
				VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 2, 1, &lightmap->descriptorSet, 0, nullptr));
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, fence.count, 1, fence.first_index, 0, 0));
			}
		}
		if (d_lists.last_sprite >= 0)
		{
			VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipeline));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &texturedVertexBase));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &texturedAttributeBase));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &vertexTransformBase));
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
			SharedMemoryTexture* previousTexture = nullptr;
			for (auto i = 0; i <= d_lists.last_sprite; i++)
			{
				auto& sprite = d_lists.sprites[i];
				auto texture = appState.Scene.spriteList[i].texture;
				if (previousTexture != texture)
				{
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr));
					previousTexture = texture;
				}
				VC(appState.Device.vkCmdDraw(commandBuffer, sprite.count, 1, sprite.first_vertex, 0));
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
			SharedMemoryTexture* previousTexture = nullptr;
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
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr));
					previousTexture = texture;
				}
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, turbulent.count, 1, turbulent.first_index, 0, 0));
			}
		}
		if (d_lists.last_turbulent32 >= 0)
		{
			void* previousVertexes = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
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
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr));
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
			SharedMemoryTexture* previousTexture = nullptr;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, colormappedIndex16Base, VK_INDEX_TYPE_UINT16));
			for (auto i = 0; i <= d_lists.last_alias16; i++)
			{
				auto& alias = d_lists.alias16[i];
				auto& vertexBuffer = appState.Scene.aliasVertex16List[i];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer->buffer, &appState.NoOffset));
				auto& texCoordsBuffer = appState.Scene.aliasTexCoords16List[i];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoordsBuffer.buffer->buffer, &appState.NoOffset));
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
						textureInfo[0].sampler = appState.Scene.samplers[colormap->mipCount];
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
				if (previousTexture != texture)
				{
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr));
					previousTexture = texture;
				}
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, alias.count, 1, alias.first_index, 0, 0));
			}
		}
		if (d_lists.last_alias32 >= 0)
		{
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32));
			for (auto i = 0; i <= d_lists.last_alias32; i++)
			{
				auto& alias = d_lists.alias32[i];
				auto& vertexBuffer = appState.Scene.aliasVertex32List[i];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer->buffer, &appState.NoOffset));
				auto& texCoordsBuffer = appState.Scene.aliasTexCoords32List[i];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoordsBuffer.buffer->buffer, &appState.NoOffset));
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
						textureInfo[0].sampler = appState.Scene.samplers[colormap->mipCount];
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
				if (previousTexture != texture)
				{
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr));
					previousTexture = texture;
				}
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, alias.count, 1, alias.first_index, 0, 0));
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
			SharedMemoryTexture* previousTexture = nullptr;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, colormappedIndex16Base, VK_INDEX_TYPE_UINT16));
			for (auto i = 0; i <= d_lists.last_viewmodel16; i++)
			{
				auto& viewmodel = d_lists.viewmodels16[i];
				auto& vertexBuffer = appState.Scene.viewmodelVertex16List[i];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer->buffer, &appState.NoOffset));
				auto& texCoordsBuffer = appState.Scene.viewmodelTexCoords16List[i];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoordsBuffer.buffer->buffer, &appState.NoOffset));
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
						textureInfo[0].sampler = appState.Scene.samplers[colormap->mipCount];
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
				if (previousTexture != texture)
				{
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr));
					previousTexture = texture;
				}
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, viewmodel.count, 1, viewmodel.first_index, 0, 0));
			}
		}
		if (d_lists.last_viewmodel32 >= 0)
		{
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32));
			for (auto i = 0; i <= d_lists.last_viewmodel32; i++)
			{
				auto& viewmodel = d_lists.viewmodels32[i];
				auto& vertexBuffer = appState.Scene.viewmodelVertex32List[i];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer->buffer, &appState.NoOffset));
				auto& texCoordsBuffer = appState.Scene.viewmodelTexCoords32List[i];
				VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoordsBuffer.buffer->buffer, &appState.NoOffset));
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
						textureInfo[0].sampler = appState.Scene.samplers[colormap->mipCount];
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
				if (previousTexture != texture)
				{
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr));
					previousTexture = texture;
				}
				VC(appState.Device.vkCmdDrawIndexed(commandBuffer, viewmodel.count, 1, viewmodel.first_index, 0, 0));
			}
		}
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
				textureInfo[0].sampler = appState.Scene.samplers[sky->mipCount];
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
			textureInfo[0].sampler = appState.Scene.samplers[appState.Scene.floorTexture.mipCount];
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
			textureInfo[0].sampler = appState.Scene.samplers[appState.Scene.controllerTexture.mipCount];
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
