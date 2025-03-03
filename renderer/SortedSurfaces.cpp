#include "SortedSurfaces.h"
#include "AppState.h"
#include "Constants.h"

void SortedSurfaces::Initialize(SortedSurfaceTexturesWithLightmaps& sorted)
{
    sorted.count = 0;
	sorted.added.clear();
}

void SortedSurfaces::Initialize(SortedSurfaceLightmapsWith2Textures& sorted)
{
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        lightmap.textures.clear();
    }
    sorted.count = 0;
    sorted.added.clear();
}

void SortedSurfaces::Initialize(SortedSurfaceTextures& sorted)
{
    sorted.count = 0;
    sorted.added.clear();
}

void SortedSurfaces::Sort(AppState& appState, LoadedSurface& loaded, int index, SortedSurfaceTexturesWithLightmaps& sorted)
{
	auto texture = loaded.texture.texture->descriptorSet;
	auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
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
	auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
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

void SortedSurfaces::Sort(AppState& appState, LoadedSurface2Textures& loaded, int index, SortedSurfaceLightmapsWith2Textures& sorted)
{
    auto lightmap = loaded.lightmap.lightmap->texture->descriptorSet;
    auto texture = loaded.texture.texture->descriptorSet;
    auto entry = sorted.added.find(lightmap);
    if (entry == sorted.added.end())
    {
        if (sorted.count >= sorted.lightmaps.size())
        {
            sorted.lightmaps.resize(sorted.lightmaps.size() + Constants::sortedSurfaceElementIncrement);
        }
        auto& sortedLightmap = sorted.lightmaps[sorted.count];
        sortedLightmap.lightmap = lightmap;
        if (sortedLightmap.textures.empty())
        {
            sortedLightmap.textures.resize(Constants::sortedSurfaceElementIncrement);
        }
        auto& sortedTexture = sortedLightmap.textures[0];
        sortedTexture.texture = texture;
        sortedTexture.glowTexture = loaded.glowTexture.texture->descriptorSet;
        sortedTexture.entries.clear();
        sortedTexture.entries.push_back(index);
        sortedLightmap.count = 1;
        sorted.added.insert({ lightmap, sorted.count });
        sorted.count++;
    }
    else
    {
        auto found = false;
        auto& sortedLightmap = sorted.lightmaps[entry->second];
        for (auto t = 0; t < sortedLightmap.count; t++)
        {
            auto& sortedTexture = sortedLightmap.textures[t];
            if (sortedTexture.texture == texture)
            {
                found = true;
                sortedTexture.entries.push_back(index);
                break;
            }
        }
        if (!found)
        {
            if (sortedLightmap.count >= sortedLightmap.textures.size())
            {
                sortedLightmap.textures.resize(sortedLightmap.textures.size() + Constants::sortedSurfaceElementIncrement);
            }
            auto& sortedTexture = sortedLightmap.textures[sortedLightmap.count];
            sortedTexture.texture = texture;
            sortedTexture.glowTexture = loaded.glowTexture.texture->descriptorSet;
            sortedTexture.entries.clear();
            sortedTexture.entries.push_back(index);
            sortedLightmap.count++;
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
        if (sorted.count >= sorted.lightmaps.size())
        {
            sorted.lightmaps.resize(sorted.lightmaps.size() + Constants::sortedSurfaceElementIncrement);
        }
        auto& sortedLightmap = sorted.lightmaps[sorted.count];
        sortedLightmap.lightmap = lightmap;
        if (sortedLightmap.textures.empty())
        {
            sortedLightmap.textures.resize(Constants::sortedSurfaceElementIncrement);
        }
        auto& sortedTexture = sortedLightmap.textures[0];
        sortedTexture.texture = texture;
        sortedTexture.glowTexture = loaded.glowTexture.texture->descriptorSet;
        sortedTexture.entries.clear();
        sortedTexture.entries.push_back(index);
        sortedLightmap.count = 1;
        sorted.added.insert({ lightmap, sorted.count });
        sorted.count++;
    }
    else
    {
        auto found = false;
        auto& sortedLightmap = sorted.lightmaps[entry->second];
        for (auto t = 0; t < sortedLightmap.count; t++)
        {
            auto& sortedTexture = sortedLightmap.textures[t];
            if (sortedTexture.texture == texture)
            {
                found = true;
                sortedTexture.entries.push_back(index);
                break;
            }
        }
        if (!found)
        {
            if (sortedLightmap.count >= sortedLightmap.textures.size())
            {
                sortedLightmap.textures.resize(sortedLightmap.textures.size() + Constants::sortedSurfaceElementIncrement);
            }
            auto& sortedTexture = sortedLightmap.textures[sortedLightmap.count];
            sortedTexture.texture = texture;
            sortedTexture.glowTexture = loaded.glowTexture.texture->descriptorSet;
            sortedTexture.entries.clear();
            sortedTexture.entries.push_back(index);
            sortedLightmap.count++;
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

void SortedSurfaces::LoadVertices(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurface>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
        for (auto l = 0; l < texture.count; l++)
		{
            auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
			{
                target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
                attributeIndexAsFloat += 4;
			}
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
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
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
            for (auto i : texture.entries)
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
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
            for (auto i : texture.entries)
			{
                target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
                attributeIndexAsFloat += 5;
			}
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotated>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
			{
                target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
                attributeIndexAsFloat += 6;
			}
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

void SortedSurfaces::LoadVertices(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, uint32_t& attributeIndex, Buffer* stagingBuffer, VkDeviceSize& offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
    auto attributeIndexAsFloat = (float)attributeIndex;
	for (auto t = 0; t < sorted.count; t++)
	{
		auto& texture = sorted.textures[t];
		for (auto l = 0; l < texture.count; l++)
		{
			auto& lightmap = texture.lightmaps[l];
			for (auto i : lightmap.entries)
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
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
            for (auto i : texture.entries)
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
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
            for (auto i : texture.entries)
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
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
		for (auto i : texture.entries)
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
    for (auto t = 0; t < sorted.count; t++)
    {
        auto& texture = sorted.textures[t];
		for (auto i : texture.entries)
		{
            target = CopyVertices(loaded[i], attributeIndexAsFloat, target);
            attributeIndexAsFloat += 5;
		}
	}
    attributeIndex = (uint32_t)attributeIndexAsFloat;
    offset = ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	XrMatrix4x4f attributes { };
	void* previousFace = nullptr;
	auto target = (XrMatrix4x4f*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
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
				attributeCount++;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	XrMatrix4x4f attributes { };
	void* previousFace = nullptr;
	auto target = (XrMatrix4x4f*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
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
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
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
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
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
				attributeCount++;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
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
				attributeCount++;
			}
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotatedColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
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
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
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
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
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
			attributeCount++;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadAttributes(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulentRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
	auto attributeCount = 0;
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
            *target++ = 0;
			attributeCount++;
		}
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurface>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
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
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
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
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
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
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
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

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
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
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
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
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
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
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint16_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
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
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices16(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
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
	auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
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
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
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
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
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

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
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
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurface2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
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
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTexturesWithLightmaps& sorted, std::vector<LoadedSurfaceRotated>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
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
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
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

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2Textures>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
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
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceLightmapsWith2Textures& sorted, std::vector<LoadedSurfaceRotated2TexturesColoredLights>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
	uint32_t index = 0;
    for (auto l = 0; l < sorted.count; l++)
    {
        auto& lightmap = sorted.lightmaps[l];
        for (auto t = 0; t < lightmap.count; t++)
        {
            auto& texture = lightmap.textures[t];
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
	}
    return ((unsigned char*)target) - ((unsigned char*)stagingBuffer->mapped);
}

VkDeviceSize SortedSurfaces::LoadIndices32(SortedSurfaceTextures& sorted, std::vector<LoadedTurbulent>& loaded, Buffer* stagingBuffer, VkDeviceSize offset)
{
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
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
	auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
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
