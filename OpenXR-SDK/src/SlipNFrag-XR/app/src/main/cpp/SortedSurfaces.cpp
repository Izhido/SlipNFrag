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

void SortedSurfaces::Initialize(std::list<SortedSurfaceTexture>& sorted)
{
	for (auto& entry : sorted)
	{
		entry.entries.clear();
	}
	addedTextures.clear();
}

void SortedSurfaces::Sort(LoadedSurface& loaded, int index, std::list<SortedSurfaceLightmap>& sorted)
{
	auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = addedLightmaps.find(lightmap);
	if (entry == addedLightmaps.end())
	{
		sorted.push_back({lightmap});
		auto lightmapEntry = sorted.end();
		lightmapEntry--;
		lightmapEntry->textures.push_back({ texture });
		lightmapEntry->textures.back().entries.push_back(index);
		addedLightmaps.insert({lightmap, lightmapEntry});
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

void SortedSurfaces::Sort(LoadedTurbulent& loaded, int index, std::list<SortedSurfaceTexture>& sorted)
{
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = addedTextures.find(texture);
	if (entry == addedTextures.end())
	{
		sorted.push_back({ texture });
		auto textureEntry = sorted.end();
		textureEntry--;
		textureEntry->entries.push_back(index);
		addedTextures.insert({texture, textureEntry});
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

float* SortedSurfaces::LoadVertices(AppState& appState, LoadedTurbulent& loaded, float* target)
{
	auto face = (msurface_t*)loaded.face;
	auto size = face->numedges * 4;
	auto entry = appState.Scene.surfaceVertexCache.find(face);
	if (entry == appState.Scene.surfaceVertexCache.end())
	{
		appState.Scene.surfaceVertexCache.emplace(face, size);
		auto cached = appState.Scene.surfaceVertexCache[face].data();
		auto model = (model_t*)loaded.model;
		auto edge = model->surfedges[face->firstedge];
		unsigned int index;
		if (edge >= 0)
		{
			index = model->edges[edge].v[0];
		}
		else
		{
			index = model->edges[-edge].v[1];
		}
		auto source = (float*)loaded.vertexes + index * 3;
		*cached++ = *source;
		*target++ = *source++;
		*cached++ = *source;
		*target++ = *source++;
		*cached++ = *source;
		*target++ = *source;
		*cached++ = 1;
		*target++ = 1;
		auto next_front = 0;
		auto next_back = face->numedges;
		auto use_back = false;
		for (auto j = 1; j < face->numedges; j++)
		{
			if (use_back)
			{
				next_back--;
				edge = model->surfedges[face->firstedge + next_back];
			}
			else
			{
				next_front++;
				edge = model->surfedges[face->firstedge + next_front];
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
			source = (float*)loaded.vertexes + index * 3;
			*cached++ = *source;
			*target++ = *source++;
			*cached++ = *source;
			*target++ = *source++;
			*cached++ = *source;
			*target++ = *source;
			*cached++ = 1;
			*target++ = 1;
		}
	}
	else
	{
		memcpy(target, entry->second.data(), size * sizeof(float));
		target += size;
	}
	return target;
}

void SortedSurfaces::LoadVertices(AppState& appState, std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				target = LoadVertices(appState, loaded[i], target);
			}
		}
	}
	offset += ((unsigned char*)target) - (((unsigned char*)stagingBuffer->mapped) + offset);
}

void SortedSurfaces::LoadVertices(AppState& appState, std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				target = LoadVertices(appState, loaded[i], target);
			}
		}
	}
	offset += ((unsigned char*)target) - (((unsigned char*)stagingBuffer->mapped) + offset);
}

void SortedSurfaces::LoadVertices(AppState& appState, std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	for (auto& entry : sorted)
	{
		for (auto i : entry.entries)
		{
			target = LoadVertices(appState, loaded[i], target);
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
				for (auto j = 0; j < face->numedges; j++)
				{
					*target++ = attributes;
				}
				attributeCount += face->numedges;
			}
		}
	}
	offset += (attributeCount * sizeof(XrMatrix4x4f));
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
				for (auto j = 0; j < face->numedges; j++)
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
					*target++ = 0;
					*target++ = surface.originX;
					*target++ = surface.originY;
					*target++ = surface.originZ;
					*target++ = 1;
					*target++ = surface.yaw * M_PI / 180;
					*target++ = surface.pitch * M_PI / 180;
					*target++ = surface.roll * M_PI / 180;
					*target++ = 0;
				}
				attributeCount += face->numedges;
			}
		}
	}
	offset += (attributeCount * 24 * sizeof(float));
}

void SortedSurfaces::LoadAttributes(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	XrMatrix4x4f attributes { };
	void* previousFace = nullptr;
	auto target = (XrMatrix4x4f*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted)
	{
		for (auto i : entry.entries)
		{
			auto& turbulent = loaded[i];
			auto face = (msurface_t*)turbulent.face;
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
				attributes.m[8] = face->texinfo->texture->width;
				attributes.m[9] = face->texinfo->texture->height;
				previousFace = face;
			}
			for (auto j = 0; j < face->numedges; j++)
			{
				*target++ = attributes;
			}
			attributeCount += face->numedges;
		}
	}
	offset += (attributeCount * sizeof(XrMatrix4x4f));
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
				auto face = (msurface_t*)surface.face;
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < face->numedges - 2; j++)
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
				auto count = (face->numedges - 2) * 3;
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
				auto face = (msurface_t*)surface.face;
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < face->numedges - 2; j++)
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
				auto count = (face->numedges - 2) * 3;
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
			auto face = (msurface_t*)surface.face;
			*target++ = index++;
			*target++ = index++;
			*target++ = index++;
			auto revert = true;
			for (auto j = 1; j < face->numedges - 2; j++)
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
			auto count = (face->numedges - 2) * 3;
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
				auto face = (msurface_t*)surface.face;
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < face->numedges - 2; j++)
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
				auto count = (face->numedges - 2) * 3;
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
				auto face = (msurface_t*)surface.face;
				*target++ = index++;
				*target++ = index++;
				*target++ = index++;
				auto revert = true;
				for (auto j = 1; j < face->numedges - 2; j++)
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
				auto count = (face->numedges - 2) * 3;
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
			auto face = (msurface_t*)surface.face;
			*target++ = index++;
			*target++ = index++;
			*target++ = index++;
			auto revert = true;
			for (auto j = 1; j < face->numedges - 2; j++)
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
			auto count = (face->numedges - 2) * 3;
			indexCount += count;
			index32Count += count;
		}
		entry.indexCount = indexCount;
	}
	offset += (index32Count * sizeof(uint32_t));
}
