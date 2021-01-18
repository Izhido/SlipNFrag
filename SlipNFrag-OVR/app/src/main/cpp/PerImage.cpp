#include "PerImage.h"
#include "vid_ovr.h"
#include "AppState.h"
#include "d_lists.h"
#include "VulkanCallWrappers.h"

void PerImage::GetStagingBufferSize(AppState& appState, View& view, VkDeviceSize& stagingBufferSize, int& floorSize)
{
	paletteOffset = -1;
	if (palette == nullptr || paletteChanged != pal_changed)
	{
		if (palette == nullptr)
		{
			palette = new Texture();
			palette->Create(appState, commandBuffer, 256, 1, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		paletteOffset = stagingBufferSize;
		stagingBufferSize += 1024;
		paletteChanged = pal_changed;
	}
	host_colormapOffset = -1;
	if (::host_colormap.size() > 0 && host_colormap == nullptr)
	{
		host_colormap = new Texture();
		host_colormap->Create(appState, commandBuffer, 256, 64, VK_FORMAT_R8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		host_colormapOffset = stagingBufferSize;
		stagingBufferSize += 16384;
	}
	if (d_lists.last_surface >= appState.Scene.surfaceList.size())
	{
		appState.Scene.surfaceList.resize(d_lists.last_surface + 1);
	}
	for (auto i = 0; i <= d_lists.last_surface; i++)
	{
		auto& surface = d_lists.surfaces[i];
		auto entry = appState.Scene.surfaces.find({ surface.surface, surface.entity });
		if (entry == appState.Scene.surfaces.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
			auto texture = new TextureFromAllocation();
			texture->Create(appState, commandBuffer, surface.width, surface.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			texture->key.first = surface.surface;
			texture->key.second = surface.entity;
			appState.Scene.surfaces.insert({ texture->key, texture });
			appState.Scene.surfaceList[i].texture = texture;
			appState.Scene.surfaceList[i].offset = stagingBufferSize;
			stagingBufferSize += surface.size;
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
					appState.Scene.surfaceList[i].texture = first;
					appState.Scene.surfaceList[i].offset = stagingBufferSize;
					stagingBufferSize += surface.size;
				}
				else
				{
					auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
					auto texture = new TextureFromAllocation();
					texture->Create(appState, commandBuffer, surface.width, surface.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
					texture->key.first = surface.surface;
					texture->key.second = surface.entity;
					texture->next = first;
					entry->second = texture;
					appState.Scene.surfaceList[i].texture = texture;
					appState.Scene.surfaceList[i].offset = stagingBufferSize;
					stagingBufferSize += surface.size;
				}
			}
			else
			{
				auto found = false;
				for (auto previous = first, texture = first->next; texture != nullptr; previous = texture, texture = texture->next)
				{
					if (texture->unusedCount >= view.framebuffer.swapChainLength)
					{
						found = true;
						texture->unusedCount = 0;
						appState.Scene.surfaceList[i].texture = texture;
						appState.Scene.surfaceList[i].offset = stagingBufferSize;
						stagingBufferSize += surface.size;
						previous->next = texture->next;
						texture->next = first;
						entry->second = texture;
						break;
					}
				}
				if (!found)
				{
					auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
					auto texture = new TextureFromAllocation();
					texture->Create(appState, commandBuffer, surface.width, surface.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
					texture->key.first = surface.surface;
					texture->key.second = surface.entity;
					texture->next = entry->second;
					entry->second = texture;
					appState.Scene.surfaceList[i].texture = texture;
					appState.Scene.surfaceList[i].offset = stagingBufferSize;
					stagingBufferSize += surface.size;
				}
			}
		}
		else
		{
			auto texture = entry->second;
			texture->unusedCount = 0;
			appState.Scene.surfaceList[i].texture = texture;
			appState.Scene.surfaceList[i].offset = -1;
		}
	}
	if (d_lists.last_sprite >= appState.Scene.spriteList.size())
	{
		appState.Scene.spriteList.resize(d_lists.last_sprite + 1);
	}
	for (auto i = 0; i <= d_lists.last_sprite; i++)
	{
		auto& sprite = d_lists.sprites[i];
		auto entry = appState.Scene.spritesPerKey.find(sprite.data);
		if (entry == appState.Scene.spritesPerKey.end())
		{
			SharedMemoryTexture* texture;
			auto mipCount = (int)(std::floor(std::log2(std::max(sprite.width, sprite.height)))) + 1;
			texture = new SharedMemoryTexture();
			texture->Create(appState, commandBuffer, sprite.width, sprite.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			appState.Scene.spriteList[i].offset = stagingBufferSize;
			stagingBufferSize += sprite.size;
			appState.Scene.spriteTextures.MoveToFront(texture);
			appState.Scene.spriteTextureCount++;
			appState.Scene.spritesPerKey.insert({ sprite.data, texture });
			appState.Scene.spriteList[i].texture = texture;
		}
		else
		{
			appState.Scene.spriteList[i].offset = -1;
			appState.Scene.spriteList[i].texture = entry->second;
		}
	}
	if (d_lists.last_turbulent >= appState.Scene.turbulentList.size())
	{
		appState.Scene.turbulentList.resize(d_lists.last_turbulent + 1);
	}
	appState.Scene.turbulentPerKey.clear();
	for (auto i = 0; i <= d_lists.last_turbulent; i++)
	{
		auto& turbulent = d_lists.turbulent[i];
		auto entry = appState.Scene.turbulentPerKey.find(turbulent.texture);
		if (hostClearCount == host_clearcount && entry != appState.Scene.turbulentPerKey.end())
		{
			appState.Scene.turbulentList[i].offset = -1;
			appState.Scene.turbulentList[i].texture = entry->second;
			entry->second->unusedCount = 0;
		}
		else if (hostClearCount == host_clearcount)
		{
			Texture* texture = nullptr;
			for (Texture** t = &this->turbulent.oldTextures; *t != nullptr; t = &(*t)->next)
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
				texture->Create(appState, commandBuffer, turbulent.width, turbulent.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				texture->key = turbulent.texture;
				appState.Scene.turbulentList[i].offset = stagingBufferSize;
				stagingBufferSize += turbulent.size;
			}
			else
			{
				appState.Scene.turbulentList[i].offset = -1;
			}
			appState.Scene.turbulentList[i].texture = texture;
			appState.Scene.turbulentPerKey.insert({ texture->key, texture });
			this->turbulent.MoveToFront(texture);
		}
		else
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
			auto texture = new Texture();
			texture->Create(appState, commandBuffer, turbulent.width, turbulent.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			texture->key = turbulent.texture;
			appState.Scene.turbulentList[i].offset = stagingBufferSize;
			stagingBufferSize += turbulent.size;
			this->turbulent.MoveToFront(texture);
			appState.Scene.turbulentList[i].texture = texture;
			appState.Scene.turbulentPerKey.insert({ texture->key, texture });
		}
	}
	if (d_lists.last_alias >= appState.Scene.aliasList.size())
	{
		appState.Scene.aliasList.resize(d_lists.last_alias + 1);
	}
	for (auto i = 0; i <= d_lists.last_alias; i++)
	{
		auto& alias = d_lists.alias[i];
		auto entry = appState.Scene.aliasPerKey.find(alias.data);
		if (entry == appState.Scene.aliasPerKey.end())
		{
			SharedMemoryTexture* texture;
			auto mipCount = (int)(std::floor(std::log2(std::max(alias.width, alias.height)))) + 1;
			texture = new SharedMemoryTexture();
			texture->Create(appState, commandBuffer, alias.width, alias.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			appState.Scene.aliasList[i].texture.offset = stagingBufferSize;
			stagingBufferSize += alias.size;
			appState.Scene.aliasTextures.MoveToFront(texture);
			appState.Scene.aliasTextureCount++;
			appState.Scene.aliasPerKey.insert({ alias.data, texture });
			appState.Scene.aliasList[i].texture.texture = texture;
		}
		else
		{
			appState.Scene.aliasList[i].texture.offset = -1;
			appState.Scene.aliasList[i].texture.texture = entry->second;
		}
	}
	for (auto i = 0; i <= d_lists.last_alias; i++)
	{
		auto& alias = d_lists.alias[i];
		if (alias.is_host_colormap)
		{
			appState.Scene.aliasList[i].colormap.offset = -1;
			appState.Scene.aliasList[i].colormap.texture = host_colormap;
		}
		else
		{
			appState.Scene.aliasList[i].colormap.offset = stagingBufferSize;
			stagingBufferSize += 16384;
		}
	}
	if (d_lists.last_viewmodel >= appState.Scene.viewmodelList.size())
	{
		appState.Scene.viewmodelList.resize(d_lists.last_viewmodel + 1);
	}
	for (auto i = 0; i <= d_lists.last_viewmodel; i++)
	{
		auto& viewmodel = d_lists.viewmodel[i];
		auto entry = appState.Scene.viewmodelsPerKey.find(viewmodel.data);
		if (entry == appState.Scene.viewmodelsPerKey.end())
		{
			SharedMemoryTexture* texture;
			auto mipCount = (int)(std::floor(std::log2(std::max(viewmodel.width, viewmodel.height)))) + 1;
			texture = new SharedMemoryTexture();
			texture->Create(appState, commandBuffer, viewmodel.width, viewmodel.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			appState.Scene.viewmodelList[i].texture.offset = stagingBufferSize;
			stagingBufferSize += viewmodel.size;
			appState.Scene.viewmodelTextures.MoveToFront(texture);
			appState.Scene.viewmodelTextureCount++;
			appState.Scene.viewmodelsPerKey.insert({ viewmodel.data, texture });
			appState.Scene.viewmodelList[i].texture.texture = texture;
		}
		else
		{
			appState.Scene.viewmodelList[i].texture.offset = -1;
			appState.Scene.viewmodelList[i].texture.texture = entry->second;
		}
	}
	for (auto i = 0; i <= d_lists.last_viewmodel; i++)
	{
		auto& viewmodel = d_lists.viewmodel[i];
		if (viewmodel.is_host_colormap)
		{
			appState.Scene.viewmodelList[i].colormap.offset = -1;
			appState.Scene.viewmodelList[i].colormap.texture = host_colormap;
		}
		else
		{
			appState.Scene.viewmodelList[i].colormap.offset = stagingBufferSize;
			stagingBufferSize += 16384;
		}
	}
	if (d_lists.last_sky >= 0)
	{
		skyOffset = stagingBufferSize;
		stagingBufferSize += 16384;
	}
	else
	{
		skyOffset = -1;
	}
	if (appState.Scene.floorTexture == nullptr)
	{
		appState.Scene.floorOffset = stagingBufferSize;
		floorSize = appState.FloorWidth * appState.FloorHeight * sizeof(uint32_t);
		stagingBufferSize += floorSize;
	}
	else
	{
		appState.Scene.floorOffset = -1;
	}
}

void PerImage::LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer, VkDeviceSize stagingBufferSize, int floorSize)
{
	VK(appState.Device.vkMapMemory(appState.Device.device, stagingBuffer->memory, 0, stagingBufferSize, 0, &stagingBuffer->mapped));
	auto offset = 0;
	if (paletteOffset >= 0)
	{
		memcpy(stagingBuffer->mapped, d_8to24table, 1024);
		offset += 1024;
	}
	if (host_colormapOffset >= 0)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, ::host_colormap.data(), 16384);
		offset += 16384;
	}
	for (auto i = 0; i <= d_lists.last_surface; i++)
	{
		if (appState.Scene.surfaceList[i].offset >= 0)
		{
			auto& surface = d_lists.surfaces[i];
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, surface.data.data(), surface.size);
			offset += surface.size;
		}
	}
	for (auto i = 0; i <= d_lists.last_sprite; i++)
	{
		if (appState.Scene.spriteList[i].offset >= 0)
		{
			auto& sprite = d_lists.sprites[i];
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, sprite.data, sprite.size);
			offset += sprite.size;
		}
	}
	for (auto i = 0; i <= d_lists.last_turbulent; i++)
	{
		if (appState.Scene.turbulentList[i].offset >= 0)
		{
			auto& turbulent = d_lists.turbulent[i];
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, turbulent.data, turbulent.size);
			offset += turbulent.size;
		}
	}
	for (auto i = 0; i <= d_lists.last_alias; i++)
	{
		auto& alias = d_lists.alias[i];
		if (appState.Scene.aliasList[i].texture.offset >= 0)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.data, alias.size);
			offset += alias.size;
		}
		if (!alias.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.colormap.data(), 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= d_lists.last_viewmodel; i++)
	{
		auto& viewmodel = d_lists.viewmodel[i];
		if (appState.Scene.viewmodelList[i].texture.offset >= 0)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.data, viewmodel.size);
			offset += viewmodel.size;
		}
		if (!viewmodel.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.colormap.data(), 16384);
			offset += 16384;
		}
	}
	if (d_lists.last_sky >= 0)
	{
		auto source = r_skysource;
		auto target = ((unsigned char*)stagingBuffer->mapped) + offset;
		for (auto i = 0; i < 128; i++)
		{
			memcpy(target, source, 128);
			source += 256;
			target += 128;
		}
		offset += 16384;
	}
	if (appState.Scene.floorOffset >= 0)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, appState.FloorData.data(), floorSize);
	}
	VkMappedMemoryRange mappedMemoryRange { };
	mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedMemoryRange.memory = stagingBuffer->memory;
	VC(appState.Device.vkFlushMappedMemoryRanges(appState.Device.device, 1, &mappedMemoryRange));
	VC(appState.Device.vkUnmapMemory(appState.Device.device, stagingBuffer->memory));
	stagingBuffer->mapped = nullptr;
}

