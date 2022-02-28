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
		addedTextures.insert({ texture, textureEntry });
	}
	else
	{
		entry->second->entries.push_back(index);
	}
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

void SortedSurfaces::LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto vertexCount = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.indices.firstSource;
				auto model = (model_t*)surface.indices.secondSource;
				auto edge = model->surfedges[face->firstedge];
				auto source = (float*)surface.vertices.source;
				uint32_t index;
				if (edge >= 0)
				{
					index = model->edges[edge].v[0];
				}
				else
				{
					index = model->edges[-edge].v[1];
				}
				index *= 3;
				*target++ = *(source + index++);
				*target++ = *(source + index++);
				*target++ = *(source + index++);
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
					index *= 3;
					*target++ = *(source + index++);
					*target++ = *(source + index++);
					*target++ = *(source + index++);
				}
				vertexCount += face->numedges;
			}
		}
	}
	offset += (vertexCount * 3 * sizeof(float));
}

void SortedSurfaces::LoadVertices(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto vertexCount = 0;
	for (auto& entry : sorted)
	{
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			auto face = (msurface_t*)surface.indices.firstSource;
			auto model = (model_t*)surface.indices.secondSource;
			auto edge = model->surfedges[face->firstedge];
			auto source = (float*)surface.vertices.source;
			uint32_t index;
			if (edge >= 0)
			{
				index = model->edges[edge].v[0];
			}
			else
			{
				index = model->edges[-edge].v[1];
			}
			index *= 3;
			*target++ = *(source + index++);
			*target++ = *(source + index++);
			*target++ = *(source + index++);
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
				index *= 3;
				*target++ = *(source + index++);
				*target++ = *(source + index++);
				*target++ = *(source + index++);
			}
			vertexCount += face->numedges;
		}
	}
	offset += (vertexCount * 3 * sizeof(float));
}

void SortedSurfaces::LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	XrMatrix4x4f attributes;
	void* previousSource = nullptr;
	attributes.m[15] = 0;
	auto target = (XrMatrix4x4f*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto source = (msurface_t*)surface.texturePositions.source;
				auto face = (msurface_t*)surface.indices.firstSource;
				if (previousSource != source)
				{
					attributes.m[0] = source->texinfo->vecs[0][0];
					attributes.m[1] = source->texinfo->vecs[0][1];
					attributes.m[2] = source->texinfo->vecs[0][2];
					attributes.m[3] = source->texinfo->vecs[0][3];
					attributes.m[4] = source->texinfo->vecs[1][0];
					attributes.m[5] = source->texinfo->vecs[1][1];
					attributes.m[6] = source->texinfo->vecs[1][2];
					attributes.m[7] = source->texinfo->vecs[1][3];
					attributes.m[8] = source->texturemins[0];
					attributes.m[9] = source->texturemins[1];
					attributes.m[10] = source->extents[0];
					attributes.m[11] = source->extents[1];
					attributes.m[12] = source->texinfo->texture->width;
					attributes.m[13] = source->texinfo->texture->height;
					previousSource = source;
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

void SortedSurfaces::LoadAttributes(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	XrMatrix4x4f attributes;
	void* previousSource = nullptr;
	attributes.m[10] = 0;
	attributes.m[11] = 0;
	attributes.m[12] = 0;
	attributes.m[13] = 0;
	attributes.m[14] = 0;
	attributes.m[15] = 0;
	auto target = (XrMatrix4x4f*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
	for (auto& entry : sorted)
	{
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			auto source = (msurface_t*)surface.texturePositions.source;
			auto face = (msurface_t*)surface.indices.firstSource;
			if (previousSource != source)
			{
				attributes.m[0] = source->texinfo->vecs[0][0];
				attributes.m[1] = source->texinfo->vecs[0][1];
				attributes.m[2] = source->texinfo->vecs[0][2];
				attributes.m[3] = source->texinfo->vecs[0][3];
				attributes.m[4] = source->texinfo->vecs[1][0];
				attributes.m[5] = source->texinfo->vecs[1][1];
				attributes.m[6] = source->texinfo->vecs[1][2];
				attributes.m[7] = source->texinfo->vecs[1][3];
				attributes.m[8] = source->texinfo->texture->width;
				attributes.m[9] = source->texinfo->texture->height;
				previousSource = source;
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

void SortedSurfaces::LoadIndices16(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, uint16_t& indexBase, VkDeviceSize& offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index16Count = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.indices.firstSource;
				*target++ = indexBase++;
				*target++ = indexBase++;
				*target++ = indexBase++;
				auto revert = true;
				for (auto j = 1; j < face->numedges - 2; j++)
				{
					if (revert)
					{
						*target++ = indexBase;
						*target++ = indexBase - 1;
						*target++ = indexBase - 2;
						indexBase++;
					}
					else
					{
						*target++ = indexBase - 2;
						*target++ = indexBase - 1;
						*target++ = indexBase++;
					}
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

void SortedSurfaces::LoadIndices16(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, uint16_t& indexBase, VkDeviceSize& offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index16Count = 0;
	for (auto& entry : sorted)
	{
		VkDeviceSize indexCount = 0;
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			auto face = (msurface_t*)surface.indices.firstSource;
			*target++ = indexBase++;
			*target++ = indexBase++;
			*target++ = indexBase++;
			auto revert = true;
			for (auto j = 1; j < face->numedges - 2; j++)
			{
				if (revert)
				{
					*target++ = indexBase;
					*target++ = indexBase - 1;
					*target++ = indexBase - 2;
					indexBase++;
				}
				else
				{
					*target++ = indexBase - 2;
					*target++ = indexBase - 1;
					*target++ = indexBase++;
				}
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

void SortedSurfaces::LoadIndices32(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, uint32_t& indexBase, VkDeviceSize& offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index32Count = 0;
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto face = (msurface_t*)surface.indices.firstSource;
				*target++ = indexBase++;
				*target++ = indexBase++;
				*target++ = indexBase++;
				auto revert = true;
				for (auto j = 1; j < face->numedges - 2; j++)
				{
					if (revert)
					{
						*target++ = indexBase;
						*target++ = indexBase - 1;
						*target++ = indexBase - 2;
						indexBase++;
					}
					else
					{
						*target++ = indexBase - 2;
						*target++ = indexBase - 1;
						*target++ = indexBase++;
					}
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

void SortedSurfaces::LoadIndices32(std::list<SortedSurfaceTexture>& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, uint32_t& indexBase, VkDeviceSize& offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto index32Count = 0;
	for (auto& entry : sorted)
	{
		VkDeviceSize indexCount = 0;
		for (auto i : entry.entries)
		{
			auto& surface = loaded[i];
			auto face = (msurface_t*)surface.indices.firstSource;
			*target++ = indexBase++;
			*target++ = indexBase++;
			*target++ = indexBase++;
			auto revert = true;
			for (auto j = 1; j < face->numedges - 2; j++)
			{
				if (revert)
				{
					*target++ = indexBase;
					*target++ = indexBase - 1;
					*target++ = indexBase - 2;
					indexBase++;
				}
				else
				{
					*target++ = indexBase - 2;
					*target++ = indexBase - 1;
					*target++ = indexBase++;
				}
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
