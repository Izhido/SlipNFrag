#include "SortedSurfaces.h"
#include "AppState.h"
#include "Constants.h"

void SortedSurfaces::Initialize(SortedSurfaceTexturesWithLightmaps& sorted)
{
    sorted.count = 0;
	sorted.added.clear();
}

void SortedSurfaces::Initialize(SortedSurfaceTexturePairsWithLightmaps& sorted)
{
    sorted.count = 0;
    sorted.added.clear();
}

void SortedSurfaces::Initialize(SortedSurfaceTextures& sorted)
{
    sorted.count = 0;
    sorted.added.clear();
}

void SortedSurfaces::Initialize(SortedAliasColormaps& sorted)
{
	sorted.count = 0;
	sorted.added.clear();
}

void SortedSurfaces::Initialize(SortedAliasIndices& sorted)
{
	sorted.count = 0;
	sorted.added.clear();
}

void SortedSurfaces::Sort(AppState& appState, LoadedSurface& loaded, int index, SortedSurfaceTexturesWithLightmaps& sorted)
{
	auto texture = loaded.texture.texture->descriptorSet;
	auto lightmap = loaded.lightmap.lightmap->buffer->descriptorSet;
	auto entry = sorted.added.find(texture);
	if (entry == sorted.added.end())
	{
        if (sorted.count >= sorted.textures.size())
        {
            sorted.textures.resize(sorted.textures.size() + Constants::sortedSurfaceElementIncrement);
        }
        auto& sortedTexture = sorted.textures[sorted.count];
		sortedTexture.texture = texture;
        if (sortedTexture.lightmaps.empty())
        {
			sortedTexture.lightmaps.resize(Constants::sortedSurfaceElementIncrement);
        }
        auto& sortedLightmap = sortedTexture.lightmaps[0];
		sortedLightmap.lightmap = lightmap;
		sortedLightmap.entries.clear();
		sortedLightmap.entries.push_back(index);
        sortedTexture.count = 1;
        sorted.added.insert({ texture, sorted.count });
        sorted.count++;
	}
	else
	{
		auto found = false;
        auto& sortedTexture = sorted.textures[entry->second];
		for (auto l = 0; l < sortedTexture.count; l++)
		{
            auto& sortedLightmap = sortedTexture.lightmaps[l];
			if (sortedLightmap.lightmap == lightmap)
			{
				found = true;
                sortedLightmap.entries.push_back(index);
				break;
			}
		}
		if (!found)
		{
            if (sortedTexture.count >= sortedTexture.lightmaps.size())
            {
                sortedTexture.lightmaps.resize(sortedTexture.lightmaps.size() + Constants::sortedSurfaceElementIncrement);
            }
            auto& sortedLightmap = sortedTexture.lightmaps[sortedTexture.count];
			sortedLightmap.lightmap = lightmap;
			sortedLightmap.entries.clear();
			sortedLightmap.entries.push_back(index);
            sortedTexture.count++;
		}
	}
}

void SortedSurfaces::Sort(AppState& appState, LoadedSurfaceColoredLights& loaded, int index, SortedSurfaceTexturesWithLightmaps& sorted)
{
    auto texture = loaded.texture.texture->descriptorSet;
	auto lightmap = loaded.lightmap.lightmap->buffer->descriptorSet;
    auto entry = sorted.added.find(texture);
    if (entry == sorted.added.end())
    {
        if (sorted.count >= sorted.textures.size())
        {
            sorted.textures.resize(sorted.textures.size() + Constants::sortedSurfaceElementIncrement);
        }
        auto& sortedTexture = sorted.textures[sorted.count];
		sortedTexture.texture = texture;
        if (sortedTexture.lightmaps.empty())
        {
			sortedTexture.lightmaps.resize(Constants::sortedSurfaceElementIncrement);
        }
        auto& sortedLightmap = sortedTexture.lightmaps[0];
		sortedLightmap.lightmap = lightmap;
		sortedLightmap.entries.clear();
		sortedLightmap.entries.push_back(index);
        sortedTexture.count = 1;
        sorted.added.insert({ texture, sorted.count });
        sorted.count++;
    }
    else
    {
        auto found = false;
        auto& sortedTexture = sorted.textures[entry->second];
        for (auto l = 0; l < sortedTexture.count; l++)
        {
            auto& sortedLightmap = sortedTexture.lightmaps[l];
            if (sortedLightmap.lightmap == lightmap)
            {
                found = true;
				sortedLightmap.entries.push_back(index);
                break;
            }
        }
        if (!found)
        {
            if (sortedTexture.count >= sortedTexture.lightmaps.size())
            {
				sortedTexture.lightmaps.resize(sortedTexture.lightmaps.size() + Constants::sortedSurfaceElementIncrement);
            }
            auto& sortedLightmap = sortedTexture.lightmaps[sortedTexture.count];
			sortedLightmap.lightmap = lightmap;
			sortedLightmap.entries.clear();
			sortedLightmap.entries.push_back(index);
            sortedTexture.count++;
        }
    }
}

