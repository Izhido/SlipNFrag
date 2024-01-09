#include "SortedSurfaces.h"
#include "AppState.h"

void SortedSurfaces::Initialize(std::list<SortedSurfaceLightmap>& sorted)
{
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			subEntry.entries.clear();
		}
	}
	addedLightmaps.clear();
}

void SortedSurfaces::Initialize(std::list<SortedSurface2TexturesLightmap>& sorted)
{
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			subEntry.entries.clear();
		}
	}
	added2TexturesLightmaps.clear();
}

void SortedSurfaces::Initialize(std::list<SortedSurfaceTexture>& sorted)
{
	for (auto& entry : sorted)
	{
		entry.entries.clear();
	}
	addedTextures.clear();
}

void SortedSurfaces::Sort(AppState& appState, LoadedSurface& loaded, int index, std::list<SortedSurfaceLightmap>& sorted)
{
	auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = addedLightmaps.find(lightmap);
	if (entry == addedLightmaps.end())
	{
		sorted.push_back({ lightmap });
		auto lightmapEntry = sorted.end();
		lightmapEntry--;
		lightmapEntry->textures.push_back({ texture });
		lightmapEntry->textures.back().entries.push_back(index);
		addedLightmaps.insert({ lightmap, lightmapEntry });
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
			entry->second->textures.push_back({ texture });
			entry->second->textures.back().entries.push_back(index);
		}
	}
}

void SortedSurfaces::Sort(AppState& appState, LoadedSurfaceColoredLights& loaded, int index, std::list<SortedSurfaceLightmap>& sorted)
{
	auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = addedLightmaps.find(lightmap);
	if (entry == addedLightmaps.end())
	{
		sorted.push_back({ lightmap });
		auto lightmapEntry = sorted.end();
		lightmapEntry--;
		lightmapEntry->textures.push_back({ texture });
		lightmapEntry->textures.back().entries.push_back(index);
		addedLightmaps.insert({ lightmap, lightmapEntry });
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
			entry->second->textures.push_back({ texture });
			entry->second->textures.back().entries.push_back(index);
		}
	}
}

void SortedSurfaces::Sort(AppState& appState, LoadedSurface2Textures& loaded, int index, std::list<SortedSurface2TexturesLightmap>& sorted)
{
	auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = added2TexturesLightmaps.find(lightmap);
	if (entry == added2TexturesLightmaps.end())
	{
		sorted.push_back({ lightmap });
		auto lightmapEntry = sorted.end();
		lightmapEntry--;
		lightmapEntry->textures.push_back({ texture, loaded.glowTexture.texture->descriptorSet });
		lightmapEntry->textures.back().entries.push_back(index);
		added2TexturesLightmaps.insert({ lightmap, lightmapEntry });
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

void SortedSurfaces::Sort(AppState& appState, LoadedSurface2TexturesColoredLights& loaded, int index, std::list<SortedSurface2TexturesLightmap>& sorted)
{
	auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = added2TexturesLightmaps.find(lightmap);
	if (entry == added2TexturesLightmaps.end())
	{
		sorted.push_back({ lightmap });
		auto lightmapEntry = sorted.end();
		lightmapEntry--;
		lightmapEntry->textures.push_back({ texture, loaded.glowTexture.texture->descriptorSet });
		lightmapEntry->textures.back().entries.push_back(index);
		added2TexturesLightmaps.insert({ lightmap, lightmapEntry });
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

void SortedSurfaces::Sort(AppState& appState, LoadedTurbulent& loaded, int index, std::list<SortedSurfaceTexture>& sorted)
{
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = addedTextures.find(texture);
	if (entry == addedTextures.end())
	{
		sorted.push_back({ texture });
		auto textureEntry = sorted.end();
		textureEntry--;
		textureEntry->entries.push_back(index);
		addedTextures.insert({ texture, textureEntry });
	}
	else
	{
		entry->second->entries.push_back(index);
	}
}

void SortedSurfaces::Cleanup(std::list<SortedSurfaceLightmap>& sorted)
{
	for (auto entry = sorted.begin(); entry != sorted.end(); )
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
			entry = sorted.erase(entry);
		}
		else
		{
			entry++;
		}
	}
}

void SortedSurfaces::Cleanup(std::list<SortedSurface2TexturesLightmap>& sorted)
{
	for (auto entry = sorted.begin(); entry != sorted.end(); )
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
			entry = sorted.erase(entry);
		}
		else
		{
			entry++;
		}
	}
}

