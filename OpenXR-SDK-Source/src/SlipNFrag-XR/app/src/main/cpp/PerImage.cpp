#include "PerImage.h"
#include "AppState.h"
#include "Constants.h"
#include "Utils.h"

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

void PerImage::GetIndices16StagingBufferSize(AppState& appState, dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size)
{
	TwinKey key { surface.surface, surface.entity };
	auto entry = appState.Scene.indicesPerKey.find(key);
	if (entry == appState.Scene.indicesPerKey.end())
	{
		loaded.indices.size = surface.count * sizeof(uint16_t);
		if (appState.Scene.latestSharedMemoryIndexBuffer16 == nullptr || appState.Scene.usedInLatestSharedMemoryIndexBuffer16 + loaded.indices.size > appState.Scene.latestSharedMemoryIndexBuffer16->size)
		{
			loaded.indices.buffer = new SharedMemoryBuffer { };
			loaded.indices.buffer->CreateIndexBuffer(appState, MEMORY_BLOCK_SIZE);
			appState.Scene.buffers.MoveToFront(loaded.indices.buffer);
			appState.Scene.latestSharedMemoryIndexBuffer16 = loaded.indices.buffer;
			appState.Scene.usedInLatestSharedMemoryIndexBuffer16 = 0;
		}
		else
		{
			loaded.indices.buffer = appState.Scene.latestSharedMemoryIndexBuffer16;
			loaded.indices.offset = appState.Scene.usedInLatestSharedMemoryIndexBuffer16;
		}
		appState.Scene.usedInLatestSharedMemoryIndexBuffer16 += loaded.indices.size;
		size += loaded.indices.size;
		loaded.indices.firstSource = surface.surface;
		loaded.indices.secondSource = surface.entity;
		appState.Scene.buffers.SetupIndices16(loaded.indices);
		SharedMemoryBufferWithOffset newEntry { loaded.indices.buffer, loaded.indices.offset };
		appState.Scene.indicesPerKey.insert({ key, newEntry });
	}
	else
	{
		loaded.indices.size = 0;
		loaded.indices.buffer = entry->second.buffer;
		loaded.indices.offset = entry->second.offset;
	}
}

void PerImage::GetIndices32StagingBufferSize(AppState& appState, dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size)
{
	TwinKey key { surface.surface, surface.entity };
	auto entry = appState.Scene.indicesPerKey.find(key);
	if (entry == appState.Scene.indicesPerKey.end())
	{
		loaded.indices.size = surface.count * sizeof(uint32_t);
		if (appState.Scene.latestSharedMemoryIndexBuffer32 == nullptr || appState.Scene.usedInLatestSharedMemoryIndexBuffer32 + loaded.indices.size > appState.Scene.latestSharedMemoryIndexBuffer32->size)
		{
			loaded.indices.buffer = new SharedMemoryBuffer { };
			loaded.indices.buffer->CreateIndexBuffer(appState, MEMORY_BLOCK_SIZE);
			appState.Scene.buffers.MoveToFront(loaded.indices.buffer);
			appState.Scene.latestSharedMemoryIndexBuffer32 = loaded.indices.buffer;
			appState.Scene.usedInLatestSharedMemoryIndexBuffer32 = 0;
		}
		else
		{
			loaded.indices.buffer = appState.Scene.latestSharedMemoryIndexBuffer32;
			loaded.indices.offset = appState.Scene.usedInLatestSharedMemoryIndexBuffer32;
		}
		appState.Scene.usedInLatestSharedMemoryIndexBuffer32 += loaded.indices.size;
		size += loaded.indices.size;
		loaded.indices.firstSource = surface.surface;
		loaded.indices.secondSource = surface.entity;
		appState.Scene.buffers.SetupIndices32(loaded.indices);
		SharedMemoryBufferWithOffset newEntry { loaded.indices.buffer, loaded.indices.offset };
		appState.Scene.indicesPerKey.insert({ key, newEntry });
	}
	else
	{
		loaded.indices.size = 0;
		loaded.indices.buffer = entry->second.buffer;
		loaded.indices.offset = entry->second.offset;
	}
}

void PerImage::GetAliasIndices16StagingBufferSize(AppState& appState, dalias_t& alias, LoadedAlias& loaded, VkDeviceSize& size)
{
	auto entry = appState.Scene.aliasIndicesPerKey.find(alias.aliashdr);
	if (entry == appState.Scene.aliasIndicesPerKey.end())
	{
		loaded.indices.size = alias.count * sizeof(uint16_t);
		if (appState.Scene.latestSharedMemoryIndexBuffer16 == nullptr || appState.Scene.usedInLatestSharedMemoryIndexBuffer16 + loaded.indices.size > appState.Scene.latestSharedMemoryIndexBuffer16->size)
		{
			loaded.indices.buffer = new SharedMemoryBuffer { };
			loaded.indices.buffer->CreateIndexBuffer(appState, MEMORY_BLOCK_SIZE);
			appState.Scene.buffers.MoveToFront(loaded.indices.buffer);
			appState.Scene.latestSharedMemoryIndexBuffer16 = loaded.indices.buffer;
			appState.Scene.usedInLatestSharedMemoryIndexBuffer16 = 0;
		}
		else
		{
			loaded.indices.buffer = appState.Scene.latestSharedMemoryIndexBuffer16;
			loaded.indices.offset = appState.Scene.usedInLatestSharedMemoryIndexBuffer16;
		}
		appState.Scene.usedInLatestSharedMemoryIndexBuffer16 += loaded.indices.size;
		size += loaded.indices.size;
		loaded.indices.source = alias.aliashdr;
		appState.Scene.buffers.SetupAliasIndices16(loaded.indices);
		SharedMemoryBufferWithOffset newEntry { loaded.indices.buffer, loaded.indices.offset };
		appState.Scene.aliasIndicesPerKey.insert({ alias.aliashdr, newEntry });
	}
	else
	{
		loaded.indices.size = 0;
		loaded.indices.buffer = entry->second.buffer;
		loaded.indices.offset = entry->second.offset;
	}
}

void PerImage::GetAliasIndices32StagingBufferSize(AppState& appState, dalias_t& alias, LoadedAlias& loaded, VkDeviceSize& size)
{
	auto entry = appState.Scene.aliasIndicesPerKey.find(alias.aliashdr);
	if (entry == appState.Scene.aliasIndicesPerKey.end())
	{
		loaded.indices.size = alias.count * sizeof(uint32_t);
		if (appState.Scene.latestSharedMemoryIndexBuffer32 == nullptr || appState.Scene.usedInLatestSharedMemoryIndexBuffer32 + loaded.indices.size > appState.Scene.latestSharedMemoryIndexBuffer32->size)
		{
			loaded.indices.buffer = new SharedMemoryBuffer { };
			loaded.indices.buffer->CreateIndexBuffer(appState, MEMORY_BLOCK_SIZE);
			appState.Scene.buffers.MoveToFront(loaded.indices.buffer);
			appState.Scene.latestSharedMemoryIndexBuffer32 = loaded.indices.buffer;
			appState.Scene.usedInLatestSharedMemoryIndexBuffer32 = 0;
		}
		else
		{
			loaded.indices.buffer = appState.Scene.latestSharedMemoryIndexBuffer32;
			loaded.indices.offset = appState.Scene.usedInLatestSharedMemoryIndexBuffer32;
		}
		appState.Scene.usedInLatestSharedMemoryIndexBuffer32 += loaded.indices.size;
		size += loaded.indices.size;
		loaded.indices.source = alias.aliashdr;
		appState.Scene.buffers.SetupAliasIndices32(loaded.indices);
		SharedMemoryBufferWithOffset newEntry { loaded.indices.buffer, loaded.indices.offset };
		appState.Scene.aliasIndicesPerKey.insert({ alias.aliashdr, newEntry });
	}
	else
	{
		loaded.indices.size = 0;
		loaded.indices.buffer = entry->second.buffer;
		loaded.indices.offset = entry->second.offset;
	}
}

