#include "SortedSurfaces.h"
#include "AppState.h"

void SortedSurfaces::Initialize(SortedSurfaceLightmapsWithTextures& sorted)
{
	for (auto l = 0; l < sorted.count; l++)
	{
        auto& lightmap = sorted.lightmaps[l];
        lightmap.textures.clear();
	}
    sorted.count = 0;
	sorted.added.clear();
}

void SortedSurfaces::Initialize(SortedSurfaceLightmapsWith2Textures& sorted)
{
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			subEntry.entries.clear();
		}
	}
    sorted.added.clear();
}

void SortedSurfaces::Initialize(SortedSurfaceTextures& sorted)
{
	for (auto& entry : sorted.textures)
	{
		entry.entries.clear();
	}
    sorted.added.clear();
}

void SortedSurfaces::Sort(AppState& appState, LoadedSurface& loaded, int index, SortedSurfaceLightmapsWithTextures& sorted)
{
	auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = sorted.added.find(lightmap);
	if (entry == sorted.added.end())
	{
        if (sorted.count >= sorted.lightmaps.size())
        {
            sorted.lightmaps.resize(sorted.lightmaps.size() + 32);
        }
        auto& sortedLightmap = sorted.lightmaps[sorted.count];
        sortedLightmap.lightmap = lightmap;
        sortedLightmap.textures.push_back({ texture });
        sortedLightmap.textures.back().entries.push_back(index);
        sorted.added.insert({ lightmap, sorted.count });
        sorted.count++;
	}
	else
	{
		auto subEntryFound = false;
        auto& sortedLightmap = sorted.lightmaps[entry->second];
		for (auto& subEntry : sortedLightmap.textures)
		{
			if (subEntry.texture == texture)
			{
				subEntryFound = true;
				subEntry.entries.push_back(index);
				break;
			}
		}
		if (!subEntryFound)
		{
            sortedLightmap.textures.push_back({ texture });
            sortedLightmap.textures.back().entries.push_back(index);
		}
	}
}

void SortedSurfaces::Sort(AppState& appState, LoadedSurfaceColoredLights& loaded, int index, SortedSurfaceLightmapsWithTextures& sorted)
{
    auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
    auto texture = loaded.texture.texture->descriptorSet;
    auto entry = sorted.added.find(lightmap);
    if (entry == sorted.added.end())
    {
        if (sorted.count >= sorted.lightmaps.size())
        {
            sorted.lightmaps.resize(sorted.lightmaps.size() + 32);
        }
        auto& sortedLightmap = sorted.lightmaps[sorted.count];
        sortedLightmap.lightmap = lightmap;
        sortedLightmap.textures.push_back({ texture });
        sortedLightmap.textures.back().entries.push_back(index);
        sorted.added.insert({ lightmap, sorted.count });
        sorted.count++;
    }
    else
    {
        auto subEntryFound = false;
        auto& sortedLightmap = sorted.lightmaps[entry->second];
        for (auto& subEntry : sortedLightmap.textures)
        {
            if (subEntry.texture == texture)
            {
                subEntryFound = true;
                subEntry.entries.push_back(index);
                break;
            }
        }
        if (!subEntryFound)
        {
            sortedLightmap.textures.push_back({ texture });
            sortedLightmap.textures.back().entries.push_back(index);
        }
    }
}

void SortedSurfaces::Sort(AppState& appState, LoadedSurface2Textures& loaded, int index, SortedSurfaceLightmapsWith2Textures& sorted)
{
	auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = sorted.added.find(lightmap);
	if (entry == sorted.added.end())
	{
		sorted.lightmaps.push_back({ lightmap });
		auto lightmapEntry = sorted.lightmaps.end();
		lightmapEntry--;
		lightmapEntry->textures.push_back({ texture, loaded.glowTexture.texture->descriptorSet });
		lightmapEntry->textures.back().entries.push_back(index);
        sorted.added.insert({ lightmap, lightmapEntry });
	}
	else
	{
		auto subEntryFound = false;
		for (auto& subEntry : entry->second->textures)
		{
			if (subEntry.texture == texture)
			{
				subEntryFound = true;
				subEntry.entries.push_back(index);
				break;
			}
		}
		if (!subEntryFound)
		{
			entry->second->textures.push_back({ texture, loaded.glowTexture.texture->descriptorSet });
			entry->second->textures.back().entries.push_back(index);
		}
	}
}