void SortedSurfaces::Cleanup(std::list<SortedSurfaceTexture>& sorted)
{
	for (auto entry = sorted.begin(); entry != sorted.end(); )
	{
		if (entry->entries.empty())
		{
			entry = sorted.erase(entry);
		}
		else
		{
			entry++;
		}
	}
}

float* SortedSurfaces::CopyVertices(LoadedTurbulent& loaded, float* target)
{
    auto face = (msurface_t*)loaded.face;
    auto next_front = face->firstedge;
    auto next_back = next_front + face->numedges;
    auto model = (model_t*)loaded.model;
    auto edge = model->surfedges[next_front];
    unsigned int index;
    if (edge >= 0)
    {
        index = model->edges[edge].v[0];
    }
    else
    {
        index = model->edges[-edge].v[1];
    }
    auto vertexes = model->vertexes;
    auto vertex = vertexes[index].position;
    *target++ = vertex[0];
    *target++ = vertex[1];
    *target++ = vertex[2];
    auto use_back = false;
    for (auto i = 1; i < face->numedges; i++)
    {
        if (use_back)
        {
            next_back--;
            edge = model->surfedges[next_back];
        }
        else
        {
            next_front++;
            edge = model->surfedges[next_front];
        }
        use_back = !use_back;
        if (edge >= 0)
        {
            index = model->edges[edge].v[0];
        }
        else
        {
            index = model->edges[-edge].v[1];
        }
        vertex = vertexes[index].position;
        *target++ = vertex[0];
        *target++ = vertex[1];
        *target++ = vertex[2];
    }
    return target;
}

void SortedSurfaces::LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], target);
			}
		}
	}
	offset += ((unsigned char*)target) - (((unsigned char*)stagingBuffer->mapped) + offset);
}

void SortedSurfaces::LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], target);
			}
		}
	}
	offset += ((unsigned char*)target) - (((unsigned char*)stagingBuffer->mapped) + offset);
}

void SortedSurfaces::LoadVertices(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], target);
			}
		}
	}
	offset += ((unsigned char*)target) - (((unsigned char*)stagingBuffer->mapped) + offset);
}

void SortedSurfaces::LoadVertices(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], target);
			}
		}
	}
	offset += ((unsigned char*)target) - (((unsigned char*)stagingBuffer->mapped) + offset);
}

void SortedSurfaces::LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], target);
			}
		}
	}
	offset += ((unsigned char*)target) - (((unsigned char*)stagingBuffer->mapped) + offset);
}

void SortedSurfaces::LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], target);
			}
		}
	}
	offset += ((unsigned char*)target) - (((unsigned char*)stagingBuffer->mapped) + offset);
}

void SortedSurfaces::LoadVertices(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], target);
			}
		}
	}
	offset += ((unsigned char*)target) - (((unsigned char*)stagingBuffer->mapped) + offset);
}

void SortedSurfaces::LoadVertices(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
                target = CopyVertices(loaded[i], target);
			}
		}
	}
	offset += ((unsigned char*)target) - (((unsigned char*)stagingBuffer->mapped) + offset);
}

void SortedSurfaces::LoadVertices(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	for (auto& entry : sorted)
	{
		for (auto i : entry.entries)
		{
            target = CopyVertices(loaded[i], target);
		}
	}
	offset += ((unsigned char*)target) - (((unsigned char*)stagingBuffer->mapped) + offset);
}

void SortedSurfaces::LoadVertices(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	for (auto& entry : sorted)
	{
		for (auto i : entry.entries)
		{
            target = CopyVertices(loaded[i], target);
		}
	}
	offset += ((unsigned char*)target) - (((unsigned char*)stagingBuffer->mapped) + offset);
}

void SortedSurfaces::LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	XrMatrix4x4f attributes { };
	void* previousFace = nullptr;
	auto target = (XrMatrix4x4f*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
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
				for (auto j = 0; j < surface.count; j++)
				{
					*target++ = attributes;
				}
				attributeCount += surface.count;
			}
		}
	}
	offset += (attributeCount * sizeof(XrMatrix4x4f));
}