void SortedSurfaces::Sort(AppState& appState, LoadedSurface2Textures& loaded, int index, SortedSurfaceTexturePairsWithLightmaps& sorted)
{
	auto texture = loaded.texture.texture->descriptorSet;
	auto lightmap = loaded.lightmap.lightmap->buffer->descriptorSet;
	auto entry = sorted.added.find(texture);
	if (entry == sorted.added.end())
	{
		if (sorted.count >= sorted.textures.size())
		{
			sorted.textures.resize(sorted.textures.size() + Constants::sortedSurfaceElementIncrement);
		}
		auto& sortedTexture = sorted.textures[sorted.count];
		sortedTexture.texture = texture;
		sortedTexture.glowTexture = loaded.glowTexture.texture->descriptorSet;
		if (sortedTexture.lightmaps.empty())
		{
			sortedTexture.lightmaps.resize(Constants::sortedSurfaceElementIncrement);
		}
		auto& sortedLightmap = sortedTexture.lightmaps[0];
		sortedLightmap.lightmap = lightmap;
		sortedLightmap.entries.clear();
		sortedLightmap.entries.push_back(index);
		sortedTexture.count = 1;
		sorted.added.insert({ texture, sorted.count });
		sorted.count++;
	}
	else
	{
		auto found = false;
		auto& sortedTexture = sorted.textures[entry->second];
		for (auto l = 0; l < sortedTexture.count; l++)
		{
			auto& sortedLightmap = sortedTexture.lightmaps[l];
			if (sortedLightmap.lightmap == lightmap)
			{
				found = true;
				sortedLightmap.entries.push_back(index);
				break;
			}
		}
		if (!found)
		{
			if (sortedTexture.count >= sortedTexture.lightmaps.size())
			{
				sortedTexture.lightmaps.resize(sortedTexture.lightmaps.size() + Constants::sortedSurfaceElementIncrement);
			}
			auto& sortedLightmap = sortedTexture.lightmaps[sortedTexture.count];
			sortedLightmap.lightmap = lightmap;
			sortedLightmap.entries.clear();
			sortedLightmap.entries.push_back(index);
			sortedTexture.count++;
		}
	}
}