void PerImage::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size)
{
	if (appState.Scene.previousVertexes != surface.vertexes)
	{
		auto entry = appState.Scene.verticesPerKey.find(surface.vertexes);
		if (entry == appState.Scene.verticesPerKey.end())
		{
			loaded.vertices.size = surface.vertex_count * 3 * sizeof(float);
			loaded.vertices.buffer = new SharedMemoryBuffer { };
			loaded.vertices.buffer->CreateVertexBuffer(appState, loaded.vertices.size);
			appState.Scene.buffers.MoveToFront(loaded.vertices.buffer);
			size += loaded.vertices.size;
			loaded.vertices.source = surface.vertexes;
			appState.Scene.buffers.SetupVertices(loaded.vertices);
			appState.Scene.verticesPerKey.insert({ surface.vertexes, loaded.vertices.buffer });
		}
		else
		{
			loaded.vertices.size = 0;
			loaded.vertices.buffer = entry->second;
		}
		appState.Scene.previousVertexes = surface.vertexes;
		appState.Scene.previousVertexBuffer = loaded.vertices.buffer;
	}
	else
	{
		loaded.vertices.size = 0;
		loaded.vertices.buffer = appState.Scene.previousVertexBuffer;
	}
	if (appState.Scene.previousSurface != surface.surface)
	{
		auto entry = appState.Scene.texturePositionsPerKey.find(surface.surface);
		if (entry == appState.Scene.texturePositionsPerKey.end())
		{
			loaded.texturePosition.size = 16 * sizeof(float);
			loaded.texturePosition.buffer = new SharedMemoryBuffer { };
			loaded.texturePosition.buffer->CreateVertexBuffer(appState, loaded.texturePosition.size);
			appState.Scene.buffers.MoveToFront(loaded.texturePosition.buffer);
			size += loaded.texturePosition.size;
			loaded.texturePosition.source = surface.surface;
			appState.Scene.buffers.SetupSurfaceTexturePosition(loaded.texturePosition);
			appState.Scene.texturePositionsPerKey.insert({ surface.surface, loaded.texturePosition.buffer });
		}
		else
		{
			loaded.texturePosition.size = 0;
			loaded.texturePosition.buffer = entry->second;
		}
		appState.Scene.previousSurface = surface.surface;
		appState.Scene.previousTexturePosition = loaded.texturePosition.buffer;
	}
	else
	{
		loaded.texturePosition.size = 0;
		loaded.texturePosition.buffer = appState.Scene.previousTexturePosition;
	}
	auto entry = appState.Scene.lightmaps.lightmaps.find({ surface.surface, surface.entity });
	if (entry == appState.Scene.lightmaps.lightmaps.end())
	{
		auto lightmap = new Lightmap { };
		lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		lightmap->key.first = surface.surface;
		lightmap->key.second = surface.entity;
		loaded.lightmap.lightmap = lightmap;
		loaded.lightmap.size = surface.lightmap_size * sizeof(float);
		size += loaded.lightmap.size;
		loaded.lightmap.source = surface.lightmap;
		appState.Scene.lightmaps.Setup(loaded.lightmap);
		appState.Scene.lightmaps.lightmaps.insert({ lightmap->key, lightmap });
	}
	else if (surface.created)
	{
		auto first = entry->second;
		auto available = (first->unusedCount >= appState.PerImage.size());
		if (first->next == nullptr || available)
		{
			if (available)
			{
				first->unusedCount = 0;
				loaded.lightmap.lightmap = first;
			}
			else
			{
				auto lightmap = new Lightmap { };
				lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				lightmap->key.first = surface.surface;
				lightmap->key.second = surface.entity;
				lightmap->next = first;
				entry->second = lightmap;
				loaded.lightmap.lightmap = lightmap;
			}
		}
		else
		{
			auto found = false;
			for (auto previous = first, lightmap = first->next; lightmap != nullptr; previous = lightmap, lightmap = lightmap->next)
			{
				if (lightmap->unusedCount >= appState.PerImage.size())
				{
					found = true;
					lightmap->unusedCount = 0;
					previous->next = lightmap->next;
					lightmap->next = first;
					entry->second = lightmap;
					loaded.lightmap.lightmap = lightmap;
					break;
				}
			}
			if (!found)
			{
				auto lightmap = new Lightmap { };
				lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				lightmap->key.first = surface.surface;
				lightmap->key.second = surface.entity;
				lightmap->next = entry->second;
				entry->second = lightmap;
				loaded.lightmap.lightmap = lightmap;
			}
		}
		loaded.lightmap.size = surface.lightmap_size * sizeof(float);
		size += loaded.lightmap.size;
		loaded.lightmap.source = surface.lightmap;
		appState.Scene.lightmaps.Setup(loaded.lightmap);
	}
	else
	{
		auto lightmap = entry->second;
		lightmap->unusedCount = 0;
		loaded.lightmap.lightmap = lightmap;
		loaded.lightmap.size = 0;
	}
	if (appState.Scene.previousTexture != surface.texture)
	{
		auto entry = appState.Scene.surfaceTexturesPerKey.find(surface.texture);
		if (entry == appState.Scene.surfaceTexturesPerKey.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(surface.texture_width, surface.texture_height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, surface.texture_width, surface.texture_height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			appState.Scene.textures.MoveToFront(texture);
			loaded.texture.size = surface.texture_size;
			size += loaded.texture.size;
			loaded.texture.texture = texture;
			loaded.texture.source = surface.texture;
			appState.Scene.textures.Setup(loaded.texture);
			appState.Scene.surfaceTexturesPerKey.insert({ surface.texture, texture });
		}
		else
		{
			loaded.texture.size = 0;
			loaded.texture.texture = entry->second;
		}
		appState.Scene.previousTexture = surface.texture;
		appState.Scene.previousSharedMemoryTexture = loaded.texture.texture;
	}
	else
	{
		loaded.texture.size = 0;
		loaded.texture.texture = appState.Scene.previousSharedMemoryTexture;
	}
	loaded.count = surface.count;
	loaded.originX = surface.origin_x;
	loaded.originY = surface.origin_y;
	loaded.originZ = surface.origin_z;
	loaded.yaw = surface.yaw;
	loaded.pitch = surface.pitch;
	loaded.roll = surface.roll;
}

void PerImage::GetStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size)
{
	if (appState.Scene.previousVertexes != turbulent.vertexes)
	{
		auto entry = appState.Scene.verticesPerKey.find(turbulent.vertexes);
		if (entry == appState.Scene.verticesPerKey.end())
		{
			loaded.vertices.size = turbulent.vertex_count * 3 * sizeof(float);
			loaded.vertices.buffer = new SharedMemoryBuffer { };
			loaded.vertices.buffer->CreateVertexBuffer(appState, loaded.vertices.size);
			appState.Scene.buffers.MoveToFront(loaded.vertices.buffer);
			size += loaded.vertices.size;
			loaded.vertices.source = turbulent.vertexes;
			appState.Scene.buffers.SetupVertices(loaded.vertices);
			appState.Scene.verticesPerKey.insert({ turbulent.vertexes, loaded.vertices.buffer });
		}
		else
		{
			loaded.vertices.size = 0;
			loaded.vertices.buffer = entry->second;
		}
		appState.Scene.previousVertexes = turbulent.vertexes;
		appState.Scene.previousVertexBuffer = loaded.vertices.buffer;
	}
	else
	{
		loaded.vertices.size = 0;
		loaded.vertices.buffer = appState.Scene.previousVertexBuffer;
	}
	if (appState.Scene.previousSurface != turbulent.surface)
	{
		auto entry = appState.Scene.texturePositionsPerKey.find(turbulent.surface);
		if (entry == appState.Scene.texturePositionsPerKey.end())
		{
			loaded.texturePosition.size = 16 * sizeof(float);
			loaded.texturePosition.buffer = new SharedMemoryBuffer { };
			loaded.texturePosition.buffer->CreateVertexBuffer(appState, loaded.texturePosition.size);
			appState.Scene.buffers.MoveToFront(loaded.texturePosition.buffer);
			size += loaded.texturePosition.size;
			loaded.texturePosition.source = turbulent.surface;
			appState.Scene.buffers.SetupTurbulentTexturePosition(loaded.texturePosition);
			appState.Scene.texturePositionsPerKey.insert({ turbulent.surface, loaded.texturePosition.buffer });
		}
		else
		{
			loaded.texturePosition.size = 0;
			loaded.texturePosition.buffer = entry->second;
		}
		appState.Scene.previousSurface = turbulent.surface;
		appState.Scene.previousTexturePosition = loaded.texturePosition.buffer;
	}
	else
	{
		loaded.texturePosition.size = 0;
		loaded.texturePosition.buffer = appState.Scene.previousTexturePosition;
	}
	if (appState.Scene.previousTexture != turbulent.texture)
	{
		auto entry = appState.Scene.turbulentPerKey.find(turbulent.texture);
		if (entry == appState.Scene.turbulentPerKey.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, turbulent.width, turbulent.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			appState.Scene.textures.MoveToFront(texture);
			loaded.texture.size = turbulent.size;
			size += loaded.texture.size;
			loaded.texture.texture = texture;
			loaded.texture.source = turbulent.data;
			appState.Scene.textures.Setup(loaded.texture);
			appState.Scene.turbulentPerKey.insert({ turbulent.texture, texture });
		}
		else
		{
			loaded.texture.size = 0;
			loaded.texture.texture = entry->second;
		}
		appState.Scene.previousTexture = turbulent.texture;
		appState.Scene.previousSharedMemoryTexture = loaded.texture.texture;
	}
	else
	{
		loaded.texture.size = 0;
		loaded.texture.texture = appState.Scene.previousSharedMemoryTexture;
	}
	loaded.count = turbulent.count;
	loaded.originX = turbulent.origin_x;
	loaded.originY = turbulent.origin_y;
	loaded.originZ = turbulent.origin_z;
	loaded.yaw = turbulent.yaw;
	loaded.pitch = turbulent.pitch;
	loaded.roll = turbulent.roll;
}