void SortedSurfaces::LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	XrMatrix4x4f attributes { };
	void* previousFace = nullptr;
	auto target = (XrMatrix4x4f*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
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
				for (auto j = 0; j < surface.count; j++)
				{
					*target++ = attributes;
				}
				attributeCount += surface.count;
			}
		}
	}
	offset += (attributeCount * sizeof(XrMatrix4x4f));
}

void SortedSurfaces::LoadAttributes(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
				for (auto j = 0; j < surface.count; j++)
				{
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
				}
				attributeCount += surface.count;
			}
		}
	}
	offset += (attributeCount * 20 * sizeof(float));
}

void SortedSurfaces::LoadAttributes(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
				for (auto j = 0; j < surface.count; j++)
				{
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
				}
				attributeCount += surface.count;
			}
		}
	}
	offset += (attributeCount * 20 * sizeof(float));
}

void SortedSurfaces::LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
				for (auto j = 0; j < surface.count; j++)
				{
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
				}
				attributeCount += surface.count;
			}
		}
	}
	offset += (attributeCount * 24 * sizeof(float));
}

void SortedSurfaces::LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
				for (auto j = 0; j < surface.count; j++)
				{
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
				}
				attributeCount += surface.count;
			}
		}
	}
	offset += (attributeCount * 24 * sizeof(float));
}

void SortedSurfaces::LoadAttributes(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
				for (auto j = 0; j < surface.count; j++)
				{
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
				}
				attributeCount += surface.count;
			}
		}
	}
	offset += (attributeCount * 24 * sizeof(float));
}

void SortedSurfaces::LoadAttributes(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.face;
				for (auto j = 0; j < surface.count; j++)
				{
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
				}
				attributeCount += surface.count;
			}
		}
	}
	offset += (attributeCount * 24 * sizeof(float));
}

void SortedSurfaces::LoadAttributes(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted)
	{
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			auto face = (msurface_t*)surface.face;
			for (auto j = 0; j < surface.count; j++)
			{
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
			}
			attributeCount += surface.count;
		}
	}
	offset += (attributeCount * 12 * sizeof(float));
}

void SortedSurfaces::LoadAttributes(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted)
	{
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			auto face = (msurface_t*)surface.face;
			for (auto j = 0; j < surface.count; j++)
			{
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
			}
			attributeCount += surface.count;
		}
	}
	offset += (attributeCount * 20 * sizeof(float));
}

void SortedSurfaces::LoadIndices16(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index16Count = 0;
	uint16_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index16Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index16Count * sizeof(uint16_t));
}

void SortedSurfaces::LoadIndices16(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index16Count = 0;
	uint16_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index16Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index16Count * sizeof(uint16_t));
}

void SortedSurfaces::LoadIndices16(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index16Count = 0;
	uint16_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index16Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index16Count * sizeof(uint16_t));
}

void SortedSurfaces::LoadIndices16(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index16Count = 0;
	uint16_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index16Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index16Count * sizeof(uint16_t));
}

void SortedSurfaces::LoadIndices16(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index16Count = 0;
	uint16_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index16Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index16Count * sizeof(uint16_t));
}

void SortedSurfaces::LoadIndices16(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index16Count = 0;
	uint16_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index16Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index16Count * sizeof(uint16_t));
}

void SortedSurfaces::LoadIndices16(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index16Count = 0;
	uint16_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index16Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index16Count * sizeof(uint16_t));
}

void SortedSurfaces::LoadIndices16(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index16Count = 0;
	uint16_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index16Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index16Count * sizeof(uint16_t));
}

void SortedSurfaces::LoadIndices16(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index16Count = 0;
	uint16_t index = 0;
	for (auto& entry : sorted)
	{
		VkDeviceSize indexCount = 0;
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			*target++ = index++;
			*target++ = index++;
			*target++ = index++;
			auto revert = true;
			for (auto j = 1; j < surface.count - 2; j++)
			{
				if (revert)
				{
					*target++ = index;
					*target++ = index - 1;
					*target++ = index - 2;
				}
				else
				{
					*target++ = index - 2;
					*target++ = index - 1;
					*target++ = index;
				}
				index++;
				revert = !revert;
			}
			auto count = (surface.count - 2) * 3;
			indexCount += count;
			index16Count += count;
		}
		entry.indexCount = indexCount;
	}
	offset += (index16Count * sizeof(uint16_t));
}