void SortedSurfaces::Sort(AppState& appState, LoadedSurface2TexturesColoredLights& loaded, int index, SortedSurfaceTexturePairsWithLightmaps& sorted)
{
	auto texture = loaded.texture.texture->descriptorSet;
	auto lightmap = loaded.lightmap.lightmap->buffer->descriptorSet;
	auto entry = sorted.added.find(texture);
	if (entry == sorted.added.end())
	{
		if (sorted.count >= sorted.textures.size())
		{
			sorted.textures.resize(sorted.textures.size() + Constants::sortedSurfaceElementIncrement);
		}
		auto& sortedTexture = sorted.textures[sorted.count];
		sortedTexture.texture = texture;
		sortedTexture.glowTexture = loaded.glowTexture.texture->descriptorSet;
		if (sortedTexture.lightmaps.empty())
		{
			sortedTexture.lightmaps.resize(Constants::sortedSurfaceElementIncrement);
		}
		auto& sortedLightmap = sortedTexture.lightmaps[0];
		sortedLightmap.lightmap = lightmap;
		sortedLightmap.entries.clear();
		sortedLightmap.entries.push_back(index);
		sortedTexture.count = 1;
		sorted.added.insert({ texture, sorted.count });
		sorted.count++;
	}
	else
	{
		auto found = false;
		auto& sortedTexture = sorted.textures[entry->second];
		for (auto l = 0; l < sortedTexture.count; l++)
		{
			auto& sortedLightmap = sortedTexture.lightmaps[l];
			if (sortedLightmap.lightmap == lightmap)
			{
				found = true;
				sortedLightmap.entries.push_back(index);
				break;
			}
		}
		if (!found)
		{
			if (sortedTexture.count >= sortedTexture.lightmaps.size())
			{
				sortedTexture.lightmaps.resize(sortedTexture.lightmaps.size() + Constants::sortedSurfaceElementIncrement);
			}
			auto& sortedLightmap = sortedTexture.lightmaps[sortedTexture.count];
			sortedLightmap.lightmap = lightmap;
			sortedLightmap.entries.clear();
			sortedLightmap.entries.push_back(index);
			sortedTexture.count++;
		}
	}
}

void SortedSurfaces::Sort(AppState& appState, LoadedTurbulent& loaded, int index, SortedSurfaceTextures& sorted)
{
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = sorted.added.find(texture);
	if (entry == sorted.added.end())
	{
        if (sorted.count >= sorted.textures.size())
        {
            sorted.textures.resize(sorted.textures.size() + Constants::sortedSurfaceElementIncrement);
        }
        auto& sortedTexture = sorted.textures[sorted.count];
        sortedTexture.texture = texture;
        sortedTexture.entries.clear();
        sortedTexture.entries.push_back(index);
        sorted.added.insert({ texture, sorted.count });
        sorted.count++;
	}
	else
	{
        auto& sortedTexture = sorted.textures[entry->second];
        sortedTexture.entries.push_back(index);
	}
}

void SortedSurfaces::Sort(AppState& appState, LoadedSprite& loaded, int index, SortedSurfaceTextures& sorted)
{
	auto texture = loaded.texture.texture->descriptorSet;
	auto entry = sorted.added.find(texture);
	if (entry == sorted.added.end())
	{
		if (sorted.count >= sorted.textures.size())
		{
			sorted.textures.resize(sorted.textures.size() + Constants::sortedSurfaceElementIncrement);
		}
		auto& sortedTexture = sorted.textures[sorted.count];
		sortedTexture.texture = texture;
		sortedTexture.entries.clear();
		sortedTexture.entries.push_back(index);
		sorted.added.insert({ texture, sorted.count });
		sorted.count++;
	}
	else
	{
		auto& sortedTexture = sorted.textures[entry->second];
		sortedTexture.entries.push_back(index);
	}
}