void PerImage::FillTextures(AppState &appState, Buffer* stagingBuffer)
{
	if (paletteOffset >= 0)
	{
		palette->Fill(appState, stagingBuffer, paletteOffset, commandBuffer);
	}
	if (host_colormapOffset >= 0)
	{
		host_colormap->Fill(appState, stagingBuffer, host_colormapOffset, commandBuffer);
	}
	for (auto i = 0; i <= d_lists.last_surface; i++)
	{
		if (appState.Scene.surfaceList[i].offset >= 0)
		{
			appState.Scene.surfaceList[i].texture->FillMipmapped(appState, stagingBuffer, appState.Scene.surfaceList[i].offset, commandBuffer);
		}
	}
	for (auto i = 0; i <= d_lists.last_sprite; i++)
	{
		if (appState.Scene.spriteList[i].offset >= 0)
		{
			appState.Scene.spriteList[i].texture->FillMipmapped(appState, stagingBuffer, appState.Scene.spriteList[i].offset, commandBuffer);
		}
	}
	for (auto i = 0; i <= d_lists.last_turbulent; i++)
	{
		if (appState.Scene.turbulentList[i].offset >= 0)
		{
			appState.Scene.turbulentList[i].texture->FillMipmapped(appState, stagingBuffer, appState.Scene.turbulentList[i].offset, commandBuffer);
		}
	}
	for (auto i = 0; i <= d_lists.last_alias; i++)
	{
		auto& alias = d_lists.alias[i];
		if (appState.Scene.aliasList[i].texture.offset >= 0)
		{
			appState.Scene.aliasList[i].texture.texture->FillMipmapped(appState, stagingBuffer, appState.Scene.aliasList[i].texture.offset, commandBuffer);
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
				texture->Create(appState, commandBuffer, 256, 64, VK_FORMAT_R8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			}
			texture->Fill(appState, stagingBuffer, appState.Scene.aliasList[i].colormap.offset, commandBuffer);
			colormaps.MoveToFront(texture);
			appState.Scene.aliasList[i].colormap.texture = texture;
			colormapCount++;
		}
	}
	for (auto i = 0; i <= d_lists.last_viewmodel; i++)
	{
		auto& viewmodel = d_lists.viewmodel[i];
		if (appState.Scene.viewmodelList[i].texture.offset >= 0)
		{
			appState.Scene.viewmodelList[i].texture.texture->FillMipmapped(appState, stagingBuffer, appState.Scene.viewmodelList[i].texture.offset, commandBuffer);
		}
		if (!viewmodel.is_host_colormap)
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
				texture->Create(appState, commandBuffer, 256, 64, VK_FORMAT_R8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			}
			texture->Fill(appState, stagingBuffer, appState.Scene.viewmodelList[i].colormap.offset, commandBuffer);
			colormaps.MoveToFront(texture);
			appState.Scene.viewmodelList[i].colormap.texture = texture;
			colormapCount++;
		}
	}
	if (d_lists.last_sky >= 0)
	{
		if (sky == nullptr)
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(128, 128)))) + 1;
			sky = new Texture();
			sky->Create(appState, commandBuffer, 128, 128, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		sky->FillMipmapped(appState, stagingBuffer, skyOffset, commandBuffer);
	}
	if (appState.Mode != AppWorldMode && appState.Scene.floorTexture == nullptr)
	{
		appState.Scene.floorTexture = new Texture();
		appState.Scene.floorTexture->Create(appState, commandBuffer, appState.FloorWidth, appState.FloorHeight, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		appState.Scene.floorTexture->Fill(appState, stagingBuffer, appState.Scene.floorOffset, commandBuffer);
	}
}