void SortedSurfaces::Sort(AppState& appState, LoadedSurface2TexturesColoredLights& loaded, int index, SortedSurfaceLightmapsWith2Textures& sorted)
{
	auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = sorted.added.find(lightmap);
	if (entry == sorted.added.end())
	{
		sorted.lightmaps.push_back({ lightmap });
		auto lightmapEntry = sorted.lightmaps.end();
		lightmapEntry--;
		lightmapEntry->textures.push_back({ texture, loaded.glowTexture.texture->descriptorSet });
		lightmapEntry->textures.back().entries.push_back(index);
        sorted.added.insert({ lightmap, lightmapEntry });
	}
	else
	{
		auto subEntryFound = false;
		for (auto& subEntry : entry->second->textures)
		{
			if (subEntry.texture == texture)
			{
				subEntryFound = true;
				subEntry.entries.push_back(index);
				break;
			}
		}
		if (!subEntryFound)
		{
			entry->second->textures.push_back({ texture, loaded.glowTexture.texture->descriptorSet });
			entry->second->textures.back().entries.push_back(index);
		}
	}
}

void SortedSurfaces::Sort(AppState& appState, LoadedTurbulent& loaded, int index, SortedSurfaceTextures& sorted)
{
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = sorted.added.find(texture);
	if (entry == sorted.added.end())
	{
		sorted.textures.push_back({ texture });
		auto textureEntry = sorted.textures.end();
		textureEntry--;
		textureEntry->entries.push_back(index);
        sorted.added.insert({ texture, textureEntry });
	}
	else
	{
		entry->second->entries.push_back(index);
	}
}

void SortedSurfaces::Cleanup(SortedSurfaceLightmapsWith2Textures& sorted)
{
	for (auto entry = sorted.lightmaps.begin(); entry != sorted.lightmaps.end(); )
	{
		for (auto subEntry = entry->textures.begin(); subEntry != entry->textures.end(); )
		{
			if (subEntry->entries.empty())
			{
				subEntry = entry->textures.erase(subEntry);
			}
			else
			{
				subEntry++;
			}
		}
		if (entry->textures.empty())
		{
			entry = sorted.lightmaps.erase(entry);
		}
		else
		{
			entry++;
		}
	}
}

void SortedSurfaces::Cleanup(SortedSurfaceTextures& sorted)
{
	for (auto entry = sorted.textures.begin(); entry != sorted.textures.end(); )
	{
		if (entry->entries.empty())
		{
			entry = sorted.textures.erase(entry);
		}
		else
		{
			entry++;
		}
	}
}

float* SortedSurfaces::CopyVertices(LoadedTurbulent& loaded, float attributeIndex, float* target)
{
    auto source = loaded.vertices;
    for (auto i = 0; i < loaded.numedges; i++)
    {
	    *target++ = *source++;
	    *target++ = *source++;
	    *target++ = *source++;
	    *target++ = attributeIndex;
    }
    return target;
}