void SortedSurfaces::Sort(AppState& appState, LoadedAlias& loaded, int index, SortedAliasColormaps& sorted)
{
	VkDescriptorSet colormap;
	if (loaded.colormap.texture == nullptr)
	{
		colormap = VK_NULL_HANDLE;
	}
	else
	{
		colormap = loaded.colormap.texture->descriptorSet;
	}
	auto indices = loaded.indices.indices.buffer->buffer;
	auto texture = loaded.texture.texture->descriptorSet;
	auto vertices = loaded.vertices.buffer->buffer;
	auto texCoords = loaded.texCoords.buffer->buffer;

	auto colormapEntry = sorted.added.find(colormap);
	if (colormapEntry == sorted.added.end())
	{
		if (sorted.count >= sorted.colormaps.size())
		{
			sorted.colormaps.resize(sorted.colormaps.size() + Constants::sortedSurfaceElementIncrement);
		}
		auto& sortedColormap = sorted.colormaps[sorted.count];
		sortedColormap.colormap = colormap;
		sortedColormap.count = 0;
		sortedColormap.added.clear();
		colormapEntry = sorted.added.insert({ colormap, sorted.count++ }).first;
	}
	auto& sortedColormap = sorted.colormaps[colormapEntry->second];

	auto indicesEntry = sortedColormap.added.find(indices);
	if (indicesEntry == sortedColormap.added.end())
	{
		if (sortedColormap.count >= sortedColormap.indices.size())
		{
			sortedColormap.indices.resize(sortedColormap.indices.size() + Constants::sortedSurfaceElementIncrement);
		}
		auto& sortedIndices = sortedColormap.indices[sortedColormap.count];
		sortedIndices.indices = indices;
		sortedIndices.indexType = loaded.indices.indices.indexType;
		sortedIndices.count = 0;
		sortedIndices.added.clear();
		indicesEntry = sortedColormap.added.insert({ indices, sortedColormap.count++ }).first;
	}
	auto& sortedIndices = sortedColormap.indices[indicesEntry->second];

	auto textureEntry = sortedIndices.added.find(texture);
	if (textureEntry == sortedIndices.added.end())
	{
		if (sortedIndices.count >= sortedIndices.textures.size())
		{
			sortedIndices.textures.resize(sortedIndices.textures.size() + Constants::sortedSurfaceElementIncrement);
		}
		auto& sortedTexture = sortedIndices.textures[sortedIndices.count];
		sortedTexture.texture = texture;
		sortedTexture.count = 0;
		sortedTexture.added.clear();
		textureEntry = sortedIndices.added.insert({ texture, sortedIndices.count++ }).first;
	}
	auto& sortedTexture = sortedIndices.textures[textureEntry->second];

	auto verticesEntry = sortedTexture.added.find(vertices);
	if (verticesEntry == sortedTexture.added.end())
	{
		if (sortedTexture.count >= sortedTexture.vertices.size())
		{
			sortedTexture.vertices.resize(sortedTexture.vertices.size() + Constants::sortedSurfaceElementIncrement);
		}
		auto& sortedVertices = sortedTexture.vertices[sortedTexture.count];
		sortedVertices.vertices = vertices;
		sortedVertices.texCoords = texCoords;
		sortedVertices.entries.clear();
		verticesEntry = sortedTexture.added.insert({ vertices, sortedTexture.count++ }).first;
	}
	auto& sortedVertices = sortedTexture.vertices[verticesEntry->second];

	sortedVertices.entries.push_back(index);
}

void SortedSurfaces::Sort(AppState& appState, LoadedAliasColoredLights& loaded, int index, SortedAliasIndices& sorted)
{
	auto indices = loaded.indices.indices.buffer->buffer;
	auto texture = loaded.texture.texture->descriptorSet;
	auto vertices = loaded.vertices.buffer->buffer;
	auto texCoords = loaded.texCoords.buffer->buffer;

	auto indicesEntry = sorted.added.find(indices);
	if (indicesEntry == sorted.added.end())
	{
		if (sorted.count >= sorted.indices.size())
		{
			sorted.indices.resize(sorted.indices.size() + Constants::sortedSurfaceElementIncrement);
		}
		auto& sortedIndices = sorted.indices[sorted.count];
		sortedIndices.indices = indices;
		sortedIndices.indexType = loaded.indices.indices.indexType;
		sortedIndices.count = 0;
		sortedIndices.added.clear();
		indicesEntry = sorted.added.insert({ indices, sorted.count++ }).first;
	}
	auto& sortedIndices = sorted.indices[indicesEntry->second];

	auto textureEntry = sortedIndices.added.find(texture);
	if (textureEntry == sortedIndices.added.end())
	{
		if (sortedIndices.count >= sortedIndices.textures.size())
		{
			sortedIndices.textures.resize(sortedIndices.textures.size() + Constants::sortedSurfaceElementIncrement);
		}
		auto& sortedTexture = sortedIndices.textures[sortedIndices.count];
		sortedTexture.texture = texture;
		sortedTexture.count = 0;
		sortedTexture.added.clear();
		textureEntry = sortedIndices.added.insert({ texture, sortedIndices.count++ }).first;
	}
	auto& sortedTexture = sortedIndices.textures[textureEntry->second];

	auto verticesEntry = sortedTexture.added.find(vertices);
	if (verticesEntry == sortedTexture.added.end())
	{
		if (sortedTexture.count >= sortedTexture.vertices.size())
		{
			sortedTexture.vertices.resize(sortedTexture.vertices.size() + Constants::sortedSurfaceElementIncrement);
		}
		auto& sortedVertices = sortedTexture.vertices[sortedTexture.count];
		sortedVertices.vertices = vertices;
		sortedVertices.texCoords = texCoords;
		sortedVertices.entries.clear();
		verticesEntry = sortedTexture.added.insert({ vertices, sortedTexture.count++ }).first;
	}
	auto& sortedVertices = sortedTexture.vertices[verticesEntry->second];

	sortedVertices.entries.push_back(index);
}