void SortedSurfaces::LoadIndices16(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index16Count = 0;
	uint16_t index = 0;
	for (auto& entry : sorted)
	{
		VkDeviceSize indexCount = 0;
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			*target++ = index++;
			*target++ = index++;
			*target++ = index++;
			auto revert = true;
			for (auto j = 1; j < surface.count - 2; j++)
			{
				if (revert)
				{
					*target++ = index;
					*target++ = index - 1;
					*target++ = index - 2;
				}
				else
				{
					*target++ = index - 2;
					*target++ = index - 1;
					*target++ = index;
				}
				index++;
				revert = !revert;
			}
			auto count = (surface.count - 2) * 3;
			indexCount += count;
			index16Count += count;
		}
		entry.indexCount = indexCount;
	}
	offset += (index16Count * sizeof(uint16_t));
}

void SortedSurfaces::LoadIndices32(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index32Count = 0;
	uint32_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index32Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index32Count * sizeof(uint32_t));
}

void SortedSurfaces::LoadIndices32(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index32Count = 0;
	uint32_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index32Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index32Count * sizeof(uint32_t));
}

void SortedSurfaces::LoadIndices32(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index32Count = 0;
	uint32_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index32Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index32Count * sizeof(uint32_t));
}

void SortedSurfaces::LoadIndices32(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index32Count = 0;
	uint32_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index32Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index32Count * sizeof(uint32_t));
}

void SortedSurfaces::LoadIndices32(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index32Count = 0;
	uint32_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index32Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index32Count * sizeof(uint32_t));
}

void SortedSurfaces::LoadIndices32(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index32Count = 0;
	uint32_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index32Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index32Count * sizeof(uint32_t));
}

void SortedSurfaces::LoadIndices32(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index32Count = 0;
	uint32_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index32Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index32Count * sizeof(uint32_t));
}

void SortedSurfaces::LoadIndices32(std::list<SortedSurface2TexturesLightmap>& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index32Count = 0;
	uint32_t index = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < surface.count - 2; j++)
				{
					if (revert)
					{
						*target++ = index;
						*target++ = index - 1;
						*target++ = index - 2;
					}
					else
					{
						*target++ = index - 2;
						*target++ = index - 1;
						*target++ = index;
					}
					index++;
					revert = !revert;
				}
				auto count = (surface.count - 2) * 3;
				indexCount += count;
				index32Count += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
	offset += (index32Count * sizeof(uint32_t));
}

void SortedSurfaces::LoadIndices32(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index32Count = 0;
	uint32_t index = 0;
	for (auto& entry : sorted)
	{
		VkDeviceSize indexCount = 0;
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			*target++ = index++;
			*target++ = index++;
			*target++ = index++;
			auto revert = true;
			for (auto j = 1; j < surface.count - 2; j++)
			{
				if (revert)
				{
					*target++ = index;
					*target++ = index - 1;
					*target++ = index - 2;
				}
				else
				{
					*target++ = index - 2;
					*target++ = index - 1;
					*target++ = index;
				}
				index++;
				revert = !revert;
			}
			auto count = (surface.count - 2) * 3;
			indexCount += count;
			index32Count += count;
		}
		entry.indexCount = indexCount;
	}
	offset += (index32Count * sizeof(uint32_t));
}

void SortedSurfaces::LoadIndices32(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index32Count = 0;
	uint32_t index = 0;
	for (auto& entry : sorted)
	{
		VkDeviceSize indexCount = 0;
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			*target++ = index++;
			*target++ = index++;
			*target++ = index++;
			auto revert = true;
			for (auto j = 1; j < surface.count - 2; j++)
			{
				if (revert)
				{
					*target++ = index;
					*target++ = index - 1;
					*target++ = index - 2;
				}
				else
				{
					*target++ = index - 2;
					*target++ = index - 1;
					*target++ = index;
				}
				index++;
				revert = !revert;
			}
			auto count = (surface.count - 2) * 3;
			indexCount += count;
			index32Count += count;
		}
		entry.indexCount = indexCount;
	}
	offset += (index32Count * sizeof(uint32_t));
}
