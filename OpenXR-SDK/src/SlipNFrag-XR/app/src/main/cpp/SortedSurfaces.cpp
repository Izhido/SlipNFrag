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
	added.clear();
}

void SortedSurfaces::Sort(LoadedSurface& loaded, int index, std::list<SortedSurfaceLightmap>& sorted)
{
	auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = added.find(lightmap);
	if (entry == added.end())
	{
		sorted.push_back({ lightmap });
		auto lightmapEntry = sorted.end();
		lightmapEntry--;
		lightmapEntry->textures.push_back({ texture });
		lightmapEntry->textures.back().entries.push_back(index);
		added.insert({ lightmap, lightmapEntry });
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

void SortedSurfaces::SortAndAccumulate(AppState& appState, LoadedSurface& loaded, int index, std::list<SortedSurfaceLightmap>& sorted)
{
	auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = added.find(lightmap);
	if (entry == added.end())
	{
		sorted.push_back({ lightmap });
		auto lightmapEntry = sorted.end();
		lightmapEntry--;
		lightmapEntry->textures.push_back({ texture });
		lightmapEntry->textures.back().entries.push_back(index);
		appState.Scene.sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		appState.Scene.sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		appState.Scene.sortedIndicesSize += ((loaded.count - 2) * 3 * sizeof(uint32_t));
		added.insert({ lightmap, lightmapEntry });
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
				appState.Scene.sortedVerticesSize += (loaded.count * 3 * sizeof(float));
				appState.Scene.sortedAttributesSize += (loaded.count * 16 * sizeof(float));
				appState.Scene.sortedIndicesSize += ((loaded.count - 2) * 3 * sizeof(uint32_t));
				break;
			}
		}
		if (!subEntryFound)
		{
			entry->second->textures.push_back({ texture });
			entry->second->textures.back().entries.push_back(index);
			appState.Scene.sortedVerticesSize += (loaded.count * 3 * sizeof(float));
			appState.Scene.sortedAttributesSize += (loaded.count * 16 * sizeof(float));
			appState.Scene.sortedIndicesSize += ((loaded.count - 2) * 3 * sizeof(uint32_t));
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

void SortedSurfaces::LoadVertices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
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
				offset += (face->numedges * 3 * sizeof(float));
			}
		}
	}
}

void SortedSurfaces::LoadAttributes(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
				auto source = (msurface_t*)surface.texturePositions.source;
				auto face = (msurface_t*)surface.indices.firstSource;
				for (auto j = 0; j < face->numedges; j++)
				{
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
					*target++ = surface.lightmap.lightmap->allocatedIndex;
					*target++ = 0;
				}
				offset += (face->numedges * 16 * sizeof(float));
			}
		}
	}
}

void SortedSurfaces::LoadIndices(std::list<SortedSurfaceLightmap>& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, uint32_t& indexBase, VkDeviceSize& offset)
{
	for (auto& entry : sorted)
	{
		for (auto& subEntry : entry.textures)
		{
			VkDeviceSize indexCount = 0;
			for (auto i : subEntry.entries)
			{
				auto& surface = loaded[i];
				auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
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
				offset += (count * sizeof(uint32_t));
				indexCount += count;
			}
			subEntry.indexCount = indexCount;
		}
	}
}