Vertex* SortedSurfaces::CopyVertices(LoadedTurbulent& loaded, uint32_t attributeIndex, Vertex* target)
{
    auto source = loaded.vertices;
    for (auto i = 0; i < loaded.numedges; i++)
    {
	    target->x = *source++;
        target->y = *source++;
        target->z = *source++;
        target->attributeIndex = attributeIndex;
        target++;
    }
    return target;
}

void SortedSurfaces::LoadVertices(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurface>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (Vertex*)((unsigned char*)stagingBuffer->mapped + offset);
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
        for (auto l = 0; l < texture.count; l++)
		{
            auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
			{
                target = CopyVertices(loaded[i], attributeIndex, target);
                attributeIndex += 4;
			}
		}
	}
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (Vertex*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
			{
                target = CopyVertices(loaded[i], attributeIndex, target);
                attributeIndex += 4;
			}
		}
	}
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2Textures>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (Vertex*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
			{
				target = CopyVertices(loaded[i], attributeIndex, target);
				attributeIndex += 5;
			}
		}
	}
	offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (Vertex*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
			{
				target = CopyVertices(loaded[i], attributeIndex, target);
				attributeIndex += 5;
			}
		}
	}
	offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotated>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (Vertex*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
			{
                target = CopyVertices(loaded[i], attributeIndex, target);
                attributeIndex += 7;
			}
		}
	}
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (Vertex*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
			{
                target = CopyVertices(loaded[i], attributeIndex, target);
                attributeIndex += 7;
			}
		}
	}
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (Vertex*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
			{
				target = CopyVertices(loaded[i], attributeIndex, target);
				attributeIndex += 7;
			}
		}
	}
	offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (Vertex*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
			{
				target = CopyVertices(loaded[i], attributeIndex, target);
				attributeIndex += 7;
			}
		}
	}
	offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (Vertex*)((unsigned char*)stagingBuffer->mapped + offset);
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
		for (auto i : texture.entries)
		{
            target = CopyVertices(loaded[i], attributeIndex, target);
            attributeIndex += 3;
		}
	}
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (Vertex*)((unsigned char*)stagingBuffer->mapped + offset);
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
		for (auto i : texture.entries)
		{
            target = CopyVertices(loaded[i], attributeIndex, target);
            attributeIndex += 5;
		}
	}
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTextures& sorted, std::vector<LoadedSprite>& loaded, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto i : texture.entries)
		{
			auto vertices = d_lists.textured_vertices.data() + loaded[i].firstVertex * 3;
			auto attributes = d_lists.textured_attributes.data() + loaded[i].firstVertex * 2;
			for (auto v = 0; v < loaded[i].count; v++)
			{
				*target++ = *vertices++;
				*target++ = *vertices++;
				*target++ = *vertices++;
				*target++ = *attributes++;
				*target++ = *attributes++;
			}
		}
	}
	offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	XrMatrix4x4f attributes { };
	void* previousFace = nullptr;
	auto target = (XrMatrix4x4f*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
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
				attributes.m[14] = surface.lightmap.lightmap->offset;
				attributes.m[15] = surface.texture.index;
                *target++ = attributes;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	XrMatrix4x4f attributes { };
	void* previousFace = nullptr;
	auto target = (XrMatrix4x4f*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
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
				attributes.m[14] = surface.lightmap.lightmap->offset;
				attributes.m[15] = surface.texture.index;
                *target++ = attributes;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
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
                *target++ = surface.lightmap.lightmap->offset;
                *target++ = surface.texture.index;
                *target++ = surface.glowTexture.index;
                *target++ = 0;
                *target++ = 0;
                *target++ = 0;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
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
                *target++ = surface.lightmap.lightmap->offset;
                *target++ = surface.texture.index;
                *target++ = surface.glowTexture.index;
                *target++ = 0;
                *target++ = 0;
                *target++ = 0;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
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
                *target++ = surface.lightmap.lightmap->offset;
                *target++ = surface.texture.index;
				*target++ = surface.alpha;
				*target++ = 0;
				*target++ = 0;
				*target++ = 0;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
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
                *target++ = surface.lightmap.lightmap->offset;
                *target++ = surface.texture.index;
				*target++ = surface.alpha;
				*target++ = 0;
				*target++ = 0;
				*target++ = 0;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
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
                *target++ = surface.lightmap.lightmap->offset;
                *target++ = surface.texture.index;
				*target++ = surface.alpha;
				*target++ = 0;
				*target++ = 0;
				*target++ = 0;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)((unsigned char*)stagingBuffer->mapped + offset);
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
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
                *target++ = surface.lightmap.lightmap->offset;
                *target++ = surface.texture.index;
				*target++ = surface.alpha;
				*target++ = 0;
				*target++ = 0;
				*target++ = 0;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)((unsigned char*)stagingBuffer->mapped + offset);
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
		for (auto i : texture.entries)
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
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)((unsigned char*)stagingBuffer->mapped + offset);
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
		for (auto i : texture.entries)
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
            *target++ = surface.alpha;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint16_t index = 0;
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
        for (auto l = 0; l < texture.count; l++)
        {
            auto& lightmap = texture.lightmaps[l];
            VkDeviceSize indexCount = 0;
            for (auto i : lightmap.entries)
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
			lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint16_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
            lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint16_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
            lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint16_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
            lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint16_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
            VkDeviceSize indexCount = 0;
            for (auto i : lightmap.entries)
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
			lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint16_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
			lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint16_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
			lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint16_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
			lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint16_t index = 0;
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
		VkDeviceSize indexCount = 0;
		for (auto i : texture.entries)
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
		texture.indexCount = indexCount;
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint16_t index = 0;
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
		VkDeviceSize indexCount = 0;
		for (auto i : texture.entries)
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
		texture.indexCount = indexCount;
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTextures& sorted, std::vector<LoadedSprite>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint16_t index = 0;
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
		VkDeviceSize indexCount = 0;
		for (auto i : texture.entries)
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
		texture.indexCount = indexCount;
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint32_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
            lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint32_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
            lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint32_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
			lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint32_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
			lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint32_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
            lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint32_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
            lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint32_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
			lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTexturePairsWithLightmaps& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint32_t index = 0;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			VkDeviceSize indexCount = 0;
			for (auto i : lightmap.entries)
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
			lightmap.indexCount = indexCount;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint32_t index = 0;
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
		VkDeviceSize indexCount = 0;
		for (auto i : texture.entries)
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
		texture.indexCount = indexCount;
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint32_t index = 0;
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
		VkDeviceSize indexCount = 0;
		for (auto i : texture.entries)
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
		texture.indexCount = indexCount;
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTextures& sorted, std::vector<LoadedSprite>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)((unsigned char*)stagingBuffer->mapped + offset);
	uint32_t index = 0;
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
		VkDeviceSize indexCount = 0;
		for (auto i : texture.entries)
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
		texture.indexCount = indexCount;
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}