void SortedSurfaces::LoadVertices(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurface>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
                attributeIndexAsFloat += 4;
			}
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
                attributeIndexAsFloat += 4;
			}
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2Textures>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
                attributeIndexAsFloat += 5;
			}
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
                attributeIndexAsFloat += 5;
			}
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotated>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
                attributeIndexAsFloat += 6;
			}
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
                attributeIndexAsFloat += 6;
			}
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
                attributeIndexAsFloat += 6;
			}
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
                attributeIndexAsFloat += 6;
			}
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
	for (auto& entry : sorted.textures)
	{
		for (auto i : entry.entries)
		{
            target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
            attributeIndexAsFloat += 3;
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
	for (auto& entry : sorted.textures)
	{
		for (auto i : entry.entries)
		{
            target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
            attributeIndexAsFloat += 5;
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	XrMatrix4x4f attributes { };
	void* previousFace = nullptr;
	auto target = (XrMatrix4x4f*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
				if (previousFace != face)
				{
					attributes.m[0] = face->texinfo->vecs[0][0];
					attributes.m[1] = face->texinfo->vecs[0][1];
					attributes.m[2] = face->texinfo->vecs[0][2];
					attributes.m[3] = face->texinfo->vecs[0][3];
					attributes.m[4] = face->texinfo->vecs[1][0];
					attributes.m[5] = face->texinfo->vecs[1][1];
					attributes.m[6] = face->texinfo->vecs[1][2];
					attributes.m[7] = face->texinfo->vecs[1][3];
					attributes.m[8] = face->texturemins[0];
					attributes.m[9] = face->texturemins[1];
					attributes.m[10] = face->extents[0];
					attributes.m[11] = face->extents[1];
					attributes.m[12] = face->texinfo->texture->width;
					attributes.m[13] = face->texinfo->texture->height;
					previousFace = face;
				}
				attributes.m[14] = surface.lightmap.lightmap->allocatedIndex;
				attributes.m[15] = surface.texture.index;
                *target++ = attributes;
				attributeCount++;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	XrMatrix4x4f attributes { };
	void* previousFace = nullptr;
	auto target = (XrMatrix4x4f*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
				if (previousFace != face)
				{
					attributes.m[0] = face->texinfo->vecs[0][0];
					attributes.m[1] = face->texinfo->vecs[0][1];
					attributes.m[2] = face->texinfo->vecs[0][2];
					attributes.m[3] = face->texinfo->vecs[0][3];
					attributes.m[4] = face->texinfo->vecs[1][0];
					attributes.m[5] = face->texinfo->vecs[1][1];
					attributes.m[6] = face->texinfo->vecs[1][2];
					attributes.m[7] = face->texinfo->vecs[1][3];
					attributes.m[8] = face->texturemins[0];
					attributes.m[9] = face->texturemins[1];
					attributes.m[10] = face->extents[0];
					attributes.m[11] = face->extents[1];
					attributes.m[12] = face->texinfo->texture->width;
					attributes.m[13] = face->texinfo->texture->height;
					previousFace = face;
				}
				attributes.m[14] = surface.lightmap.lightmap->allocatedIndex;
				attributes.m[15] = surface.texture.index;
                *target++ = attributes;
				attributeCount++;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
                *target++ = face->texinfo->vecs[0][0];
                *target++ = face->texinfo->vecs[0][1];
                *target++ = face->texinfo->vecs[0][2];
                *target++ = face->texinfo->vecs[0][3];
                *target++ = face->texinfo->vecs[1][0];
                *target++ = face->texinfo->vecs[1][1];
                *target++ = face->texinfo->vecs[1][2];
                *target++ = face->texinfo->vecs[1][3];
                *target++ = face->texturemins[0];
                *target++ = face->texturemins[1];
                *target++ = face->extents[0];
                *target++ = face->extents[1];
                *target++ = face->texinfo->texture->width;
                *target++ = face->texinfo->texture->height;
                *target++ = surface.lightmap.lightmap->allocatedIndex;
                *target++ = surface.texture.index;
                *target++ = surface.glowTexture.index;
                *target++ = 0;
                *target++ = 0;
                *target++ = 0;
				attributeCount++;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
                *target++ = face->texinfo->vecs[0][0];
                *target++ = face->texinfo->vecs[0][1];
                *target++ = face->texinfo->vecs[0][2];
                *target++ = face->texinfo->vecs[0][3];
                *target++ = face->texinfo->vecs[1][0];
                *target++ = face->texinfo->vecs[1][1];
                *target++ = face->texinfo->vecs[1][2];
                *target++ = face->texinfo->vecs[1][3];
                *target++ = face->texturemins[0];
                *target++ = face->texturemins[1];
                *target++ = face->extents[0];
                *target++ = face->extents[1];
                *target++ = face->texinfo->texture->width;
                *target++ = face->texinfo->texture->height;
                *target++ = surface.lightmap.lightmap->allocatedIndex;
                *target++ = surface.texture.index;
                *target++ = surface.glowTexture.index;
                *target++ = 0;
                *target++ = 0;
                *target++ = 0;
				attributeCount++;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
                *target++ = surface.originX;
                *target++ = surface.originY;
                *target++ = surface.originZ;
                *target++ = 1;
                *target++ = surface.yaw * M_PI / 180;
                *target++ = surface.pitch * M_PI / 180;
                *target++ = surface.roll * M_PI / 180;
                *target++ = 0;
                *target++ = face->texinfo->vecs[0][0];
                *target++ = face->texinfo->vecs[0][1];
                *target++ = face->texinfo->vecs[0][2];
                *target++ = face->texinfo->vecs[0][3];
                *target++ = face->texinfo->vecs[1][0];
                *target++ = face->texinfo->vecs[1][1];
                *target++ = face->texinfo->vecs[1][2];
                *target++ = face->texinfo->vecs[1][3];
                *target++ = face->texturemins[0];
                *target++ = face->texturemins[1];
                *target++ = face->extents[0];
                *target++ = face->extents[1];
                *target++ = face->texinfo->texture->width;
                *target++ = face->texinfo->texture->height;
                *target++ = surface.lightmap.lightmap->allocatedIndex;
                *target++ = surface.texture.index;
				attributeCount++;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
                *target++ = surface.originX;
                *target++ = surface.originY;
                *target++ = surface.originZ;
                *target++ = 1;
                *target++ = surface.yaw * M_PI / 180;
                *target++ = surface.pitch * M_PI / 180;
                *target++ = surface.roll * M_PI / 180;
                *target++ = 0;
                *target++ = face->texinfo->vecs[0][0];
                *target++ = face->texinfo->vecs[0][1];
                *target++ = face->texinfo->vecs[0][2];
                *target++ = face->texinfo->vecs[0][3];
                *target++ = face->texinfo->vecs[1][0];
                *target++ = face->texinfo->vecs[1][1];
                *target++ = face->texinfo->vecs[1][2];
                *target++ = face->texinfo->vecs[1][3];
                *target++ = face->texturemins[0];
                *target++ = face->texturemins[1];
                *target++ = face->extents[0];
                *target++ = face->extents[1];
                *target++ = face->texinfo->texture->width;
                *target++ = face->texinfo->texture->height;
                *target++ = surface.lightmap.lightmap->allocatedIndex;
                *target++ = surface.texture.index;
				attributeCount++;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
                *target++ = surface.originX;
                *target++ = surface.originY;
                *target++ = surface.originZ;
                *target++ = 1;
                *target++ = surface.yaw * M_PI / 180;
                *target++ = surface.pitch * M_PI / 180;
                *target++ = surface.roll * M_PI / 180;
                *target++ = surface.glowTexture.index;
                *target++ = face->texinfo->vecs[0][0];
                *target++ = face->texinfo->vecs[0][1];
                *target++ = face->texinfo->vecs[0][2];
                *target++ = face->texinfo->vecs[0][3];
                *target++ = face->texinfo->vecs[1][0];
                *target++ = face->texinfo->vecs[1][1];
                *target++ = face->texinfo->vecs[1][2];
                *target++ = face->texinfo->vecs[1][3];
                *target++ = face->texturemins[0];
                *target++ = face->texturemins[1];
                *target++ = face->extents[0];
                *target++ = face->extents[1];
                *target++ = face->texinfo->texture->width;
                *target++ = face->texinfo->texture->height;
                *target++ = surface.lightmap.lightmap->allocatedIndex;
                *target++ = surface.texture.index;
				attributeCount++;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
                *target++ = surface.originX;
                *target++ = surface.originY;
                *target++ = surface.originZ;
                *target++ = 1;
                *target++ = surface.yaw * M_PI / 180;
                *target++ = surface.pitch * M_PI / 180;
                *target++ = surface.roll * M_PI / 180;
                *target++ = surface.glowTexture.index;
                *target++ = face->texinfo->vecs[0][0];
                *target++ = face->texinfo->vecs[0][1];
                *target++ = face->texinfo->vecs[0][2];
                *target++ = face->texinfo->vecs[0][3];
                *target++ = face->texinfo->vecs[1][0];
                *target++ = face->texinfo->vecs[1][1];
                *target++ = face->texinfo->vecs[1][2];
                *target++ = face->texinfo->vecs[1][3];
                *target++ = face->texturemins[0];
                *target++ = face->texturemins[1];
                *target++ = face->extents[0];
                *target++ = face->extents[1];
                *target++ = face->texinfo->texture->width;
                *target++ = face->texinfo->texture->height;
                *target++ = surface.lightmap.lightmap->allocatedIndex;
                *target++ = surface.texture.index;
				attributeCount++;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted.textures)
	{
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			auto face = (msurface_t*)surface.face;
            *target++ = face->texinfo->vecs[0][0];
            *target++ = face->texinfo->vecs[0][1];
            *target++ = face->texinfo->vecs[0][2];
            *target++ = face->texinfo->vecs[0][3];
            *target++ = face->texinfo->vecs[1][0];
            *target++ = face->texinfo->vecs[1][1];
            *target++ = face->texinfo->vecs[1][2];
            *target++ = face->texinfo->vecs[1][3];
            *target++ = face->texinfo->texture->width;
            *target++ = face->texinfo->texture->height;
            *target++ = surface.texture.index;
            *target++ = 0;
			attributeCount++;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted.textures)
	{
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			auto face = (msurface_t*)surface.face;
            *target++ = surface.originX;
            *target++ = surface.originY;
            *target++ = surface.originZ;
            *target++ = 1;
            *target++ = surface.yaw * M_PI / 180;
            *target++ = surface.pitch * M_PI / 180;
            *target++ = surface.roll * M_PI / 180;
            *target++ = 0;
            *target++ = face->texinfo->vecs[0][0];
            *target++ = face->texinfo->vecs[0][1];
            *target++ = face->texinfo->vecs[0][2];
            *target++ = face->texinfo->vecs[0][3];
            *target++ = face->texinfo->vecs[1][0];
            *target++ = face->texinfo->vecs[1][1];
            *target++ = face->texinfo->vecs[1][2];
            *target++ = face->texinfo->vecs[1][3];
            *target++ = face->texinfo->texture->width;
            *target++ = face->texinfo->texture->height;
            *target++ = surface.texture.index;
            *target++ = 0;
			attributeCount++;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
	for (auto& entry : sorted.textures)
	{
		VkDeviceSize indexCount = 0;
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			auto count = surface.count - 2;
			auto first = index;
			*target++ = index++;
			*target++ = index++;
			*target++ = index;
			for (auto j = 1; j < count; j++)
			{
				*target++ = first;
				*target++ = index++;
				*target++ = index;
			}
			index++;
			count *= 3;
			indexCount += count;
		}
		entry.indexCount = indexCount;
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
	for (auto& entry : sorted.textures)
	{
		VkDeviceSize indexCount = 0;
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			auto count = surface.count - 2;
			auto first = index;
			*target++ = index++;
			*target++ = index++;
			*target++ = index;
			for (auto j = 1; j < count; j++)
			{
				*target++ = first;
				*target++ = index++;
				*target++ = index;
			}
			index++;
			count *= 3;
			indexCount += count;
		}
		entry.indexCount = indexCount;
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceLightmapsWithTextures& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
		for (auto& subEntry : lightmap.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
	for (auto& entry : sorted.lightmaps)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto count = surface.count - 2;
				auto first = index;
				*target++ = index++;
				*target++ = index++;
				*target++ = index;
				for (auto j = 1; j < count; j++)
				{
					*target++ = first;
					*target++ = index++;
					*target++ = index;
				}
				index++;
				count *= 3;
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
	for (auto& entry : sorted.textures)
	{
		VkDeviceSize indexCount = 0;
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			auto count = surface.count - 2;
			auto first = index;
			*target++ = index++;
			*target++ = index++;
			*target++ = index;
			for (auto j = 1; j < count; j++)
			{
				*target++ = first;
				*target++ = index++;
				*target++ = index;
			}
			index++;
			count *= 3;
			indexCount += count;
		}
		entry.indexCount = indexCount;
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
	for (auto& entry : sorted.textures)
	{
		VkDeviceSize indexCount = 0;
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			auto count = surface.count - 2;
			auto first = index;
			*target++ = index++;
			*target++ = index++;
			*target++ = index;
			for (auto j = 1; j < count; j++)
			{
				*target++ = first;
				*target++ = index++;
				*target++ = index;
			}
			index++;
			count *= 3;
			indexCount += count;
		}
		entry.indexCount = indexCount;
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}