void PerImage::GetStagingBufferSize(AppState& appState, const dalias_t& alias, LoadedAlias& loaded, VkDeviceSize& size)
{
	if (appState.Scene.previousApverts != alias.apverts)
	{
		auto entry = appState.Scene.aliasVerticesPerKey.find(alias.apverts);
		if (entry == appState.Scene.aliasVerticesPerKey.end())
		{
			loaded.vertices.size = alias.vertex_count * 2 * 4 * sizeof(float);
			loaded.vertices.buffer = new SharedMemoryBuffer { };
			loaded.vertices.buffer->CreateVertexBuffer(appState, loaded.vertices.size);
			appState.Scene.buffers.MoveToFront(loaded.vertices.buffer);
			size += loaded.vertices.size;
			loaded.vertices.source = alias.apverts;
			appState.Scene.buffers.SetupAliasVertices(loaded.vertices);
			loaded.texCoords.size = alias.vertex_count * 2 * 2 * sizeof(float);
			loaded.texCoords.buffer = new SharedMemoryBuffer { };
			loaded.texCoords.buffer->CreateVertexBuffer(appState, loaded.texCoords.size);
			appState.Scene.buffers.MoveToFront(loaded.texCoords.buffer);
			size += loaded.texCoords.size;
			loaded.texCoords.source = alias.texture_coordinates;
			loaded.texCoords.width = alias.width;
			loaded.texCoords.height = alias.height;
			appState.Scene.buffers.SetupAliasTexCoords(loaded.texCoords);
			appState.Scene.aliasVerticesPerKey.insert({ alias.apverts, { loaded.vertices.buffer, loaded.texCoords.buffer } });
		}
		else
		{
			loaded.vertices.size = 0;
			loaded.vertices.buffer = entry->second.vertices;
			loaded.texCoords.size = 0;
			loaded.texCoords.buffer = entry->second.texCoords;
		}
		appState.Scene.previousApverts = alias.apverts;
		appState.Scene.previousVertexBuffer = loaded.vertices.buffer;
		appState.Scene.previousTexCoordsBuffer = loaded.texCoords.buffer;
	}
	else
	{
		loaded.vertices.size = 0;
		loaded.vertices.buffer = appState.Scene.previousVertexBuffer;
		loaded.texCoords.size = 0;
		loaded.texCoords.buffer = appState.Scene.previousTexCoordsBuffer;
	}
	if (alias.is_host_colormap)
	{
		loaded.colormapped.colormap.size = 0;
		loaded.colormapped.colormap.texture = host_colormap;
	}
	else
	{
		loaded.colormapped.colormap.size = 16384;
		size += loaded.colormapped.colormap.size;
	}
	if (appState.Scene.previousTexture != alias.data)
	{
		auto entry = appState.Scene.aliasTexturesPerKey.find(alias.data);
		if (entry == appState.Scene.aliasTexturesPerKey.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(alias.width, alias.height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, alias.width, alias.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			appState.Scene.textures.MoveToFront(texture);
			loaded.colormapped.texture.size = alias.size;
			size += loaded.colormapped.texture.size;
			loaded.colormapped.texture.texture = texture;
			loaded.colormapped.texture.source = alias.data;
			appState.Scene.textures.Setup(loaded.colormapped.texture);
			appState.Scene.aliasTexturesPerKey.insert({ alias.data, texture });
		}
		else
		{
			loaded.colormapped.texture.size = 0;
			loaded.colormapped.texture.texture = entry->second;
		}
		appState.Scene.previousTexture = alias.data;
		appState.Scene.previousSharedMemoryTexture = loaded.colormapped.texture.texture;
	}
	else
	{
		loaded.colormapped.texture.size = 0;
		loaded.colormapped.texture.texture = appState.Scene.previousSharedMemoryTexture;
	}
	loaded.firstAttribute = alias.first_attribute;
	loaded.isHostColormap = alias.is_host_colormap;
	loaded.count = alias.count;
	for (auto j = 0; j < 3; j++)
	{
		for (auto i = 0; i < 4; i++)
		{
			loaded.transform[j][i] = alias.transform[j][i];
		}
	}
}

VkDeviceSize PerImage::GetStagingBufferSize(AppState& appState)
{
	appState.Scene.lastSurface16 = d_lists.last_surface16;
	appState.Scene.lastSurface32 = d_lists.last_surface32;
	appState.Scene.lastFence16 = d_lists.last_fence16;
	appState.Scene.lastFence32 = d_lists.last_fence32;
	appState.Scene.lastSprite = d_lists.last_sprite;
	appState.Scene.lastTurbulent16 = d_lists.last_turbulent16;
	appState.Scene.lastTurbulent32 = d_lists.last_turbulent32;
	appState.Scene.lastAlias16 = d_lists.last_alias16;
	appState.Scene.lastAlias32 = d_lists.last_alias32;
	appState.Scene.lastViewmodel16 = d_lists.last_viewmodel16;
	appState.Scene.lastViewmodel32 = d_lists.last_viewmodel32;
	appState.Scene.lastColoredIndex16 = d_lists.last_colored_index16;
	appState.Scene.lastColoredIndex32 = d_lists.last_colored_index32;
	appState.Scene.lastSky = d_lists.last_sky;
	if (appState.Scene.lastSurface16 >= appState.Scene.loadedSurfaces16.size())
	{
		appState.Scene.loadedSurfaces16.resize(appState.Scene.lastSurface16 + 1);
	}
	if (appState.Scene.lastSurface32 >= appState.Scene.loadedSurfaces32.size())
	{
		appState.Scene.loadedSurfaces32.resize(appState.Scene.lastSurface32 + 1);
	}
	if (appState.Scene.lastFence16 >= appState.Scene.loadedFences16.size())
	{
		appState.Scene.loadedFences16.resize(appState.Scene.lastFence16 + 1);
	}
	if (appState.Scene.lastFence32 >= appState.Scene.loadedFences32.size())
	{
		appState.Scene.loadedFences32.resize(appState.Scene.lastFence32 + 1);
	}
	if (appState.Scene.lastSprite >= appState.Scene.loadedSprites.size())
	{
		appState.Scene.loadedSprites.resize(appState.Scene.lastSprite + 1);
	}
	if (appState.Scene.lastTurbulent16 >= appState.Scene.loadedTurbulent16.size())
	{
		appState.Scene.loadedTurbulent16.resize(appState.Scene.lastTurbulent16 + 1);
	}
	if (appState.Scene.lastTurbulent32 >= appState.Scene.loadedTurbulent32.size())
	{
		appState.Scene.loadedTurbulent32.resize(appState.Scene.lastTurbulent32 + 1);
	}
	if (appState.Scene.lastAlias16 >= appState.Scene.loadedAlias16.size())
	{
		appState.Scene.loadedAlias16.resize(appState.Scene.lastAlias16 + 1);
	}
	if (appState.Scene.lastAlias32 >= appState.Scene.loadedAlias32.size())
	{
		appState.Scene.loadedAlias32.resize(appState.Scene.lastAlias32 + 1);
	}
	if (appState.Scene.lastViewmodel16 >= appState.Scene.loadedViewmodels16.size())
	{
		appState.Scene.loadedViewmodels16.resize(appState.Scene.lastViewmodel16 + 1);
	}
	if (appState.Scene.lastViewmodel32 >= appState.Scene.loadedViewmodels32.size())
	{
		appState.Scene.loadedViewmodels32.resize(appState.Scene.lastViewmodel32 + 1);
	}
	auto size = appState.Scene.matrices.size;
	appState.Scene.paletteSize = 0;
	if (palette == nullptr || paletteChanged != pal_changed)
	{
		if (palette == nullptr)
		{
			palette = new Texture();
			palette->Create(appState, 256, 1, COLOR_FORMAT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
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
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousSurface = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastSurface16; i++)
	{
		GetStagingBufferSize(appState, d_lists.surfaces16[i], appState.Scene.loadedSurfaces16[i], size);
		GetIndices16StagingBufferSize(appState, d_lists.surfaces16[i], appState.Scene.loadedSurfaces16[i], size);
	}
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousSurface = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastSurface32; i++)
	{
		GetStagingBufferSize(appState, d_lists.surfaces32[i], appState.Scene.loadedSurfaces32[i], size);
		GetIndices32StagingBufferSize(appState, d_lists.surfaces32[i], appState.Scene.loadedSurfaces32[i], size);
	}
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousSurface = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastFence16; i++)
	{
		GetStagingBufferSize(appState, d_lists.fences16[i], appState.Scene.loadedFences16[i], size);
		GetIndices16StagingBufferSize(appState, d_lists.fences16[i], appState.Scene.loadedFences16[i], size);
	}
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousSurface = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastFence32; i++)
	{
		GetStagingBufferSize(appState, d_lists.fences32[i], appState.Scene.loadedFences32[i], size);
		GetIndices32StagingBufferSize(appState, d_lists.fences32[i], appState.Scene.loadedFences32[i], size);
	}
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastSprite; i++)
	{
		auto& sprite = d_lists.sprites[i];
		auto& loaded = appState.Scene.loadedSprites[i];
		if (appState.Scene.previousTexture != sprite.data)
		{
			auto entry = appState.Scene.spritesPerKey.find(sprite.data);
			if (entry == appState.Scene.spritesPerKey.end())
			{
				auto mipCount = (int)(std::floor(std::log2(std::max(sprite.width, sprite.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, sprite.width, sprite.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				appState.Scene.textures.MoveToFront(texture);
				loaded.texture.size = sprite.size;
				size += loaded.texture.size;
				loaded.texture.texture = texture;
				loaded.texture.source = sprite.data;
				appState.Scene.textures.Setup(loaded.texture);
				appState.Scene.spritesPerKey.insert({ sprite.data, texture });
			}
			else
			{
				loaded.texture.size = 0;
				loaded.texture.texture = entry->second;
			}
			appState.Scene.previousTexture = sprite.data;
			appState.Scene.previousSharedMemoryTexture = loaded.texture.texture;
		}
		else
		{
			loaded.texture.size = 0;
			loaded.texture.texture = appState.Scene.previousSharedMemoryTexture;
		}
		loaded.count = sprite.count;
		loaded.firstVertex = sprite.first_vertex;
	}
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousSurface = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastTurbulent16; i++)
	{
		auto& turbulent = d_lists.turbulent16[i];
		auto& loaded = appState.Scene.loadedTurbulent16[i];
		GetStagingBufferSize(appState, turbulent, loaded, size);
		TwinKey key { turbulent.surface, turbulent.entity };
		auto entry = appState.Scene.indicesPerKey.find(key);
		if (entry == appState.Scene.indicesPerKey.end())
		{
			loaded.indices.size = turbulent.count * sizeof(uint16_t);
			if (appState.Scene.latestSharedMemoryIndexBuffer16 == nullptr || appState.Scene.usedInLatestSharedMemoryIndexBuffer16 + loaded.indices.size > appState.Scene.latestSharedMemoryIndexBuffer16->size)
			{
				loaded.indices.buffer = new SharedMemoryBuffer { };
				loaded.indices.buffer->CreateIndexBuffer(appState, MEMORY_BLOCK_SIZE);
				appState.Scene.buffers.MoveToFront(loaded.indices.buffer);
				appState.Scene.latestSharedMemoryIndexBuffer16 = loaded.indices.buffer;
				appState.Scene.usedInLatestSharedMemoryIndexBuffer16 = 0;
			}
			else
			{
				loaded.indices.buffer = appState.Scene.latestSharedMemoryIndexBuffer16;
				loaded.indices.offset = appState.Scene.usedInLatestSharedMemoryIndexBuffer16;
			}
			appState.Scene.usedInLatestSharedMemoryIndexBuffer16 += loaded.indices.size;
			size += loaded.indices.size;
			loaded.indices.firstSource = turbulent.surface;
			loaded.indices.secondSource = turbulent.entity;
			appState.Scene.buffers.SetupIndices16(loaded.indices);
			SharedMemoryBufferWithOffset newEntry { loaded.indices.buffer, loaded.indices.offset };
			appState.Scene.indicesPerKey.insert({ key, newEntry });
		}
		else
		{
			loaded.indices.size = 0;
			loaded.indices.buffer = entry->second.buffer;
			loaded.indices.offset = entry->second.offset;
		}
	}
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousSurface = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastTurbulent32; i++)
	{
		auto& turbulent = d_lists.turbulent32[i];
		auto& loaded = appState.Scene.loadedTurbulent32[i];
		GetStagingBufferSize(appState, turbulent, loaded, size);
		TwinKey key { turbulent.surface, turbulent.entity };
		auto entry = appState.Scene.indicesPerKey.find(key);
		if (entry == appState.Scene.indicesPerKey.end())
		{
			loaded.indices.size = turbulent.count * sizeof(uint32_t);
			if (appState.Scene.latestSharedMemoryIndexBuffer32 == nullptr || appState.Scene.usedInLatestSharedMemoryIndexBuffer32 + loaded.indices.size > appState.Scene.latestSharedMemoryIndexBuffer32->size)
			{
				loaded.indices.buffer = new SharedMemoryBuffer { };
				loaded.indices.buffer->CreateIndexBuffer(appState, MEMORY_BLOCK_SIZE);
				appState.Scene.buffers.MoveToFront(loaded.indices.buffer);
				appState.Scene.latestSharedMemoryIndexBuffer32 = loaded.indices.buffer;
				appState.Scene.usedInLatestSharedMemoryIndexBuffer32 = 0;
			}
			else
			{
				loaded.indices.buffer = appState.Scene.latestSharedMemoryIndexBuffer32;
				loaded.indices.offset = appState.Scene.usedInLatestSharedMemoryIndexBuffer32;
			}
			appState.Scene.usedInLatestSharedMemoryIndexBuffer32 += loaded.indices.size;
			size += loaded.indices.size;
			loaded.indices.firstSource = turbulent.surface;
			loaded.indices.secondSource = turbulent.entity;
			appState.Scene.buffers.SetupIndices32(loaded.indices);
			SharedMemoryBufferWithOffset newEntry { loaded.indices.buffer, loaded.indices.offset };
			appState.Scene.indicesPerKey.insert({ key, newEntry });
		}
		else
		{
			loaded.indices.size = 0;
			loaded.indices.buffer = entry->second.buffer;
			loaded.indices.offset = entry->second.offset;
		}
	}
	appState.Scene.previousApverts = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastAlias16; i++)
	{
		GetStagingBufferSize(appState, d_lists.alias16[i], appState.Scene.loadedAlias16[i], size);
		GetAliasIndices16StagingBufferSize(appState, d_lists.alias16[i], appState.Scene.loadedAlias16[i], size);
	}
	appState.Scene.previousApverts = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastAlias32; i++)
	{
		GetStagingBufferSize(appState, d_lists.alias32[i], appState.Scene.loadedAlias32[i], size);
		GetAliasIndices32StagingBufferSize(appState, d_lists.alias32[i], appState.Scene.loadedAlias32[i], size);
	}
	appState.Scene.previousApverts = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastViewmodel16; i++)
	{
		GetStagingBufferSize(appState, d_lists.viewmodels16[i], appState.Scene.loadedViewmodels16[i], size);
		GetAliasIndices16StagingBufferSize(appState, d_lists.viewmodels16[i], appState.Scene.loadedViewmodels16[i], size);
	}
	appState.Scene.previousApverts = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastViewmodel32; i++)
	{
		GetStagingBufferSize(appState, d_lists.viewmodels32[i], appState.Scene.loadedViewmodels32[i], size);
		GetAliasIndices16StagingBufferSize(appState, d_lists.viewmodels32[i], appState.Scene.loadedViewmodels32[i], size);
	}
	if (appState.Scene.lastSky >= 0)
	{
		if (sky == nullptr)
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(128, 128)))) + 1;
			sky = new Texture();
			sky->Create(appState, 128, 128, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		appState.Scene.skySize = 16384;
		size += appState.Scene.skySize;
		appState.Scene.skyCount = d_lists.sky[0].count;
		appState.Scene.firstSkyVertex = d_lists.sky[0].first_vertex;
	}
	while (size % 4 != 0)
	{
		size++;
	}
	return size;
}

void PerImage::LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer)
{
	VkDeviceSize offset = 0;
	size_t matricesSize = 2 * sizeof(XrMatrix4x4f);
	memcpy(stagingBuffer->mapped, appState.Scene.viewMatrices, matricesSize);
	offset += matricesSize;
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, appState.Scene.projectionMatrices, matricesSize);
	offset += matricesSize;
	auto loadedBuffer = appState.Scene.buffers.firstVertices;
	while (loadedBuffer != nullptr)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, loadedBuffer->source, loadedBuffer->size);
		offset += loadedBuffer->size;
		loadedBuffer = loadedBuffer->next;
	}
	auto loadedIndexBuffer = appState.Scene.buffers.firstIndices16;
	while (loadedIndexBuffer != nullptr)
	{
		auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto face = (msurface_t*)loadedIndexBuffer->firstSource;
		auto entity = (entity_t*)loadedIndexBuffer->secondSource;
		auto edge = entity->model->surfedges[face->firstedge];
		uint16_t index;
		if (edge >= 0)
		{
			index = entity->model->edges[edge].v[0];
		}
		else
		{
			index = entity->model->edges[-edge].v[1];
		}
		*target++ = index;
		auto next_front = 0;
		auto next_back = face->numedges;
		auto use_back = false;
		for (auto i = 1; i < face->numedges; i++)
		{
			if (use_back)
			{
				next_back--;
				edge = entity->model->surfedges[face->firstedge + next_back];
			}
			else
			{
				next_front++;
				edge = entity->model->surfedges[face->firstedge + next_front];
			}
			use_back = !use_back;
			if (edge >= 0)
			{
				index = entity->model->edges[edge].v[0];
			}
			else
			{
				index = entity->model->edges[-edge].v[1];
			}
			*target++ = index;
		}
		offset += loadedIndexBuffer->size;
		loadedIndexBuffer = loadedIndexBuffer->next;
	}
	auto loadedAliasIndexBuffer = appState.Scene.buffers.firstAliasIndices16;
	while (loadedAliasIndexBuffer != nullptr)
	{
		auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto aliashdr = (aliashdr_t *)loadedAliasIndexBuffer->source;
		auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
		auto triangle = (mtriangle_t *)((byte *)aliashdr + aliashdr->triangles);
		auto stverts = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);
		for (auto i = 0; i < mdl->numtris; i++)
		{
			auto v0 = triangle->vertindex[0];
			auto v1 = triangle->vertindex[1];
			auto v2 = triangle->vertindex[2];
			auto v0back = (((stverts[v0].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v1back = (((stverts[v1].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v2back = (((stverts[v2].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			*target++ = v0 * 2 + (v0back ? 1 : 0);
			*target++ = v1 * 2 + (v1back ? 1 : 0);
			*target++ = v2 * 2 + (v2back ? 1 : 0);
			triangle++;
		}
		offset += loadedAliasIndexBuffer->size;
		loadedAliasIndexBuffer = loadedAliasIndexBuffer->next;
	}
	while (offset % 4 != 0)
	{
		offset++;
	}
	loadedIndexBuffer = appState.Scene.buffers.firstIndices32;
	while (loadedIndexBuffer != nullptr)
	{
		auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto face = (msurface_t*)loadedIndexBuffer->firstSource;
		auto entity = (entity_t*)loadedIndexBuffer->secondSource;
		auto edge = entity->model->surfedges[face->firstedge];
		uint32_t index;
		if (edge >= 0)
		{
			index = entity->model->edges[edge].v[0];
		}
		else
		{
			index = entity->model->edges[-edge].v[1];
		}
		*target++ = index;
		auto next_front = 0;
		auto next_back = face->numedges;
		auto use_back = false;
		for (auto i = 1; i < face->numedges; i++)
		{
			if (use_back)
			{
				next_back--;
				edge = entity->model->surfedges[face->firstedge + next_back];
			}
			else
			{
				next_front++;
				edge = entity->model->surfedges[face->firstedge + next_front];
			}
			use_back = !use_back;
			if (edge >= 0)
			{
				index = entity->model->edges[edge].v[0];
			}
			else
			{
				index = entity->model->edges[-edge].v[1];
			}
			*target++ = index;
		}
		offset += loadedIndexBuffer->size;
		loadedIndexBuffer = loadedIndexBuffer->next;
	}
	loadedAliasIndexBuffer = appState.Scene.buffers.firstAliasIndices32;
	while (loadedAliasIndexBuffer != nullptr)
	{
		auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto aliashdr = (aliashdr_t *)loadedAliasIndexBuffer->source;
		auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
		auto triangle = (mtriangle_t *)((byte *)aliashdr + aliashdr->triangles);
		auto stverts = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);
		for (auto i = 0; i < mdl->numtris; i++)
		{
			auto v0 = triangle->vertindex[0];
			auto v1 = triangle->vertindex[1];
			auto v2 = triangle->vertindex[2];
			auto v0back = (((stverts[v0].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v1back = (((stverts[v1].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v2back = (((stverts[v2].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			*target++ = v0 * 2 + (v0back ? 1 : 0);
			*target++ = v1 * 2 + (v1back ? 1 : 0);
			*target++ = v2 * 2 + (v2back ? 1 : 0);
			triangle++;
		}
		offset += loadedAliasIndexBuffer->size;
		loadedAliasIndexBuffer = loadedAliasIndexBuffer->next;
	}
	loadedBuffer = appState.Scene.buffers.firstSurfaceTexturePosition;
	while (loadedBuffer != nullptr)
	{
		auto source = (msurface_t*)loadedBuffer->source;
		auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
		*target++ = source->texinfo->vecs[0][0];
		*target++ = source->texinfo->vecs[0][1];
		*target++ = source->texinfo->vecs[0][2];
		*target++ = source->texinfo->vecs[0][3];
		*target++ = source->texinfo->vecs[1][0];
		*target++ = source->texinfo->vecs[1][1];
		*target++ = source->texinfo->vecs[1][2];
		*target++ = source->texinfo->vecs[1][3];
		*target++ = source->texturemins[0];
		*target++ = source->texturemins[1];
		*target++ = source->extents[0];
		*target++ = source->extents[1];
		*target++ = source->texinfo->texture->width;
		*target++ = source->texinfo->texture->height;
		offset += loadedBuffer->size;
		loadedBuffer = loadedBuffer->next;
	}
	loadedBuffer = appState.Scene.buffers.firstTurbulentTexturePosition;
	while (loadedBuffer != nullptr)
	{
		auto source = (msurface_t*)loadedBuffer->source;
		auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
		*target++ = source->texinfo->vecs[0][0];
		*target++ = source->texinfo->vecs[0][1];
		*target++ = source->texinfo->vecs[0][2];
		*target++ = source->texinfo->vecs[0][3];
		*target++ = source->texinfo->vecs[1][0];
		*target++ = source->texinfo->vecs[1][1];
		*target++ = source->texinfo->vecs[1][2];
		*target++ = source->texinfo->vecs[1][3];
		*target++ = source->texinfo->texture->width;
		*target++ = source->texinfo->texture->height;
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
	auto loadedLightmap = appState.Scene.lightmaps.first;
	while (loadedLightmap != nullptr)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, loadedLightmap->source, loadedLightmap->size);
		offset += loadedLightmap->size;
		loadedLightmap = loadedLightmap->next;
	}
	auto loadedTexture = appState.Scene.textures.first;
	while (loadedTexture != nullptr)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, loadedTexture->source, loadedTexture->size);
		offset += loadedTexture->size;
		loadedTexture = loadedTexture->next;
	}
	for (auto i = 0; i <= appState.Scene.lastAlias16; i++)
	{
		auto& alias = d_lists.alias16[i];
		if (!alias.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.colormap.data(), 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= appState.Scene.lastAlias32; i++)
	{
		auto& alias = d_lists.alias32[i];
		if (!alias.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.colormap.data(), 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= appState.Scene.lastViewmodel16; i++)
	{
		auto& viewmodel = d_lists.viewmodels16[i];
		if (!viewmodel.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.colormap.data(), 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= appState.Scene.lastViewmodel32; i++)
	{
		auto& viewmodel = d_lists.viewmodels32[i];
		if (!viewmodel.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.colormap.data(), 16384);
			offset += 16384;
		}
	}
	if (appState.Scene.lastSky >= 0)
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

void PerImage::FillColormapTextures(AppState& appState, LoadedColormappedTexture& loadedTexture, dalias_t& alias)
{
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

void PerImage::FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer)
{
	auto loaded = first;
	while (loaded != nullptr)
	{
		bufferCopy.dstOffset = loaded->offset;
		bufferCopy.size = loaded->size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, loaded->buffer->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += loaded->size;
		if (previousBuffer != loaded->buffer)
		{
			appState.Scene.AddToBufferBarrier(loaded->buffer->buffer);
			previousBuffer = loaded->buffer;
		}
		loaded = loaded->next;
	}
}

void PerImage::FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryAliasIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer)
{
	auto loaded = first;
	while (loaded != nullptr)
	{
		bufferCopy.dstOffset = loaded->offset;
		bufferCopy.size = loaded->size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, loaded->buffer->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += loaded->size;
		if (previousBuffer != loaded->buffer)
		{
			appState.Scene.AddToBufferBarrier(loaded->buffer->buffer);
			previousBuffer = loaded->buffer;
		}
		loaded = loaded->next;
	}
}

void PerImage::FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryBuffer* first, VkBufferCopy& bufferCopy)
{
	auto loaded = first;
	while (loaded != nullptr)
	{
		bufferCopy.size = loaded->size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, loaded->buffer->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += loaded->size;
		appState.Scene.AddToBufferBarrier(loaded->buffer->buffer);
		loaded = loaded->next;
	}
}

void PerImage::FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer)
{
	appState.Scene.stagingBuffer.lastBarrier = -1;
	VkBufferCopy bufferCopy { };
	bufferCopy.size = appState.Scene.matrices.size;
	vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, appState.Scene.matrices.buffer, 1, &bufferCopy);
	VkBufferMemoryBarrier barrier { };
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.buffer = appState.Scene.matrices.buffer;
	barrier.size = VK_WHOLE_SIZE;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	bufferCopy.srcOffset += appState.Scene.matrices.size;
	auto loadedBuffer = appState.Scene.buffers.firstVertices;
	while (loadedBuffer != nullptr)
	{
		bufferCopy.size = loadedBuffer->size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, loadedBuffer->buffer->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += loadedBuffer->size;
		appState.Scene.AddToBufferBarrier(loadedBuffer->buffer->buffer);
		loadedBuffer = loadedBuffer->next;
	}
	SharedMemoryBuffer* previousBuffer = nullptr;
	FillFromStagingBuffer(appState, stagingBuffer, appState.Scene.buffers.firstIndices16, bufferCopy, previousBuffer);
	FillFromStagingBuffer(appState, stagingBuffer, appState.Scene.buffers.firstAliasIndices16, bufferCopy, previousBuffer);
	while (bufferCopy.srcOffset % 4 != 0)
	{
		bufferCopy.srcOffset++;
	}
	FillFromStagingBuffer(appState, stagingBuffer, appState.Scene.buffers.firstIndices32, bufferCopy, previousBuffer);
	FillFromStagingBuffer(appState, stagingBuffer, appState.Scene.buffers.firstAliasIndices32, bufferCopy, previousBuffer);
	bufferCopy.dstOffset = 0;
	FillFromStagingBuffer(appState, stagingBuffer, appState.Scene.buffers.firstSurfaceTexturePosition, bufferCopy);
	FillFromStagingBuffer(appState, stagingBuffer, appState.Scene.buffers.firstTurbulentTexturePosition, bufferCopy);
	FillFromStagingBuffer(appState, stagingBuffer, appState.Scene.buffers.firstAliasVertices, bufferCopy);
	auto loadedTexCoordsBuffer = appState.Scene.buffers.firstAliasTexCoords;
	while (loadedTexCoordsBuffer != nullptr)
	{
		bufferCopy.size = loadedTexCoordsBuffer->size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, loadedTexCoordsBuffer->buffer->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += loadedTexCoordsBuffer->size;
		appState.Scene.AddToBufferBarrier(loadedTexCoordsBuffer->buffer->buffer);
		loadedTexCoordsBuffer = loadedTexCoordsBuffer->next;
	}
	if (appState.Scene.stagingBuffer.lastBarrier >= 0)
	{
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, appState.Scene.stagingBuffer.lastBarrier + 1, appState.Scene.stagingBuffer.bufferBarriers.data(), 0, nullptr);
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
	auto loadedLightmap = appState.Scene.lightmaps.first;
	while (loadedLightmap != nullptr)
	{
		loadedLightmap->lightmap->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += loadedLightmap->size;
		loadedLightmap = loadedLightmap->next;
	}
	auto loadedTexture = appState.Scene.textures.first;
	while (loadedTexture != nullptr)
	{
		loadedTexture->texture->FillMipmapped(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += loadedTexture->size;
		loadedTexture = loadedTexture->next;
	}
	for (auto i = 0; i <= appState.Scene.lastAlias16; i++)
	{
		auto& alias = d_lists.alias16[i];
		FillColormapTextures(appState, appState.Scene.loadedAlias16[i].colormapped, alias);
	}
	for (auto i = 0; i <= appState.Scene.lastAlias32; i++)
	{
		auto& alias = d_lists.alias32[i];
		FillColormapTextures(appState, appState.Scene.loadedAlias32[i].colormapped, alias);
	}
	for (auto i = 0; i <= appState.Scene.lastViewmodel16; i++)
	{
		auto& viewmodel = d_lists.viewmodels16[i];
		FillColormapTextures(appState, appState.Scene.loadedViewmodels16[i].colormapped, viewmodel);
	}
	for (auto i = 0; i <= appState.Scene.lastViewmodel32; i++)
	{
		auto& viewmodel = d_lists.viewmodels32[i];
		FillColormapTextures(appState, appState.Scene.loadedViewmodels32[i].colormapped, viewmodel);
	}
	if (appState.Scene.lastSky >= 0)
	{
		sky->FillMipmapped(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += appState.Scene.skySize;
	}
	if (appState.Scene.stagingBuffer.lastBarrier >= 0)
	{
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, appState.Scene.stagingBuffer.lastBarrier + 1, appState.Scene.stagingBuffer.imageBarriers.data());
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
	/*if (key_dest == key_console || key_dest == key_menu || appState.Mode != AppWorldMode)
	{
		if (appState.LeftController.TrackingResult == ovrSuccess)
		{
			appState.Scene.controllerVerticesSize += 2 * 8 * 3 * sizeof(float);
		}
		if (appState.RightController.TrackingResult == ovrSuccess)
		{
			appState.Scene.controllerVerticesSize += 2 * 8 * 3 * sizeof(float);
		}
	}*/
	appState.Scene.verticesSize = floorVerticesSize + texturedVerticesSize + coloredVerticesSize + appState.Scene.controllerVerticesSize;
	if (appState.Scene.verticesSize > 0)
	{
		vertices = cachedVertices.GetVertexBuffer(appState, appState.Scene.verticesSize);
		if (floorVerticesSize > 0)
		{
			auto mapped = (float*)vertices->mapped;
			*mapped++ = -0.5;
			*mapped++ = appState.DistanceToFloor;
			*mapped++ = -0.5;
			*mapped++ = 0.5;
			*mapped++ = appState.DistanceToFloor;
			*mapped++ = -0.5;
			*mapped++ = 0.5;
			*mapped++ = appState.DistanceToFloor;
			*mapped++ = 0.5;
			*mapped++ = -0.5;
			*mapped++ = appState.DistanceToFloor;
			*mapped++ = 0.5;
		}
		texturedVertexBase = floorVerticesSize;
		memcpy((unsigned char*)vertices->mapped + texturedVertexBase, d_lists.textured_vertices.data(), texturedVerticesSize);
		coloredVertexBase = texturedVertexBase + texturedVerticesSize;
		memcpy((unsigned char*)vertices->mapped + coloredVertexBase, d_lists.colored_vertices.data(), coloredVerticesSize);
		controllerVertexBase = coloredVertexBase + coloredVerticesSize;
		if (appState.Scene.controllerVerticesSize > 0)
		{
			/*auto mapped = (float*)vertices->mapped + controllerVertexBase / sizeof(float);
			if (appState.LeftController.TrackingResult == ovrSuccess)
			{
				mapped = appState.LeftController.WriteVertices(mapped);
			}
			if (appState.RightController.TrackingResult == ovrSuccess)
			{
				mapped = appState.RightController.WriteVertices(mapped);
			}*/
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
		/*if (appState.LeftController.TrackingResult == ovrSuccess)
		{
			controllerAttributesSize += 2 * 8 * 2 * sizeof(float);
		}
		if (appState.RightController.TrackingResult == ovrSuccess)
		{
			controllerAttributesSize += 2 * 8 * 2 * sizeof(float);
		}*/
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
	*mapped++ = appState.OriginX;
	*mapped++ = appState.OriginY;
	*mapped++ = appState.OriginZ;
	*mapped++ = 1;
	controllerAttributeBase = vertexTransformBase + 16 * sizeof(float);
	if (controllerAttributesSize > 0)
	{
		/*if (appState.LeftController.TrackingResult == ovrSuccess)
		{
			mapped = appState.LeftController.WriteAttributes(mapped);
		}
		if (appState.RightController.TrackingResult == ovrSuccess)
		{
			mapped = appState.RightController.WriteAttributes(mapped);
		}*/
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
	VkDeviceSize coloredIndices16Size = (d_lists.last_colored_index16 + 1) * sizeof(uint16_t);
	VkDeviceSize controllerIndices16Size = 0;
	if (appState.Scene.controllerVerticesSize > 0)
	{
		/*if (appState.LeftController.TrackingResult == ovrSuccess)
		{
			controllerIndices16Size += 2 * 36 * sizeof(uint16_t);
		}
		if (appState.RightController.TrackingResult == ovrSuccess)
		{
			controllerIndices16Size += 2 * 36 * sizeof(uint16_t);
		}*/
	}
	VkDeviceSize indices16Size = floorIndicesSize + coloredIndices16Size + controllerIndices16Size;
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
		coloredIndex16Base = floorIndicesSize;
		memcpy((unsigned char*)indices16->mapped + coloredIndex16Base, d_lists.colored_indices16.data(), coloredIndices16Size);
		controllerIndex16Base = coloredIndex16Base + coloredIndices16Size;
		if (controllerIndices16Size > 0)
		{
			/*auto mapped = (uint16_t*)indices16->mapped + controllerIndex16Base / sizeof(uint16_t);
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
			}*/
		}
		indices16->SubmitIndexBuffer(appState, commandBuffer);
	}
	VkDeviceSize indices32Size = (d_lists.last_colored_index32 + 1) * sizeof(uint32_t);
	if (indices32Size > 0)
	{
		indices32 = cachedIndices32.GetIndexBuffer(appState, indices32Size);
		memcpy((unsigned char*)indices32->mapped, d_lists.colored_indices32.data(), indices32Size);
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

void PerImage::SetPushConstants(const LoadedSurface& loaded, float pushConstants[])
{
	pushConstants[0] = loaded.originX;
	pushConstants[1] = loaded.originY;
	pushConstants[2] = loaded.originZ;
	pushConstants[3] = loaded.yaw * M_PI / 180;
	pushConstants[4] = loaded.pitch * M_PI / 180;
	pushConstants[5] = -loaded.roll * M_PI / 180;
}

void PerImage::SetPushConstants(const LoadedTurbulent& loaded, float pushConstants[])
{
	pushConstants[0] = loaded.originX;
	pushConstants[1] = loaded.originY;
	pushConstants[2] = loaded.originZ;
	pushConstants[3] = loaded.yaw * M_PI / 180;
	pushConstants[4] = loaded.pitch * M_PI / 180;
	pushConstants[5] = -loaded.roll * M_PI / 180;
}

void PerImage::SetPushConstants(const LoadedAlias& loaded, float pushConstants[])
{
	pushConstants[0] = loaded.transform[0][0];
	pushConstants[1] = loaded.transform[2][0];
	pushConstants[2] = -loaded.transform[1][0];
	pushConstants[4] = loaded.transform[0][2];
	pushConstants[5] = loaded.transform[2][2];
	pushConstants[6] = -loaded.transform[1][2];
	pushConstants[8] = -loaded.transform[0][1];
	pushConstants[9] = -loaded.transform[2][1];
	pushConstants[10] = loaded.transform[1][1];
	pushConstants[12] = loaded.transform[0][3];
	pushConstants[13] = loaded.transform[2][3];
	pushConstants[14] = -loaded.transform[1][3];
}

void PerImage::Render(AppState& appState)
{
	if (appState.Scene.lastSurface16 < 0 &&
		appState.Scene.lastSurface32 < 0 &&
		appState.Scene.lastFence16 < 0 &&
		appState.Scene.lastFence32 < 0 &&
		appState.Scene.lastTurbulent16 < 0 &&
		appState.Scene.lastTurbulent32 < 0 &&
		appState.Scene.lastAlias16 < 0 &&
		appState.Scene.lastAlias32 < 0 &&
		appState.Scene.lastViewmodel16 < 0 &&
		appState.Scene.lastViewmodel32 < 0 &&
		appState.Scene.lastSky < 0 &&
		appState.Scene.verticesSize == 0)
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
		CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &host_colormapResources.descriptorPool));
		descriptorSetAllocateInfo.descriptorPool = host_colormapResources.descriptorPool;
		descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
		CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &host_colormapResources.descriptorSet));
		writes[0].dstSet = host_colormapResources.descriptorSet;
		vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
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
			CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = sceneMatricesResources.descriptorPool;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleBufferLayout;
			CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &sceneMatricesResources.descriptorSet));
			writes[0].dstSet = sceneMatricesResources.descriptorSet;
			vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
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
				CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesAndPaletteResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = sceneMatricesAndPaletteResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.bufferAndImageLayout;
				CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &sceneMatricesAndPaletteResources.descriptorSet));
				writes[0].dstSet = sceneMatricesAndPaletteResources.descriptorSet;
				writes[1].dstSet = sceneMatricesAndPaletteResources.descriptorSet;
				vkUpdateDescriptorSets(appState.Device, 2, writes, 0, nullptr);
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
				CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesAndColormapResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = sceneMatricesAndColormapResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.bufferAndTwoImagesLayout;
				CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &sceneMatricesAndColormapResources.descriptorSet));
				writes[0].dstSet = sceneMatricesAndColormapResources.descriptorSet;
				writes[1].dstSet = sceneMatricesAndColormapResources.descriptorSet;
				writes[2].dstSet = sceneMatricesAndColormapResources.descriptorSet;
				vkUpdateDescriptorSets(appState.Device, 3, writes, 0, nullptr);
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
		if (appState.Scene.lastSurface16 >= 0 || appState.Scene.lastSurface32 >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &vertexTransformBase);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
		}
		if (appState.Scene.lastSurface16 >= 0)
		{
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePosition = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastSurface16; i++)
			{
				auto& loaded = appState.Scene.loadedSurfaces16[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = loaded.vertices.buffer;
				}
				auto texturePosition = loaded.texturePosition.buffer;
				if (previousTexturePosition != texturePosition)
				{
					vkCmdBindVertexBuffers(commandBuffer, 2, 1, &texturePosition->buffer, &appState.NoOffset);
					previousTexturePosition = texturePosition;
				}
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.surfaces.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 6 * sizeof(float), pushConstants);
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto lightmap = loaded.lightmap.lightmap;
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 2, 1, &lightmap->descriptorSet, 0, nullptr);
				auto indices = loaded.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, VK_INDEX_TYPE_UINT16);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.offset / 2, 0, 0);
			}
		}
		if (appState.Scene.lastSurface32 >= 0)
		{
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePosition = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastSurface32; i++)
			{
				auto& loaded = appState.Scene.loadedSurfaces32[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = loaded.vertices.buffer;
				}
				auto texturePosition = loaded.texturePosition.buffer;
				if (previousTexturePosition != texturePosition)
				{
					vkCmdBindVertexBuffers(commandBuffer, 2, 1, &texturePosition->buffer, &appState.NoOffset);
					previousTexturePosition = texturePosition;
				}
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.surfaces.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 6 * sizeof(float), pushConstants);
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto lightmap = loaded.lightmap.lightmap;
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 2, 1, &lightmap->descriptorSet, 0, nullptr);
				auto indices = loaded.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, VK_INDEX_TYPE_UINT32);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.offset / 4, 0, 0);
			}
		}
		if (appState.Scene.lastFence16 >= 0 || appState.Scene.lastFence32 >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &vertexTransformBase);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
		}
		if (appState.Scene.lastFence16 >= 0)
		{
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePosition = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastFence16; i++)
			{
				auto& loaded = appState.Scene.loadedFences16[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = loaded.vertices.buffer;
				}
				auto texturePosition = loaded.texturePosition.buffer;
				if (previousTexturePosition != texturePosition)
				{
					vkCmdBindVertexBuffers(commandBuffer, 2, 1, &texturePosition->buffer, &appState.NoOffset);
					previousTexturePosition = texturePosition;
				}
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.fences.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 6 * sizeof(float), pushConstants);
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto lightmap = loaded.lightmap.lightmap;
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 2, 1, &lightmap->descriptorSet, 0, nullptr);
				auto indices = loaded.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, VK_INDEX_TYPE_UINT16);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.offset / 2, 0, 0);
			}
		}
		if (appState.Scene.lastFence32 >= 0)
		{
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePosition = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastFence32; i++)
			{
				auto& loaded = appState.Scene.loadedFences32[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = loaded.vertices.buffer;
				}
				auto texturePosition = loaded.texturePosition.buffer;
				if (previousTexturePosition != texturePosition)
				{
					vkCmdBindVertexBuffers(commandBuffer, 2, 1, &texturePosition->buffer, &appState.NoOffset);
					previousTexturePosition = texturePosition;
				}
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.fences.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 6 * sizeof(float), pushConstants);
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto lightmap = loaded.lightmap.lightmap;
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 2, 1, &lightmap->descriptorSet, 0, nullptr);
				auto indices = loaded.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, VK_INDEX_TYPE_UINT32);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.offset / 4, 0, 0);
			}
		}
		if (appState.Scene.lastSprite >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &texturedVertexBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &texturedAttributeBase);
			vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &vertexTransformBase);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			SharedMemoryTexture* previousTexture = nullptr;
			for (auto i = 0; i <= appState.Scene.lastSprite; i++)
			{
				auto& loaded = appState.Scene.loadedSprites[i];
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				vkCmdDraw(commandBuffer, loaded.count, 1, loaded.firstVertex, 0);
			}
		}
		auto descriptorSetIndex = 0;
		if (appState.Scene.lastTurbulent16 >= 0 || appState.Scene.lastTurbulent32 >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &vertexTransformBase);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[6] = (float)cl.time;
		}
		if (appState.Scene.lastTurbulent16 >= 0)
		{
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePosition = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastTurbulent16; i++)
			{
				auto& loaded = appState.Scene.loadedTurbulent16[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = loaded.vertices.buffer;
				}
				auto texturePosition = loaded.texturePosition.buffer;
				if (previousTexturePosition != texturePosition)
				{
					vkCmdBindVertexBuffers(commandBuffer, 2, 1, &texturePosition->buffer, &appState.NoOffset);
					previousTexturePosition = texturePosition;
				}
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.turbulent.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 7 * sizeof(float), pushConstants);
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, VK_INDEX_TYPE_UINT16);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.offset / 2, 0, 0);
			}
		}
		if (appState.Scene.lastTurbulent32 >= 0)
		{
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePosition = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastTurbulent32; i++)
			{
				auto& loaded = appState.Scene.loadedTurbulent32[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = loaded.vertices.buffer;
				}
				auto texturePosition = loaded.texturePosition.buffer;
				if (previousTexturePosition != texturePosition)
				{
					vkCmdBindVertexBuffers(commandBuffer, 2, 1, &texturePosition->buffer, &appState.NoOffset);
					previousTexturePosition = texturePosition;
				}
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.turbulent.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 7 * sizeof(float), pushConstants);
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, VK_INDEX_TYPE_UINT32);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.offset / 4, 0, 0);
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
					CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &colormapResources.descriptorPool));
					colormapResources.descriptorSetLayouts.resize(toCreate);
					colormapResources.descriptorSets.resize(toCreate);
					colormapResources.bound.resize(toCreate);
					std::fill(colormapResources.descriptorSetLayouts.begin(), colormapResources.descriptorSetLayouts.end(), appState.Scene.singleImageLayout);
					descriptorSetAllocateInfo.descriptorPool = colormapResources.descriptorPool;
					descriptorSetAllocateInfo.descriptorSetCount = toCreate;
					descriptorSetAllocateInfo.pSetLayouts = colormapResources.descriptorSetLayouts.data();
					CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, colormapResources.descriptorSets.data()));
					colormapResources.created = true;
				}
			}
		}
		descriptorSetIndex = 0;
		if (appState.Scene.lastAlias16 >= 0 || appState.Scene.lastAlias32 >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 3, 1, &attributes->buffer, &vertexTransformBase);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
		}
		if (appState.Scene.lastAlias16 >= 0)
		{
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastAlias16; i++)
			{
				auto& loaded = appState.Scene.loadedAlias16[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = colormappedAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.alias.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants);
				if (loaded.isHostColormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr);
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = loaded.colormapped.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo[0].sampler = appState.Scene.samplers[colormap->mipCount];
						textureInfo[0].imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.colormapped.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, VK_INDEX_TYPE_UINT16);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.offset / 2, 0, 0);
			}
		}
		if (appState.Scene.lastAlias32 >= 0)
		{
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastAlias32; i++)
			{
				auto& loaded = appState.Scene.loadedAlias32[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = colormappedAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.alias.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants);
				if (loaded.isHostColormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr);
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = loaded.colormapped.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo[0].sampler = appState.Scene.samplers[colormap->mipCount];
						textureInfo[0].imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.colormapped.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, VK_INDEX_TYPE_UINT32);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.offset / 4, 0, 0);
			}
		}
		if (appState.Scene.lastViewmodel16 >= 0 || appState.Scene.lastViewmodel32 >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 3, 1, &attributes->buffer, &vertexTransformBase);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
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
		if (appState.Scene.lastViewmodel16 >= 0)
		{
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastViewmodel16; i++)
			{
				auto& loaded = appState.Scene.loadedViewmodels16[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = colormappedAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.viewmodel.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 24 * sizeof(float), pushConstants);
				if (loaded.isHostColormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr);
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = loaded.colormapped.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo[0].sampler = appState.Scene.samplers[colormap->mipCount];
						textureInfo[0].imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.colormapped.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, VK_INDEX_TYPE_UINT16);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.offset / 2, 0, 0);
			}
		}
		if (appState.Scene.lastViewmodel32 >= 0)
		{
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastViewmodel32; i++)
			{
				auto& loaded = appState.Scene.loadedViewmodels32[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = colormappedAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.viewmodel.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 24 * sizeof(float), pushConstants);
				if (loaded.isHostColormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr);
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = loaded.colormapped.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo[0].sampler = appState.Scene.samplers[colormap->mipCount];
						textureInfo[0].imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.colormapped.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, VK_INDEX_TYPE_UINT16);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.offset / 4, 0, 0);
			}
		}
		if (appState.Scene.lastColoredIndex16 >= 0 || appState.Scene.lastColoredIndex32 >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.colored.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &coloredVertexBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &vertexTransformBase);
			vkCmdBindVertexBuffers(commandBuffer, 2, 1, &colors->buffer, &appState.NoOffset);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.colored.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			if (appState.Scene.lastColoredIndex16 >= 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, coloredIndex16Base, VK_INDEX_TYPE_UINT16);
				vkCmdDrawIndexed(commandBuffer, appState.Scene.lastColoredIndex16 + 1, 1, 0, 0, 0);
			}
			if (appState.Scene.lastColoredIndex32 >= 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(commandBuffer, appState.Scene.lastColoredIndex32 + 1, 1, 0, 0, 0);
			}
		}
		if (appState.Scene.lastSky >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &texturedVertexBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &texturedAttributeBase);
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			if (!skyResources.created)
			{
				CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &skyResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = skyResources.descriptorPool;
				descriptorSetAllocateInfo.descriptorSetCount = 1;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
				CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &skyResources.descriptorSet));
				textureInfo[0].sampler = appState.Scene.samplers[sky->mipCount];
				textureInfo[0].imageView = sky->view;
				writes[0].dstSet = skyResources.descriptorSet;
				vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
				skyResources.created = true;
			}
			VkDescriptorSet descriptorSets[2];
			descriptorSets[0] = sceneMatricesAndPaletteResources.descriptorSet;
			descriptorSets[1] = skyResources.descriptorSet;
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
			XrMatrix4x4f orientation;
			XrMatrix4x4f_CreateFromQuaternion(&orientation, &appState.Scene.orientation);
			pushConstants[0] = -orientation.m[8];
			pushConstants[1] = orientation.m[10];
			pushConstants[2] = -orientation.m[9];
			pushConstants[3] = orientation.m[0];
			pushConstants[4] = -orientation.m[2];
			pushConstants[5] = orientation.m[1];
			pushConstants[6] = orientation.m[4];
			pushConstants[7] = -orientation.m[6];
			pushConstants[8] = orientation.m[5];
			pushConstants[9] = appState.EyeTextureWidth;
			pushConstants[10] = appState.EyeTextureHeight;
			pushConstants[11] = appState.EyeTextureMaxDimension;
			pushConstants[12] = skytime*skyspeed;
			vkCmdPushConstants(commandBuffer, appState.Scene.sky.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 13 * sizeof(float), &pushConstants);
			vkCmdDraw(commandBuffer, appState.Scene.skyCount, 1, appState.Scene.firstSkyVertex, 0);
		}
	}
	else
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipeline);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
		vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &appState.NoOffset);
		if (!floorResources.created)
		{
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &floorResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = floorResources.descriptorPool;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
			CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &floorResources.descriptorSet));
			textureInfo[0].sampler = appState.Scene.samplers[appState.Scene.floorTexture.mipCount];
			textureInfo[0].imageView = appState.Scene.floorTexture.view;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].pImageInfo = textureInfo;
			writes[0].dstSet = floorResources.descriptorSet;
			vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
			floorResources.created = true;
		}
		VkDescriptorSet descriptorSets[2];
		descriptorSets[0] = sceneMatricesResources.descriptorSet;
		descriptorSets[1] = floorResources.descriptorSet;
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
		vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
	}
	if (appState.Scene.controllerVerticesSize > 0)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipeline);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &controllerVertexBase);
		vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &controllerAttributeBase);
		if (!controllerResources.created)
		{
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &controllerResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = controllerResources.descriptorPool;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
			CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &controllerResources.descriptorSet));
			textureInfo[0].sampler = appState.Scene.samplers[appState.Scene.controllerTexture.mipCount];
			textureInfo[0].imageView = appState.Scene.controllerTexture.view;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].pImageInfo = textureInfo;
			writes[0].dstSet = controllerResources.descriptorSet;
			vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
			controllerResources.created = true;
		}
		VkDescriptorSet descriptorSets[2];
		descriptorSets[0] = sceneMatricesResources.descriptorSet;
		descriptorSets[1] = controllerResources.descriptorSet;
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
		vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, controllerIndex16Base, VK_INDEX_TYPE_UINT16);
		/*VkDeviceSize size = 0;
		if (appState.LeftController.TrackingResult == ovrSuccess)
		{
			size += 2 * 36;
		}
		if (appState.RightController.TrackingResult == ovrSuccess)
		{
			size += 2 * 36;
		}
		vkCmdDrawIndexed(commandBuffer, size, 1, 0, 0, 0);*/
	}
}
