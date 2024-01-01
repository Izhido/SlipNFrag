#include "AppState.h"
#include "Scene.h" // This header is specified second in the list to allow headers in AppState.h to include the core engine structs
#include "Utils.h"
#include "Constants.h"

void Scene::Initialize()
{
    sortedVerticesSize = 0;
    sortedAttributesSize = 0;
    sortedIndicesCount = 0;
    paletteSize = 0;
    colormapSize = 0;
    controllerVerticesSize = 0;
    buffers.Initialize();
    indexBuffers.Initialize();
    lightmaps.first = nullptr;
    lightmaps.current = nullptr;
    lightmapsRGBA.first = nullptr;
    lightmapsRGBA.current = nullptr;
    for (auto& cached : surfaceTextures)
    {
        cached.first = nullptr;
        cached.current = nullptr;
    }
    for (auto& cached : surfaceRGBATextures)
    {
        cached.first = nullptr;
        cached.current = nullptr;
    }
    textures.first = nullptr;
    textures.current = nullptr;
}

void Scene::AddSampler(AppState& appState, uint32_t mipCount)
{
    if (samplers.size() <= mipCount)
    {
        samplers.resize(mipCount + 1);
    }
    if (samplers[mipCount] == VK_NULL_HANDLE)
    {
        VkSamplerCreateInfo samplerCreateInfo { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerCreateInfo.maxLod = mipCount - 1;
        CHECK_VKCMD(vkCreateSampler(appState.Device, &samplerCreateInfo, nullptr, &samplers[mipCount]));
    }
}

void Scene::AddToBufferBarrier(VkBuffer buffer)
{
    stagingBuffer.lastEndBarrier++;
    if (stagingBuffer.bufferBarriers.size() <= stagingBuffer.lastEndBarrier)
    {
        stagingBuffer.bufferBarriers.emplace_back();
    }

    auto& barrier = stagingBuffer.bufferBarriers[stagingBuffer.lastEndBarrier];
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.buffer = buffer;
    barrier.size = VK_WHOLE_SIZE;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
}

VkDeviceSize Scene::GetAllocatedFor(int width, int height)
{
    VkDeviceSize allocated = 0;
    while (width > 1 || height > 1)
    {
        allocated += (width * height);
        width /= 2;
        if (width < 1)
        {
            width = 1;
        }
        height /= 2;
        if (height < 1)
        {
            height = 1;
        }
    }
    return allocated;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedLightmap& loaded, VkDeviceSize& size)
{
    auto lightmapEntry = lightmaps.lightmaps.find(surface.face);
    if (lightmapEntry == lightmaps.lightmaps.end())
    {
        auto lightmap = new Lightmap { };
        lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R16_UINT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        lightmap->createdFrameCount = surface.created;
        loaded.lightmap = lightmap;
        loaded.size = surface.lightmap_size * sizeof(uint16_t);
        size += loaded.size;
        loaded.source = d_lists.lightmap_texels.data() + surface.lightmap_texels;
        lightmaps.Setup(loaded);
        lightmaps.lightmaps.insert({ surface.face, lightmap });
    }
    else
    {
        auto lightmap = lightmapEntry->second;
        if (lightmap->createdFrameCount != surface.created)
        {
            lightmap->next = lightmaps.oldLightmaps;
            lightmaps.oldLightmaps = lightmap;
            lightmap = new Lightmap { };
            lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R16_UINT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            lightmap->createdFrameCount = surface.created;
            lightmapEntry->second = lightmap;
            loaded.lightmap = lightmap;
            loaded.size = surface.lightmap_size * sizeof(uint16_t);
            size += loaded.size;
            loaded.source = d_lists.lightmap_texels.data() + surface.lightmap_texels;
            lightmaps.Setup(loaded);
        }
        else
        {
            loaded.lightmap = lightmap;
        }
    }
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedLightmapRGBA& loaded, VkDeviceSize& size)
{
    auto lightmapEntry = lightmapsRGBA.lightmaps.find(surface.face);
    if (lightmapEntry == lightmapsRGBA.lightmaps.end())
    {
        auto lightmap = new LightmapRGBA { };
        lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R16G16B16A16_UINT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        lightmap->createdFrameCount = surface.created;
        loaded.lightmap = lightmap;
        loaded.size = surface.lightmap_size * sizeof(uint16_t);
        size += loaded.size;
        loaded.source = d_lists.lightmap_texels.data() + surface.lightmap_texels;
        lightmapsRGBA.Setup(loaded);
        lightmapsRGBA.lightmaps.insert({ surface.face, lightmap });
    }
    else
    {
        auto lightmap = lightmapEntry->second;
        if (lightmap->createdFrameCount != surface.created)
        {
            lightmap->next = lightmapsRGBA.oldLightmaps;
            lightmapsRGBA.oldLightmaps = lightmap;
            lightmap = new LightmapRGBA { };
            lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R16G16B16A16_UINT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            lightmap->createdFrameCount = surface.created;
            lightmapEntry->second = lightmap;
            loaded.lightmap = lightmap;
            loaded.size = surface.lightmap_size * sizeof(uint16_t);
            size += loaded.size;
            loaded.source = d_lists.lightmap_texels.data() + surface.lightmap_texels;
            lightmapsRGBA.Setup(loaded);
        }
        else
        {
            loaded.lightmap = lightmap;
        }
    }
}

void Scene::GetStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size)
{
    loaded.vertexes = ((model_t*)turbulent.model)->vertexes;
    loaded.face = turbulent.face;
    loaded.model = turbulent.model;
    if (previousTexture != turbulent.data)
    {
        auto entry = surfaceTextureCache.find(turbulent.data);
        if (entry == surfaceTextureCache.end())
        {
            CachedSharedMemoryTextures* cached = nullptr;
            for (auto& entry : surfaceTextures)
            {
                if (entry.width == turbulent.width && entry.height == turbulent.height)
                {
                    cached = &entry;
                    break;
                }
            }
            if (cached == nullptr)
            {
                surfaceTextures.push_back({ turbulent.width, turbulent.height });
                cached = &surfaceTextures.back();
            }
            if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount)
            {
                uint32_t layerCount;
                if (cached->textures == nullptr)
                {
                    layerCount = 4;
                }
                else
                {
                    layerCount = 64;
                }
                auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
                auto texture = new SharedMemoryTexture { };
                texture->Create(appState, turbulent.width, turbulent.height, VK_FORMAT_R8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                cached->MoveToFront(texture);
                loaded.texture.texture = texture;
                cached->currentIndex = 0;
            }
            else
            {
                loaded.texture.texture = cached->textures;
            }
            cached->Setup(loaded.texture);
            loaded.texture.index = cached->currentIndex;
            loaded.texture.size = turbulent.size;
            loaded.texture.allocated = GetAllocatedFor(turbulent.width, turbulent.height);
            size += loaded.texture.allocated;
            loaded.texture.source = turbulent.data;
            loaded.texture.mips = turbulent.mips;
            surfaceTextureCache.insert({ turbulent.data, { loaded.texture.texture, loaded.texture.index } });
            cached->currentIndex++;
        }
        else
        {
            loaded.texture.texture = entry->second.texture;
            loaded.texture.index = entry->second.index;
        }
        previousTexture = turbulent.data;
        previousSharedMemoryTexture = loaded.texture.texture;
        previousSharedMemoryTextureIndex = loaded.texture.index;
    }
    else
    {
        loaded.texture.texture = previousSharedMemoryTexture;
        loaded.texture.index = previousSharedMemoryTextureIndex;
    }
    loaded.count = turbulent.count;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size)
{
    loaded.vertexes = ((model_t*)turbulent.model)->vertexes;
    loaded.face = turbulent.face;
    loaded.model = turbulent.model;
    if (previousTexture != turbulent.data)
    {
        auto entry = surfaceTextureCache.find(turbulent.data);
        if (entry == surfaceTextureCache.end())
        {
            CachedSharedMemoryTextures* cached = nullptr;
            for (auto& entry : surfaceRGBATextures)
            {
                if (entry.width == turbulent.width && entry.height == turbulent.height)
                {
                    cached = &entry;
                    break;
                }
            }
            if (cached == nullptr)
            {
                surfaceRGBATextures.push_back({ turbulent.width, turbulent.height });
                cached = &surfaceRGBATextures.back();
            }
            if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount)
            {
                uint32_t layerCount;
                if (cached->textures == nullptr)
                {
                    layerCount = 4;
                }
                else
                {
                    layerCount = 64;
                }
                auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
                auto texture = new SharedMemoryTexture { };
                texture->Create(appState, turbulent.width, turbulent.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                cached->MoveToFront(texture);
                loaded.texture.texture = texture;
                cached->currentIndex = 0;
            }
            else
            {
                loaded.texture.texture = cached->textures;
            }
            cached->Setup(loaded.texture);
            loaded.texture.index = cached->currentIndex;
            loaded.texture.size = turbulent.size;
            loaded.texture.allocated = GetAllocatedFor(turbulent.width, turbulent.height) * sizeof(unsigned);
            size += loaded.texture.allocated;
            loaded.texture.source = turbulent.data;
            loaded.texture.mips = turbulent.mips;
            surfaceTextureCache.insert({ turbulent.data, { loaded.texture.texture, loaded.texture.index } });
            cached->currentIndex++;
        }
        else
        {
            loaded.texture.texture = entry->second.texture;
            loaded.texture.index = entry->second.index;
        }
        previousTexture = turbulent.data;
        previousSharedMemoryTexture = loaded.texture.texture;
        previousSharedMemoryTextureIndex = loaded.texture.index;
    }
    else
    {
        loaded.texture.texture = previousSharedMemoryTexture;
        loaded.texture.index = previousSharedMemoryTextureIndex;
    }
    loaded.count = turbulent.count;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dturbulent_t&)surface, loaded, size);
    GetStagingBufferSize(appState, surface, loaded.lightmap, size);
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurfaceColoredLights& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dturbulent_t&)surface, loaded, size);
    GetStagingBufferSize(appState, surface, loaded.lightmap, size);
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacewithglow_t& surface, LoadedSurface2Textures& loaded, VkDeviceSize& size)
{
    loaded.vertexes = ((model_t*)surface.model)->vertexes;
    loaded.face = surface.face;
    loaded.model = surface.model;
    if (previousTexture != surface.data)
    {
        auto entry = surfaceTextureCache.find(surface.data);
        if (entry == surfaceTextureCache.end())
        {
            CachedSharedMemoryTextures* cached = nullptr;
            for (auto& entry : surfaceRGBATextures)
            {
                if (entry.width == surface.width && entry.height == surface.height)
                {
                    cached = &entry;
                    break;
                }
            }
            if (cached == nullptr)
            {
                surfaceRGBATextures.push_back({ surface.width, surface.height });
                cached = &surfaceRGBATextures.back();
            }
            // By doing this, we guarantee that the next texture allocations (color and glow) will be done
            // in the same texture array, which will allow the sorted surfaces algorithm to work as expected:
            auto extra = (surface.glow_data != nullptr ? 1 : 0);
            if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount - extra)
            {
                uint32_t layerCount;
                if (cached->textures == nullptr)
                {
                    layerCount = 4;
                }
                else
                {
                    layerCount = 64;
                }
                auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
                auto texture = new SharedMemoryTexture { };
                texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                cached->MoveToFront(texture);
                loaded.texture.texture = texture;
                cached->currentIndex = 0;
            }
            else
            {
                loaded.texture.texture = cached->textures;
            }
            cached->Setup(loaded.texture);
            loaded.texture.index = cached->currentIndex;
            loaded.texture.size = surface.size;
            loaded.texture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
            size += loaded.texture.allocated;
            loaded.texture.source = surface.data;
            loaded.texture.mips = surface.mips;
            surfaceTextureCache.insert({ surface.data, { loaded.texture.texture, loaded.texture.index } });
            cached->currentIndex++;
        }
        else
        {
            loaded.texture.texture = entry->second.texture;
            loaded.texture.index = entry->second.index;
        }
        previousTexture = surface.data;
        previousSharedMemoryTexture = loaded.texture.texture;
        previousSharedMemoryTextureIndex = loaded.texture.index;
    }
    else
    {
        loaded.texture.texture = previousSharedMemoryTexture;
        loaded.texture.index = previousSharedMemoryTextureIndex;
    }
    if (previousGlowTexture != surface.glow_data)
    {
        auto entry = surfaceTextureCache.find(surface.glow_data);
        if (entry == surfaceTextureCache.end())
        {
            CachedSharedMemoryTextures* cached = nullptr;
            for (auto& entry : surfaceRGBATextures)
            {
                if (entry.width == surface.width && entry.height == surface.height)
                {
                    cached = &entry;
                    break;
                }
            }
            if (cached == nullptr)
            {
                surfaceRGBATextures.push_back({ surface.width, surface.height });
                cached = &surfaceRGBATextures.back();
            }
            if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount)
            {
                uint32_t layerCount;
                if (cached->textures == nullptr)
                {
                    layerCount = 4;
                }
                else
                {
                    layerCount = 64;
                }
                auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
                auto texture = new SharedMemoryTexture { };
                texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                cached->MoveToFront(texture);
                loaded.glowTexture.texture = texture;
                cached->currentIndex = 0;
            }
            else
            {
                loaded.glowTexture.texture = cached->textures;
            }
            cached->Setup(loaded.glowTexture);
            loaded.glowTexture.index = cached->currentIndex;
            loaded.glowTexture.size = surface.size;
            loaded.glowTexture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
            size += loaded.glowTexture.allocated;
            loaded.glowTexture.source = surface.glow_data;
            loaded.glowTexture.mips = surface.mips;
            surfaceTextureCache.insert({ surface.glow_data, { loaded.glowTexture.texture, loaded.glowTexture.index } });
            cached->currentIndex++;
        }
        else
        {
            loaded.glowTexture.texture = entry->second.texture;
            loaded.glowTexture.index = entry->second.index;
        }
        previousGlowTexture = surface.glow_data;
        previousGlowSharedMemoryTexture = loaded.glowTexture.texture;
        previousGlowSharedMemoryTextureIndex = loaded.glowTexture.index;
    }
    else
    {
        loaded.glowTexture.texture = previousGlowSharedMemoryTexture;
        loaded.glowTexture.index = previousGlowSharedMemoryTextureIndex;
    }
    GetStagingBufferSize(appState, surface, loaded.lightmap, size);
    loaded.count = surface.count;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacewithglow_t& surface, LoadedSurface2TexturesColoredLights& loaded, VkDeviceSize& size)
{
    loaded.vertexes = ((model_t*)surface.model)->vertexes;
    loaded.face = surface.face;
    loaded.model = surface.model;
    if (previousTexture != surface.data)
    {
        auto entry = surfaceTextureCache.find(surface.data);
        if (entry == surfaceTextureCache.end())
        {
            CachedSharedMemoryTextures* cached = nullptr;
            for (auto& entry : surfaceRGBATextures)
            {
                if (entry.width == surface.width && entry.height == surface.height)
                {
                    cached = &entry;
                    break;
                }
            }
            if (cached == nullptr)
            {
                surfaceRGBATextures.push_back({ surface.width, surface.height });
                cached = &surfaceRGBATextures.back();
            }
            // By doing this, we guarantee that the next texture allocations (color and glow) will be done
            // in the same texture array, which will allow the sorted surfaces algorithm to work as expected:
            auto extra = (surface.glow_data != nullptr ? 1 : 0);
            if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount - extra)
            {
                uint32_t layerCount;
                if (cached->textures == nullptr)
                {
                    layerCount = 4;
                }
                else
                {
                    layerCount = 64;
                }
                auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
                auto texture = new SharedMemoryTexture { };
                texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                cached->MoveToFront(texture);
                loaded.texture.texture = texture;
                cached->currentIndex = 0;
            }
            else
            {
                loaded.texture.texture = cached->textures;
            }
            cached->Setup(loaded.texture);
            loaded.texture.index = cached->currentIndex;
            loaded.texture.size = surface.size;
            loaded.texture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
            size += loaded.texture.allocated;
            loaded.texture.source = surface.data;
            loaded.texture.mips = surface.mips;
            surfaceTextureCache.insert({ surface.data, { loaded.texture.texture, loaded.texture.index } });
            cached->currentIndex++;
        }
        else
        {
            loaded.texture.texture = entry->second.texture;
            loaded.texture.index = entry->second.index;
        }
        previousTexture = surface.data;
        previousSharedMemoryTexture = loaded.texture.texture;
        previousSharedMemoryTextureIndex = loaded.texture.index;
    }
    else
    {
        loaded.texture.texture = previousSharedMemoryTexture;
        loaded.texture.index = previousSharedMemoryTextureIndex;
    }
    if (previousGlowTexture != surface.glow_data)
    {
        auto entry = surfaceTextureCache.find(surface.glow_data);
        if (entry == surfaceTextureCache.end())
        {
            CachedSharedMemoryTextures* cached = nullptr;
            for (auto& entry : surfaceRGBATextures)
            {
                if (entry.width == surface.width && entry.height == surface.height)
                {
                    cached = &entry;
                    break;
                }
            }
            if (cached == nullptr)
            {
                surfaceRGBATextures.push_back({ surface.width, surface.height });
                cached = &surfaceRGBATextures.back();
            }
            if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount)
            {
                uint32_t layerCount;
                if (cached->textures == nullptr)
                {
                    layerCount = 4;
                }
                else
                {
                    layerCount = 64;
                }
                auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
                auto texture = new SharedMemoryTexture { };
                texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                cached->MoveToFront(texture);
                loaded.glowTexture.texture = texture;
                cached->currentIndex = 0;
            }
            else
            {
                loaded.glowTexture.texture = cached->textures;
            }
            cached->Setup(loaded.glowTexture);
            loaded.glowTexture.index = cached->currentIndex;
            loaded.glowTexture.size = surface.size;
            loaded.glowTexture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
            size += loaded.glowTexture.allocated;
            loaded.glowTexture.source = surface.glow_data;
            loaded.glowTexture.mips = surface.mips;
            surfaceTextureCache.insert({ surface.glow_data, { loaded.glowTexture.texture, loaded.glowTexture.index } });
            cached->currentIndex++;
        }
        else
        {
            loaded.glowTexture.texture = entry->second.texture;
            loaded.glowTexture.index = entry->second.index;
        }
        previousGlowTexture = surface.glow_data;
        previousGlowSharedMemoryTexture = loaded.glowTexture.texture;
        previousGlowSharedMemoryTextureIndex = loaded.glowTexture.index;
    }
    else
    {
        loaded.glowTexture.texture = previousGlowSharedMemoryTexture;
        loaded.glowTexture.index = previousGlowSharedMemoryTextureIndex;
    }
    GetStagingBufferSize(appState, surface, loaded.lightmap, size);
    loaded.count = surface.count;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size)
{
    loaded.vertexes = ((model_t*)surface.model)->vertexes;
    loaded.face = surface.face;
    loaded.model = surface.model;
    if (previousTexture != surface.data)
    {
        auto entry = surfaceTextureCache.find(surface.data);
        if (entry == surfaceTextureCache.end())
        {
            CachedSharedMemoryTextures* cached = nullptr;
            for (auto& entry : surfaceRGBATextures)
            {
                if (entry.width == surface.width && entry.height == surface.height)
                {
                    cached = &entry;
                    break;
                }
            }
            if (cached == nullptr)
            {
                surfaceRGBATextures.push_back({ surface.width, surface.height });
                cached = &surfaceRGBATextures.back();
            }
            if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount)
            {
                uint32_t layerCount;
                if (cached->textures == nullptr)
                {
                    layerCount = 4;
                }
                else
                {
                    layerCount = 64;
                }
                auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
                auto texture = new SharedMemoryTexture { };
                texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                cached->MoveToFront(texture);
                loaded.texture.texture = texture;
                cached->currentIndex = 0;
            }
            else
            {
                loaded.texture.texture = cached->textures;
            }
            cached->Setup(loaded.texture);
            loaded.texture.index = cached->currentIndex;
            loaded.texture.size = surface.size;
            loaded.texture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
            size += loaded.texture.allocated;
            loaded.texture.source = surface.data;
            loaded.texture.mips = surface.mips;
            surfaceTextureCache.insert({ surface.data, { loaded.texture.texture, loaded.texture.index } });
            cached->currentIndex++;
        }
        else
        {
            loaded.texture.texture = entry->second.texture;
            loaded.texture.index = entry->second.index;
        }
        previousTexture = surface.data;
        previousSharedMemoryTexture = loaded.texture.texture;
        previousSharedMemoryTextureIndex = loaded.texture.index;
    }
    else
    {
        loaded.texture.texture = previousSharedMemoryTexture;
        loaded.texture.index = previousSharedMemoryTextureIndex;
    }
    GetStagingBufferSize(appState, surface, loaded.lightmap, size);
    loaded.count = surface.count;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurface_t& surface, LoadedSurfaceColoredLights& loaded, VkDeviceSize& size)
{
    loaded.vertexes = ((model_t*)surface.model)->vertexes;
    loaded.face = surface.face;
    loaded.model = surface.model;
    if (previousTexture != surface.data)
    {
        auto entry = surfaceTextureCache.find(surface.data);
        if (entry == surfaceTextureCache.end())
        {
            CachedSharedMemoryTextures* cached = nullptr;
            for (auto& entry : surfaceRGBATextures)
            {
                if (entry.width == surface.width && entry.height == surface.height)
                {
                    cached = &entry;
                    break;
                }
            }
            if (cached == nullptr)
            {
                surfaceRGBATextures.push_back({ surface.width, surface.height });
                cached = &surfaceRGBATextures.back();
            }
            if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount)
            {
                uint32_t layerCount;
                if (cached->textures == nullptr)
                {
                    layerCount = 4;
                }
                else
                {
                    layerCount = 64;
                }
                auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
                auto texture = new SharedMemoryTexture { };
                texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                cached->MoveToFront(texture);
                loaded.texture.texture = texture;
                cached->currentIndex = 0;
            }
            else
            {
                loaded.texture.texture = cached->textures;
            }
            cached->Setup(loaded.texture);
            loaded.texture.index = cached->currentIndex;
            loaded.texture.size = surface.size;
            loaded.texture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
            size += loaded.texture.allocated;
            loaded.texture.source = surface.data;
            loaded.texture.mips = surface.mips;
            surfaceTextureCache.insert({ surface.data, { loaded.texture.texture, loaded.texture.index } });
            cached->currentIndex++;
        }
        else
        {
            loaded.texture.texture = entry->second.texture;
            loaded.texture.index = entry->second.index;
        }
        previousTexture = surface.data;
        previousSharedMemoryTexture = loaded.texture.texture;
        previousSharedMemoryTextureIndex = loaded.texture.index;
    }
    else
    {
        loaded.texture.texture = previousSharedMemoryTexture;
        loaded.texture.index = previousSharedMemoryTextureIndex;
    }
    GetStagingBufferSize(appState, surface, loaded.lightmap, size);
    loaded.count = surface.count;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dsurface_t&)surface, loaded, size);
    loaded.originX = surface.origin_x;
    loaded.originY = surface.origin_y;
    loaded.originZ = surface.origin_z;
    loaded.yaw = surface.yaw;
    loaded.pitch = surface.pitch;
    loaded.roll = surface.roll;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotatedColoredLights& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dsurface_t&)surface, (LoadedSurfaceColoredLights&)loaded, size);
    loaded.originX = surface.origin_x;
    loaded.originY = surface.origin_y;
    loaded.originZ = surface.origin_z;
    loaded.yaw = surface.yaw;
    loaded.pitch = surface.pitch;
    loaded.roll = surface.roll;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacerotatedwithglow_t& surface, LoadedSurfaceRotated2Textures& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dsurfacewithglow_t&)surface, loaded, size);
    loaded.originX = surface.origin_x;
    loaded.originY = surface.origin_y;
    loaded.originZ = surface.origin_z;
    loaded.yaw = surface.yaw;
    loaded.pitch = surface.pitch;
    loaded.roll = surface.roll;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacerotatedwithglow_t& surface, LoadedSurfaceRotated2TexturesColoredLights& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dsurfacewithglow_t&)surface, loaded, size);
    loaded.originX = surface.origin_x;
    loaded.originY = surface.origin_y;
    loaded.originZ = surface.origin_z;
    loaded.yaw = surface.yaw;
    loaded.pitch = surface.pitch;
    loaded.roll = surface.roll;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size)
{
    GetStagingBufferSizeRGBANoGlow(appState, (const dsurface_t&)surface, loaded, size);
    loaded.originX = surface.origin_x;
    loaded.originY = surface.origin_y;
    loaded.originZ = surface.origin_z;
    loaded.yaw = surface.yaw;
    loaded.pitch = surface.pitch;
    loaded.roll = surface.roll;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotatedColoredLights& loaded, VkDeviceSize& size)
{
    GetStagingBufferSizeRGBANoGlow(appState, (const dsurface_t&)surface, loaded, size);
    loaded.originX = surface.origin_x;
    loaded.originY = surface.origin_y;
    loaded.originZ = surface.origin_z;
    loaded.yaw = surface.yaw;
    loaded.pitch = surface.pitch;
    loaded.roll = surface.roll;
}

void Scene::GetStagingBufferSize(AppState& appState, const dturbulentrotated_t& turbulent, LoadedTurbulentRotated& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dturbulent_t&)turbulent, loaded, size);
    loaded.originX = turbulent.origin_x;
    loaded.originY = turbulent.origin_y;
    loaded.originZ = turbulent.origin_z;
    loaded.yaw = turbulent.yaw;
    loaded.pitch = turbulent.pitch;
    loaded.roll = turbulent.roll;
}

void Scene::GetStagingBufferSize(AppState& appState, const dspritedata_t& sprite, LoadedSprite& loaded, VkDeviceSize& size)
{
    if (previousTexture != sprite.data)
    {
        auto entry = spriteCache.find(sprite.data);
        if (entry == spriteCache.end())
        {
            auto mipCount = (int)(std::floor(std::log2(std::max(sprite.width, sprite.height)))) + 1;
            auto texture = new SharedMemoryTexture { };
            texture->Create(appState, sprite.width, sprite.height, VK_FORMAT_R8_UINT, mipCount, 1, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            textures.MoveToFront(texture);
            loaded.texture.size = sprite.size;
            size += loaded.texture.size;
            loaded.texture.texture = texture;
            loaded.texture.source = sprite.data;
            loaded.texture.mips = 1;
            textures.Setup(loaded.texture);
            spriteCache.insert({ sprite.data, texture });
        }
        else
        {
            loaded.texture.texture = entry->second;
            loaded.texture.index = 0;
        }
        previousTexture = sprite.data;
        previousSharedMemoryTexture = loaded.texture.texture;
    }
    else
    {
        loaded.texture.texture = previousSharedMemoryTexture;
        loaded.texture.index = 0;
    }
    loaded.count = sprite.count;
    loaded.firstVertex = sprite.first_vertex;
}

void Scene::GetStagingBufferSize(AppState& appState, const dalias_t& alias, LoadedAlias& loaded, Texture* host_colormap, VkDeviceSize& size)
{
    if (previousApverts != alias.apverts)
    {
        auto entry = aliasVertexCache.find(alias.apverts);
        if (entry == aliasVertexCache.end())
        {
            auto vertexSize = 4 * sizeof(byte);
            loaded.vertices.size = alias.vertex_count * 2 * vertexSize;
            loaded.vertices.buffer = new SharedMemoryBuffer { };
            loaded.vertices.buffer->CreateVertexBuffer(appState, loaded.vertices.size);
            buffers.MoveToFront(loaded.vertices.buffer);
            size += loaded.vertices.size;
            loaded.vertices.source = alias.apverts;
            buffers.SetupAliasVertices(loaded.vertices);
            loaded.texCoords.size = alias.vertex_count * 2 * 2 * sizeof(float);
            loaded.texCoords.buffer = new SharedMemoryBuffer { };
            loaded.texCoords.buffer->CreateVertexBuffer(appState, loaded.texCoords.size);
            buffers.MoveToFront(loaded.texCoords.buffer);
            size += loaded.texCoords.size;
            loaded.texCoords.source = alias.texture_coordinates;
            loaded.texCoords.width = alias.width;
            loaded.texCoords.height = alias.height;
            buffers.SetupAliasTexCoords(loaded.texCoords);
            aliasVertexCache.insert({ alias.apverts, { loaded.vertices.buffer, loaded.texCoords.buffer } });
        }
        else
        {
            loaded.vertices.buffer = entry->second.vertices;
            loaded.texCoords.buffer = entry->second.texCoords;
        }
        previousApverts = alias.apverts;
        previousVertexBuffer = loaded.vertices.buffer;
        previousTexCoordsBuffer = loaded.texCoords.buffer;
    }
    else
    {
        loaded.vertices.buffer = previousVertexBuffer;
        loaded.texCoords.buffer = previousTexCoordsBuffer;
    }
    if (alias.colormap == nullptr)
    {
        loaded.colormap.size = 0;
        loaded.colormap.texture = host_colormap;
    }
    else
    {
        loaded.colormap.size = 16384;
        size += loaded.colormap.size;
    }
    if (previousTexture != alias.data)
    {
        auto entry = aliasTextureCache.find(alias.data);
        if (entry == aliasTextureCache.end())
        {
            auto mipCount = (int)(std::floor(std::log2(std::max(alias.width, alias.height)))) + 1;
            auto texture = new SharedMemoryTexture { };
            texture->Create(appState, alias.width, alias.height, VK_FORMAT_R8_UINT, mipCount, 1, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            textures.MoveToFront(texture);
            loaded.texture.size = alias.size;
            size += loaded.texture.size;
            loaded.texture.texture = texture;
            loaded.texture.source = alias.data;
            loaded.texture.mips = 1;
            textures.Setup(loaded.texture);
            aliasTextureCache.insert({ alias.data, texture });
        }
        else
        {
            loaded.texture.texture = entry->second;
            loaded.texture.index = 0;
        }
        previousTexture = alias.data;
        previousSharedMemoryTexture = loaded.texture.texture;
    }
    else
    {
        loaded.texture.texture = previousSharedMemoryTexture;
        loaded.texture.index = 0;
    }
    auto entry = aliasIndexCache.find(alias.aliashdr);
    if (entry == aliasIndexCache.end())
    {
        auto aliashdr = (aliashdr_t *)alias.aliashdr;
        auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
        auto triangle = (mtriangle_t *)((byte *)aliashdr + aliashdr->triangles);
        auto stverts = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);
        unsigned int maxIndex = 0;
        for (auto i = 0; i < mdl->numtris; i++)
        {
            auto v0 = triangle->vertindex[0];
            auto v1 = triangle->vertindex[1];
            auto v2 = triangle->vertindex[2];
            auto v0back = (((stverts[v0].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
            auto v1back = (((stverts[v1].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
            auto v2back = (((stverts[v2].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
            maxIndex = std::max(maxIndex, (unsigned int)(v0 * 2 + (v0back ? 1 : 0)));
            maxIndex = std::max(maxIndex, (unsigned int)(v1 * 2 + (v1back ? 1 : 0)));
            maxIndex = std::max(maxIndex, (unsigned int)(v2 * 2 + (v2back ? 1 : 0)));
            triangle++;
        }
        if (maxIndex < UPPER_8BIT_LIMIT && appState.IndexTypeUInt8Enabled)
        {
            loaded.indices.size = alias.count;
            if (latestIndexBuffer8 == nullptr || usedInLatestIndexBuffer8 + loaded.indices.size > latestIndexBuffer8->size)
            {
                loaded.indices.indices.buffer = new SharedMemoryBuffer { };
                loaded.indices.indices.buffer->CreateIndexBuffer(appState, Constants::indexBuffer8BitSize);
                indexBuffers.MoveToFront(loaded.indices.indices.buffer);
                latestIndexBuffer8 = loaded.indices.indices.buffer;
                usedInLatestIndexBuffer8 = 0;
            }
            else
            {
                loaded.indices.indices.buffer = latestIndexBuffer8;
            }
            loaded.indices.indices.offset = usedInLatestIndexBuffer8;
            usedInLatestIndexBuffer8 += loaded.indices.size;
            size += loaded.indices.size;
            loaded.indices.source = alias.aliashdr;
            loaded.indices.indices.indexType = VK_INDEX_TYPE_UINT8_EXT;
            loaded.indices.indices.firstIndex = loaded.indices.indices.offset;
            indexBuffers.SetupAliasIndices8(loaded.indices);
        }
        else if (maxIndex < UPPER_16BIT_LIMIT)
        {
            loaded.indices.size = alias.count * sizeof(uint16_t);
            if (latestIndexBuffer16 == nullptr || usedInLatestIndexBuffer16 + loaded.indices.size > latestIndexBuffer16->size)
            {
                loaded.indices.indices.buffer = new SharedMemoryBuffer { };
                loaded.indices.indices.buffer->CreateIndexBuffer(appState, Constants::indexBuffer16BitSize);
                indexBuffers.MoveToFront(loaded.indices.indices.buffer);
                latestIndexBuffer16 = loaded.indices.indices.buffer;
                usedInLatestIndexBuffer16 = 0;
            }
            else
            {
                loaded.indices.indices.buffer = latestIndexBuffer16;
            }
            loaded.indices.indices.offset = usedInLatestIndexBuffer16;
            usedInLatestIndexBuffer16 += loaded.indices.size;
            size += loaded.indices.size;
            loaded.indices.source = alias.aliashdr;
            loaded.indices.indices.indexType = VK_INDEX_TYPE_UINT16;
            loaded.indices.indices.firstIndex = loaded.indices.indices.offset / 2;
            indexBuffers.SetupAliasIndices16(loaded.indices);
        }
        else
        {
            loaded.indices.size = alias.count * sizeof(uint32_t);
            if (latestIndexBuffer32 == nullptr || usedInLatestIndexBuffer32 + loaded.indices.size > latestIndexBuffer32->size)
            {
                loaded.indices.indices.buffer = new SharedMemoryBuffer { };
                loaded.indices.indices.buffer->CreateIndexBuffer(appState, Constants::indexBuffer32BitSize);
                indexBuffers.MoveToFront(loaded.indices.indices.buffer);
                latestIndexBuffer32 = loaded.indices.indices.buffer;
                usedInLatestIndexBuffer32 = 0;
            }
            else
            {
                loaded.indices.indices.buffer = latestIndexBuffer32;
            }
            loaded.indices.indices.offset = usedInLatestIndexBuffer32;
            usedInLatestIndexBuffer32 += loaded.indices.size;
            size += loaded.indices.size;
            loaded.indices.source = alias.aliashdr;
            loaded.indices.indices.indexType = VK_INDEX_TYPE_UINT32;
            loaded.indices.indices.firstIndex = loaded.indices.indices.offset / 4;
            indexBuffers.SetupAliasIndices32(loaded.indices);
        }
        aliasIndexCache.insert({ alias.aliashdr, loaded.indices.indices });
    }
    else
    {
        loaded.indices.indices = entry->second;
    }
    loaded.firstAttribute = alias.first_attribute;
    loaded.isHostColormap = (alias.colormap == nullptr);
    loaded.count = alias.count;
    for (auto j = 0; j < 3; j++)
    {
        for (auto i = 0; i < 4; i++)
        {
            loaded.transform[j][i] = alias.transform[j][i];
        }
    }
}

VkDeviceSize Scene::GetStagingBufferSize(AppState& appState, PerFrame& perFrame)
{
    surfaces.Allocate(d_lists.last_surface);
    surfacesColoredLights.Allocate(d_lists.last_surface_colored_lights);
    surfacesRGBA.Allocate(d_lists.last_surface_rgba);
    surfacesRGBAColoredLights.Allocate(d_lists.last_surface_rgba_colored_lights);
    surfacesRGBANoGlow.Allocate(d_lists.last_surface_rgba_no_glow);
    surfacesRGBANoGlowColoredLights.Allocate(d_lists.last_surface_rgba_no_glow_colored_lights);
    surfacesRotated.Allocate(d_lists.last_surface_rotated);
    surfacesRotatedColoredLights.Allocate(d_lists.last_surface_rotated_colored_lights);
    surfacesRotatedRGBA.Allocate(d_lists.last_surface_rotated_rgba);
    surfacesRotatedRGBAColoredLights.Allocate(d_lists.last_surface_rotated_rgba_colored_lights);
    surfacesRotatedRGBANoGlow.Allocate(d_lists.last_surface_rotated_rgba_no_glow);
    surfacesRotatedRGBANoGlowColoredLights.Allocate(d_lists.last_surface_rotated_rgba_no_glow_colored_lights);
    fences.Allocate(d_lists.last_fence);
    fencesColoredLights.Allocate(d_lists.last_fence_colored_lights);
    fencesRGBA.Allocate(d_lists.last_fence_rgba);
    fencesRGBAColoredLights.Allocate(d_lists.last_fence_rgba_colored_lights);
    fencesRGBANoGlow.Allocate(d_lists.last_fence_rgba_no_glow);
    fencesRGBANoGlowColoredLights.Allocate(d_lists.last_fence_rgba_no_glow_colored_lights);
    fencesRotated.Allocate(d_lists.last_fence_rotated);
    fencesRotatedColoredLights.Allocate(d_lists.last_fence_rotated_colored_lights);
    fencesRotatedRGBA.Allocate(d_lists.last_fence_rotated_rgba);
    fencesRotatedRGBAColoredLights.Allocate(d_lists.last_fence_rotated_rgba_colored_lights);
    fencesRotatedRGBANoGlow.Allocate(d_lists.last_fence_rotated_rgba_no_glow);
    fencesRotatedRGBANoGlowColoredLights.Allocate(d_lists.last_fence_rotated_rgba_no_glow_colored_lights);
    turbulent.Allocate(d_lists.last_turbulent);
    turbulentRGBA.Allocate(d_lists.last_turbulent_rgba);
    turbulentLit.Allocate(d_lists.last_turbulent_lit);
    turbulentColoredLights.Allocate(d_lists.last_turbulent_colored_lights);
    turbulentRGBALit.Allocate(d_lists.last_turbulent_rgba_lit);
    turbulentRGBAColoredLights.Allocate(d_lists.last_turbulent_rgba_colored_lights);
    turbulentRotated.Allocate(d_lists.last_turbulent_rotated);
    turbulentRotatedRGBA.Allocate(d_lists.last_turbulent_rotated_rgba);
    turbulentRotatedLit.Allocate(d_lists.last_turbulent_rotated_lit);
    turbulentRotatedColoredLights.Allocate(d_lists.last_turbulent_rotated_colored_lights);
    turbulentRotatedRGBALit.Allocate(d_lists.last_turbulent_rotated_rgba_lit);
    turbulentRotatedRGBAColoredLights.Allocate(d_lists.last_turbulent_rotated_rgba_colored_lights);
    sprites.Allocate(d_lists.last_sprite);
    alias.Allocate(d_lists.last_alias);
    viewmodels.Allocate(d_lists.last_viewmodel);
    lastParticle = d_lists.last_particle_color;
    lastColoredIndex8 = d_lists.last_colored_index8;
    lastColoredIndex16 = d_lists.last_colored_index16;
    lastColoredIndex32 = d_lists.last_colored_index32;
    lastSky = d_lists.last_sky;
    lastSkyRGBA = d_lists.last_sky_rgba;
    appState.FromEngine.vieworg0 = d_lists.vieworg0;
    appState.FromEngine.vieworg1 = d_lists.vieworg1;
    appState.FromEngine.vieworg2 = d_lists.vieworg2;
    appState.FromEngine.vright0 = d_lists.vright0;
    appState.FromEngine.vright1 = d_lists.vright1;
    appState.FromEngine.vright2 = d_lists.vright2;
    appState.FromEngine.vup0 = d_lists.vup0;
    appState.FromEngine.vup1 = d_lists.vup1;
    appState.FromEngine.vup2 = d_lists.vup2;
    VkDeviceSize size = 0;
    if (palette == nullptr || paletteChangedFrame != pal_changed)
    {
        SharedMemoryBuffer* newPalette = nullptr;
        bool skip = true;
        for (auto p = &palette; *p != nullptr; )
        {
            if (skip)
            {
                p = &(*p)->next;
                skip = false;
            }
            else
            {
                (*p)->unusedCount++;
                if ((*p)->unusedCount >= Constants::maxUnusedCount)
                {
                    auto next = (*p)->next;
                    newPalette = (*p);
                    newPalette->unusedCount = 0;
                    (*p) = next;
                    break;
                }
                else
                {
                    p = &(*p)->next;
                }
            }
        }
        if (newPalette == nullptr)
        {
            newPalette = new SharedMemoryBuffer { };
            newPalette->CreateUniformBuffer(appState, 256 * 4 * sizeof(float));
        }
        newPalette->next = palette;
        palette = newPalette;
        paletteSize = palette->size;
        size += paletteSize;
        newPalette = nullptr;
        skip = true;
        for (auto p = &neutralPalette; *p != nullptr; )
        {
            if (skip)
            {
                p = &(*p)->next;
                skip = false;
            }
            else
            {
                (*p)->unusedCount++;
                if ((*p)->unusedCount >= Constants::maxUnusedCount)
                {
                    auto next = (*p)->next;
                    newPalette = (*p);
                    newPalette->unusedCount = 0;
                    (*p) = next;
                    break;
                }
                else
                {
                    p = &(*p)->next;
                }
            }
        }
        if (newPalette == nullptr)
        {
            newPalette = new SharedMemoryBuffer { };
            newPalette->CreateUniformBuffer(appState, 256 * 4 * sizeof(float));
        }
        newPalette->next = neutralPalette;
        neutralPalette = newPalette;
        size += paletteSize;
        paletteChangedFrame = pal_changed;
    }
    if (!host_colormap.empty() && perFrame.colormap == nullptr)
    {
        perFrame.colormap = new Texture();
        perFrame.colormap->Create(appState, 256, 64, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        colormapSize = 16384;
        size += colormapSize;
    }
    surfaces.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(surfaces.sorted);
    for (auto i = 0; i <= surfaces.last; i++)
    {
        auto& loaded = surfaces.loaded[i];
        GetStagingBufferSize(appState, d_lists.surfaces[i], loaded, size);
        sorted.Sort(appState, loaded, i, surfaces.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(surfaces.sorted);
    surfacesColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(surfacesColoredLights.sorted);
    for (auto i = 0; i <= surfacesColoredLights.last; i++)
    {
        auto& loaded = surfacesColoredLights.loaded[i];
        GetStagingBufferSize(appState, d_lists.surfaces_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, surfacesColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(surfacesColoredLights.sorted);
    surfacesRGBA.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    previousGlowTexture = nullptr;
    sorted.Initialize(surfacesRGBA.sorted);
    for (auto i = 0; i <= surfacesRGBA.last; i++)
    {
        auto& loaded = surfacesRGBA.loaded[i];
        GetStagingBufferSize(appState, d_lists.surfaces_rgba[i], loaded, size);
        sorted.Sort(appState, loaded, i, surfacesRGBA.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 20 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(surfacesRGBA.sorted);
    surfacesRGBAColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    previousGlowTexture = nullptr;
    sorted.Initialize(surfacesRGBAColoredLights.sorted);
    for (auto i = 0; i <= surfacesRGBAColoredLights.last; i++)
    {
        auto& loaded = surfacesRGBAColoredLights.loaded[i];
        GetStagingBufferSize(appState, d_lists.surfaces_rgba_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, surfacesRGBAColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 20 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(surfacesRGBAColoredLights.sorted);
    surfacesRGBANoGlow.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(surfacesRGBANoGlow.sorted);
    for (auto i = 0; i <= surfacesRGBANoGlow.last; i++)
    {
        auto& loaded = surfacesRGBANoGlow.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.surfaces_rgba_no_glow[i], loaded, size);
        sorted.Sort(appState, loaded, i, surfacesRGBANoGlow.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(surfacesRGBANoGlow.sorted);
    surfacesRGBANoGlowColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(surfacesRGBANoGlowColoredLights.sorted);
    for (auto i = 0; i <= surfacesRGBANoGlowColoredLights.last; i++)
    {
        auto& loaded = surfacesRGBANoGlowColoredLights.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.surfaces_rgba_no_glow_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, surfacesRGBANoGlowColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(surfacesRGBANoGlowColoredLights.sorted);
    surfacesRotated.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(surfacesRotated.sorted);
    for (auto i = 0; i <= surfacesRotated.last; i++)
    {
        auto& loaded = surfacesRotated.loaded[i];
        GetStagingBufferSize(appState, d_lists.surfaces_rotated[i], loaded, size);
        sorted.Sort(appState, loaded, i, surfacesRotated.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 24 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(surfacesRotated.sorted);
    surfacesRotatedColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(surfacesRotatedColoredLights.sorted);
    for (auto i = 0; i <= surfacesRotatedColoredLights.last; i++)
    {
        auto& loaded = surfacesRotatedColoredLights.loaded[i];
        GetStagingBufferSize(appState, d_lists.surfaces_rotated_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, surfacesRotatedColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 24 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(surfacesRotatedColoredLights.sorted);
    surfacesRotatedRGBA.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    previousGlowTexture = nullptr;
    sorted.Initialize(surfacesRotatedRGBA.sorted);
    for (auto i = 0; i <= surfacesRotatedRGBA.last; i++)
    {
        auto& loaded = surfacesRotatedRGBA.loaded[i];
        GetStagingBufferSize(appState, d_lists.surfaces_rotated_rgba[i], loaded, size);
        sorted.Sort(appState, loaded, i, surfacesRotatedRGBA.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 24 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(surfacesRotatedRGBA.sorted);
    surfacesRotatedRGBAColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    previousGlowTexture = nullptr;
    sorted.Initialize(surfacesRotatedRGBAColoredLights.sorted);
    for (auto i = 0; i <= surfacesRotatedRGBAColoredLights.last; i++)
    {
        auto& loaded = surfacesRotatedRGBAColoredLights.loaded[i];
        GetStagingBufferSize(appState, d_lists.surfaces_rotated_rgba_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, surfacesRotatedRGBAColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 24 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(surfacesRotatedRGBAColoredLights.sorted);
    surfacesRotatedRGBANoGlow.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(surfacesRotatedRGBANoGlow.sorted);
    for (auto i = 0; i <= surfacesRotatedRGBANoGlow.last; i++)
    {
        auto& loaded = surfacesRotatedRGBANoGlow.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.surfaces_rotated_rgba_no_glow[i], loaded, size);
        sorted.Sort(appState, loaded, i, surfacesRotatedRGBANoGlow.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 24 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(surfacesRotatedRGBANoGlow.sorted);
    surfacesRotatedRGBANoGlowColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(surfacesRotatedRGBANoGlowColoredLights.sorted);
    for (auto i = 0; i <= surfacesRotatedRGBANoGlowColoredLights.last; i++)
    {
        auto& loaded = surfacesRotatedRGBANoGlowColoredLights.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.surfaces_rotated_rgba_no_glow_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, surfacesRotatedRGBANoGlowColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 24 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(surfacesRotatedRGBANoGlowColoredLights.sorted);
    fences.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(fences.sorted);
    for (auto i = 0; i <= fences.last; i++)
    {
        auto& loaded = fences.loaded[i];
        GetStagingBufferSize(appState, d_lists.fences[i], loaded, size);
        sorted.Sort(appState, loaded, i, fences.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(fences.sorted);
    fencesColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(fencesColoredLights.sorted);
    for (auto i = 0; i <= fencesColoredLights.last; i++)
    {
        auto& loaded = fencesColoredLights.loaded[i];
        GetStagingBufferSize(appState, d_lists.fences_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, fencesColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(fencesColoredLights.sorted);
    fencesRGBA.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    previousGlowTexture = nullptr;
    sorted.Initialize(fencesRGBA.sorted);
    for (auto i = 0; i <= fencesRGBA.last; i++)
    {
        auto& loaded = fencesRGBA.loaded[i];
        GetStagingBufferSize(appState, d_lists.fences_rgba[i], loaded, size);
        sorted.Sort(appState, loaded, i, fencesRGBA.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 20 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(fencesRGBA.sorted);
    fencesRGBAColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    previousGlowTexture = nullptr;
    sorted.Initialize(fencesRGBAColoredLights.sorted);
    for (auto i = 0; i <= fencesRGBAColoredLights.last; i++)
    {
        auto& loaded = fencesRGBAColoredLights.loaded[i];
        GetStagingBufferSize(appState, d_lists.fences_rgba_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, fencesRGBAColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 20 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(fencesRGBAColoredLights.sorted);
    fencesRGBANoGlow.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(fencesRGBANoGlow.sorted);
    for (auto i = 0; i <= fencesRGBANoGlow.last; i++)
    {
        auto& loaded = fencesRGBANoGlow.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.fences_rgba_no_glow[i], loaded, size);
        sorted.Sort(appState, loaded, i, fencesRGBANoGlow.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(fencesRGBANoGlow.sorted);
    fencesRGBANoGlowColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(fencesRGBANoGlowColoredLights.sorted);
    for (auto i = 0; i <= fencesRGBANoGlowColoredLights.last; i++)
    {
        auto& loaded = fencesRGBANoGlowColoredLights.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.fences_rgba_no_glow_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, fencesRGBANoGlowColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(fencesRGBANoGlowColoredLights.sorted);
    fencesRotated.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(fencesRotated.sorted);
    for (auto i = 0; i <= fencesRotated.last; i++)
    {
        auto& loaded = fencesRotated.loaded[i];
        GetStagingBufferSize(appState, d_lists.fences_rotated[i], loaded, size);
        sorted.Sort(appState, loaded, i, fencesRotated.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 24 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(fencesRotated.sorted);
    fencesRotatedColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(fencesRotatedColoredLights.sorted);
    for (auto i = 0; i <= fencesRotatedColoredLights.last; i++)
    {
        auto& loaded = fencesRotatedColoredLights.loaded[i];
        GetStagingBufferSize(appState, d_lists.fences_rotated_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, fencesRotatedColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 24 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(fencesRotatedColoredLights.sorted);
    fencesRotatedRGBA.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    previousGlowTexture = nullptr;
    sorted.Initialize(fencesRotatedRGBA.sorted);
    for (auto i = 0; i <= fencesRotatedRGBA.last; i++)
    {
        auto& loaded = fencesRotatedRGBA.loaded[i];
        GetStagingBufferSize(appState, d_lists.fences_rotated_rgba[i], loaded, size);
        sorted.Sort(appState, loaded, i, fencesRotatedRGBA.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 24 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(fencesRotatedRGBA.sorted);
    fencesRotatedRGBAColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    previousGlowTexture = nullptr;
    sorted.Initialize(fencesRotatedRGBAColoredLights.sorted);
    for (auto i = 0; i <= fencesRotatedRGBAColoredLights.last; i++)
    {
        auto& loaded = fencesRotatedRGBAColoredLights.loaded[i];
        GetStagingBufferSize(appState, d_lists.fences_rotated_rgba_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, fencesRotatedRGBAColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 24 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(fencesRotatedRGBAColoredLights.sorted);
    fencesRotatedRGBANoGlow.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(fencesRotatedRGBANoGlow.sorted);
    for (auto i = 0; i <= fencesRotatedRGBANoGlow.last; i++)
    {
        auto& loaded = fencesRotatedRGBANoGlow.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.fences_rotated_rgba_no_glow[i], loaded, size);
        sorted.Sort(appState, loaded, i, fencesRotatedRGBANoGlow.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 24 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(fencesRotatedRGBANoGlow.sorted);
    fencesRotatedRGBANoGlowColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(fencesRotatedRGBANoGlowColoredLights.sorted);
    for (auto i = 0; i <= fencesRotatedRGBANoGlowColoredLights.last; i++)
    {
        auto& loaded = fencesRotatedRGBANoGlowColoredLights.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.fences_rotated_rgba_no_glow_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, fencesRotatedRGBANoGlowColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 24 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(fencesRotatedRGBANoGlowColoredLights.sorted);
    turbulent.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(turbulent.sorted);
    for (auto i = 0; i <= turbulent.last; i++)
    {
        auto& loaded = turbulent.loaded[i];
        GetStagingBufferSize(appState, d_lists.turbulent[i], loaded, size);
        sorted.Sort(appState, loaded, i, turbulent.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 12 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(turbulent.sorted);
    turbulentRGBA.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(turbulentRGBA.sorted);
    for (auto i = 0; i <= turbulentRGBA.last; i++)
    {
        auto& loaded = turbulentRGBA.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rgba[i], loaded, size);
        sorted.Sort(appState, loaded, i, turbulentRGBA.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 12 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(turbulentRGBA.sorted);
    turbulentLit.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(turbulentLit.sorted);
    for (auto i = 0; i <= turbulentLit.last; i++)
    {
        auto& loaded = turbulentLit.loaded[i];
        GetStagingBufferSize(appState, d_lists.turbulent_lit[i], loaded, size);
        sorted.Sort(appState, loaded, i, turbulentLit.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(turbulentLit.sorted);
    turbulentColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(turbulentColoredLights.sorted);
    for (auto i = 0; i <= turbulentColoredLights.last; i++)
    {
        auto& loaded = turbulentColoredLights.loaded[i];
        GetStagingBufferSize(appState, d_lists.turbulent_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, turbulentColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(turbulentColoredLights.sorted);
    turbulentRGBALit.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(turbulentRGBALit.sorted);
    for (auto i = 0; i <= turbulentRGBALit.last; i++)
    {
        auto& loaded = turbulentRGBALit.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rgba_lit[i], loaded, size);
        sorted.Sort(appState, loaded, i, turbulentRGBALit.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(turbulentRGBALit.sorted);
    turbulentRGBAColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(turbulentRGBAColoredLights.sorted);
    for (auto i = 0; i <= turbulentRGBAColoredLights.last; i++)
    {
        auto& loaded = turbulentRGBAColoredLights.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rgba_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, turbulentRGBAColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(turbulentRGBAColoredLights.sorted);
    turbulentRotated.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(turbulentRotated.sorted);
    for (auto i = 0; i <= turbulentRotated.last; i++)
    {
        auto& loaded = turbulentRotated.loaded[i];
        GetStagingBufferSize(appState, d_lists.turbulent_rotated[i], loaded, size);
        sorted.Sort(appState, loaded, i, turbulentRotated.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 20 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(turbulentRotated.sorted);
    turbulentRotatedRGBA.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(turbulentRotatedRGBA.sorted);
    for (auto i = 0; i <= turbulentRotatedRGBA.last; i++)
    {
        auto& loaded = turbulentRotatedRGBA.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rotated_rgba[i], loaded, size);
        sorted.Sort(appState, loaded, i, turbulentRotatedRGBA.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 12 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(turbulentRotatedRGBA.sorted);
    turbulentRotatedLit.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(turbulentRotatedLit.sorted);
    for (auto i = 0; i <= turbulentRotatedLit.last; i++)
    {
        auto& loaded = turbulentRotatedLit.loaded[i];
        GetStagingBufferSize(appState, d_lists.turbulent_rotated_lit[i], loaded, size);
        sorted.Sort(appState, loaded, i, turbulentRotatedLit.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(turbulentRotatedLit.sorted);
    turbulentRotatedColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(turbulentRotatedColoredLights.sorted);
    for (auto i = 0; i <= turbulentRotatedColoredLights.last; i++)
    {
        auto& loaded = turbulentRotatedColoredLights.loaded[i];
        GetStagingBufferSize(appState, d_lists.turbulent_rotated_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, turbulentRotatedColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(turbulentRotatedColoredLights.sorted);
    turbulentRotatedRGBALit.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(turbulentRotatedRGBALit.sorted);
    for (auto i = 0; i <= turbulentRotatedRGBALit.last; i++)
    {
        auto& loaded = turbulentRotatedRGBALit.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rotated_rgba_lit[i], loaded, size);
        sorted.Sort(appState, loaded, i, turbulentRotatedRGBALit.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(turbulentRotatedRGBALit.sorted);
    turbulentRotatedRGBAColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
    previousTexture = nullptr;
    sorted.Initialize(turbulentRotatedRGBAColoredLights.sorted);
    for (auto i = 0; i <= turbulentRotatedRGBAColoredLights.last; i++)
    {
        auto& loaded = turbulentRotatedRGBAColoredLights.loaded[i];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rotated_rgba_colored_lights[i], loaded, size);
        sorted.Sort(appState, loaded, i, turbulentRotatedRGBAColoredLights.sorted);
        sortedVerticesSize += (loaded.count * 3 * sizeof(float));
        sortedAttributesSize += (loaded.count * 16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    SortedSurfaces::Cleanup(turbulentRotatedRGBAColoredLights.sorted);
    previousTexture = nullptr;
    for (auto i = 0; i <= sprites.last; i++)
    {
        GetStagingBufferSize(appState, d_lists.sprites[i], sprites.loaded[i], size);
    }
    previousApverts = nullptr;
    previousTexture = nullptr;
    for (auto i = 0; i <= alias.last; i++)
    {
        GetStagingBufferSize(appState, d_lists.alias[i], alias.loaded[i], perFrame.colormap, size);
    }
    previousApverts = nullptr;
    previousTexture = nullptr;
    for (auto i = 0; i <= viewmodels.last; i++)
    {
        GetStagingBufferSize(appState, d_lists.viewmodels[i], viewmodels.loaded[i], perFrame.colormap, size);
    }
    if (lastSky >= 0)
    {
        loadedSky.width = d_lists.sky[0].width;
        loadedSky.height = d_lists.sky[0].height;
        loadedSky.size = d_lists.sky[0].size;
        loadedSky.data = d_lists.sky[0].data;
        loadedSky.firstVertex = d_lists.sky[0].first_vertex;
        loadedSky.count = d_lists.sky[0].count;
        if (perFrame.sky == nullptr)
        {
            perFrame.sky = new Texture();
            perFrame.sky->Create(appState, loadedSky.width, loadedSky.height, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        }
        size += loadedSky.size;
    }
    if (lastSkyRGBA >= 0)
    {
        loadedSkyRGBA.width = d_lists.sky_rgba[0].width;
        loadedSkyRGBA.height = d_lists.sky_rgba[0].height;
        loadedSkyRGBA.size = d_lists.sky_rgba[0].size;
        loadedSkyRGBA.data = d_lists.sky_rgba[0].data;
        loadedSkyRGBA.firstVertex = d_lists.sky_rgba[0].first_vertex;
        loadedSkyRGBA.count = d_lists.sky_rgba[0].count;
        if (perFrame.skyRGBA == nullptr)
        {
            perFrame.skyRGBA = new Texture();
            perFrame.skyRGBA->Create(appState, loadedSkyRGBA.width * 2, loadedSkyRGBA.height, VK_FORMAT_R8G8B8A8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        }
        size += loadedSkyRGBA.size;
    }
    floorVerticesSize = 0;
    if (appState.Mode != AppWorldMode)
    {
        floorVerticesSize += 3 * 4 * sizeof(float);
    }
    if (appState.Focused && (key_dest == key_console || key_dest == key_menu || appState.Mode != AppWorldMode))
    {
        if (appState.LeftController.PoseIsValid)
        {
            controllerVerticesSize += 2 * 8 * 3 * sizeof(float);
        }
        if (appState.RightController.PoseIsValid)
        {
            controllerVerticesSize += 2 * 8 * 3 * sizeof(float);
        }
    }
    texturedVerticesSize = (d_lists.last_textured_vertex + 1) * sizeof(float);
    particlePositionsSize = (d_lists.last_particle_position + 1) * sizeof(float);
    coloredVerticesSize = (d_lists.last_colored_vertex + 1) * sizeof(float);
    verticesSize = floorVerticesSize + controllerVerticesSize + texturedVerticesSize + particlePositionsSize + coloredVerticesSize;
    if (verticesSize + sortedVerticesSize > 0)
    {
        perFrame.vertices = perFrame.cachedVertices.GetVertexBuffer(appState, verticesSize + sortedVerticesSize);
    }
    size += verticesSize;
    size += sortedVerticesSize;
    floorAttributesSize = 0;
    if (appState.Mode != AppWorldMode)
    {
        floorAttributesSize += 2 * 4 * sizeof(float);
    }
    controllerAttributesSize = 0;
    if (controllerVerticesSize > 0)
    {
        if (appState.LeftController.PoseIsValid)
        {
            controllerAttributesSize += 2 * 8 * 2 * sizeof(float);
        }
        if (appState.RightController.PoseIsValid)
        {
            controllerAttributesSize += 2 * 8 * 2 * sizeof(float);
        }
    }
    texturedAttributesSize = (d_lists.last_textured_attribute + 1) * sizeof(float);
    colormappedLightsSize = (d_lists.last_colormapped_attribute + 1) * sizeof(float);
    attributesSize = floorAttributesSize + controllerAttributesSize + texturedAttributesSize + colormappedLightsSize;
    if (attributesSize + sortedAttributesSize> 0)
    {
        perFrame.attributes = perFrame.cachedAttributes.GetVertexBuffer(appState, attributesSize + sortedAttributesSize);
    }
    size += attributesSize;
    size += sortedAttributesSize;
    particleColorsSize = (d_lists.last_particle_color + 1) * sizeof(float);
    coloredColorsSize = (d_lists.last_colored_color + 1) * sizeof(float);
    colorsSize = particleColorsSize + coloredColorsSize;
    if (colorsSize > 0)
    {
        perFrame.colors = perFrame.cachedColors.GetVertexBuffer(appState, colorsSize);
    }
    size += colorsSize;
    floorIndicesSize = 0;
    if (appState.Mode != AppWorldMode)
    {
        floorIndicesSize = 4;
    }
    controllerIndicesSize = 0;
    if (controllerVerticesSize > 0)
    {
        if (appState.LeftController.PoseIsValid)
        {
            controllerIndicesSize += 2 * 36;
        }
        if (appState.RightController.PoseIsValid)
        {
            controllerIndicesSize += 2 * 36;
        }
    }
    coloredIndices8Size = lastColoredIndex8 + 1;
    coloredIndices16Size = (lastColoredIndex16 + 1) * sizeof(uint16_t);

    // This is only intended as an approximation - real 16-bit index limits are not even close to being reached before this happens:
    if (sortedIndicesCount < UPPER_16BIT_LIMIT)
    {
        sortedIndices16Size = sortedIndicesCount * sizeof(uint16_t);
        sortedIndices32Size = 0;
        surfaces.ScaleIndexBase(sizeof(uint16_t));
        surfacesColoredLights.ScaleIndexBase(sizeof(uint16_t));
        surfacesRGBA.ScaleIndexBase(sizeof(uint16_t));
        surfacesRGBAColoredLights.ScaleIndexBase(sizeof(uint16_t));
        surfacesRGBANoGlow.ScaleIndexBase(sizeof(uint16_t));
        surfacesRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint16_t));
        surfacesRotated.ScaleIndexBase(sizeof(uint16_t));
        surfacesRotatedColoredLights.ScaleIndexBase(sizeof(uint16_t));
        surfacesRotatedRGBA.ScaleIndexBase(sizeof(uint16_t));
        surfacesRotatedRGBAColoredLights.ScaleIndexBase(sizeof(uint16_t));
        surfacesRotatedRGBANoGlow.ScaleIndexBase(sizeof(uint16_t));
        surfacesRotatedRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint16_t));
        fences.ScaleIndexBase(sizeof(uint16_t));
        fencesColoredLights.ScaleIndexBase(sizeof(uint16_t));
        fencesRGBA.ScaleIndexBase(sizeof(uint16_t));
        fencesRGBAColoredLights.ScaleIndexBase(sizeof(uint16_t));
        fencesRGBANoGlow.ScaleIndexBase(sizeof(uint16_t));
        fencesRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint16_t));
        fencesRotated.ScaleIndexBase(sizeof(uint16_t));
        fencesRotatedColoredLights.ScaleIndexBase(sizeof(uint16_t));
        fencesRotatedRGBA.ScaleIndexBase(sizeof(uint16_t));
        fencesRotatedRGBAColoredLights.ScaleIndexBase(sizeof(uint16_t));
        fencesRotatedRGBANoGlow.ScaleIndexBase(sizeof(uint16_t));
        fencesRotatedRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint16_t));
        turbulent.ScaleIndexBase(sizeof(uint16_t));
        turbulentRGBA.ScaleIndexBase(sizeof(uint16_t));
        turbulentLit.ScaleIndexBase(sizeof(uint16_t));
        turbulentColoredLights.ScaleIndexBase(sizeof(uint16_t));
        turbulentRGBALit.ScaleIndexBase(sizeof(uint16_t));
        turbulentRGBAColoredLights.ScaleIndexBase(sizeof(uint16_t));
        turbulentRotated.ScaleIndexBase(sizeof(uint16_t));
        turbulentRotatedRGBA.ScaleIndexBase(sizeof(uint16_t));
        turbulentRotatedLit.ScaleIndexBase(sizeof(uint16_t));
        turbulentRotatedColoredLights.ScaleIndexBase(sizeof(uint16_t));
        turbulentRotatedRGBALit.ScaleIndexBase(sizeof(uint16_t));
        turbulentRotatedRGBAColoredLights.ScaleIndexBase(sizeof(uint16_t));
    }
    else
    {
        sortedIndices16Size = 0;
        sortedIndices32Size = sortedIndicesCount * sizeof(uint32_t);
        surfaces.ScaleIndexBase(sizeof(uint32_t));
        surfacesColoredLights.ScaleIndexBase(sizeof(uint32_t));
        surfacesRGBA.ScaleIndexBase(sizeof(uint32_t));
        surfacesRGBAColoredLights.ScaleIndexBase(sizeof(uint32_t));
        surfacesRGBANoGlow.ScaleIndexBase(sizeof(uint32_t));
        surfacesRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint32_t));
        surfacesRotated.ScaleIndexBase(sizeof(uint32_t));
        surfacesRotatedColoredLights.ScaleIndexBase(sizeof(uint32_t));
        surfacesRotatedRGBA.ScaleIndexBase(sizeof(uint32_t));
        surfacesRotatedRGBAColoredLights.ScaleIndexBase(sizeof(uint32_t));
        surfacesRotatedRGBANoGlow.ScaleIndexBase(sizeof(uint32_t));
        surfacesRotatedRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint32_t));
        fences.ScaleIndexBase(sizeof(uint32_t));
        fencesColoredLights.ScaleIndexBase(sizeof(uint32_t));
        fencesRGBA.ScaleIndexBase(sizeof(uint32_t));
        fencesRGBAColoredLights.ScaleIndexBase(sizeof(uint32_t));
        fencesRGBANoGlow.ScaleIndexBase(sizeof(uint32_t));
        fencesRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint32_t));
        fencesRotated.ScaleIndexBase(sizeof(uint32_t));
        fencesRotatedColoredLights.ScaleIndexBase(sizeof(uint32_t));
        fencesRotatedRGBA.ScaleIndexBase(sizeof(uint32_t));
        fencesRotatedRGBAColoredLights.ScaleIndexBase(sizeof(uint32_t));
        fencesRotatedRGBANoGlow.ScaleIndexBase(sizeof(uint32_t));
        fencesRotatedRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint32_t));
        turbulent.ScaleIndexBase(sizeof(uint32_t));
        turbulentRGBA.ScaleIndexBase(sizeof(uint32_t));
        turbulentLit.ScaleIndexBase(sizeof(uint32_t));
        turbulentColoredLights.ScaleIndexBase(sizeof(uint32_t));
        turbulentRGBALit.ScaleIndexBase(sizeof(uint32_t));
        turbulentRGBAColoredLights.ScaleIndexBase(sizeof(uint32_t));
        turbulentRotated.ScaleIndexBase(sizeof(uint32_t));
        turbulentRotatedRGBA.ScaleIndexBase(sizeof(uint32_t));
        turbulentRotatedLit.ScaleIndexBase(sizeof(uint32_t));
        turbulentRotatedColoredLights.ScaleIndexBase(sizeof(uint32_t));
        turbulentRotatedRGBALit.ScaleIndexBase(sizeof(uint32_t));
        turbulentRotatedRGBAColoredLights.ScaleIndexBase(sizeof(uint32_t));
    }

    if (appState.IndexTypeUInt8Enabled)
    {
        indices8Size = floorIndicesSize + controllerIndicesSize + coloredIndices8Size;
        indices16Size = coloredIndices16Size;
    }
    else
    {
        floorIndicesSize *= sizeof(uint16_t);
        controllerIndicesSize *= sizeof(uint16_t);
        indices8Size = 0;
        indices16Size = floorIndicesSize + controllerIndicesSize + coloredIndices8Size * sizeof(uint16_t) + coloredIndices16Size;
    }

    if (indices8Size > 0)
    {
        perFrame.indices8 = perFrame.cachedIndices8.GetIndexBuffer(appState, indices8Size);
    }
    size += indices8Size;

    if (indices16Size + sortedIndices16Size > 0)
    {
        perFrame.indices16 = perFrame.cachedIndices16.GetIndexBuffer(appState, indices16Size + sortedIndices16Size);
    }
    size += indices16Size;
    size += sortedIndices16Size;

    indices32Size = (lastColoredIndex32 + 1) * sizeof(uint32_t);
    if (indices32Size + sortedIndices32Size > 0)
    {
        perFrame.indices32 = perFrame.cachedIndices32.GetIndexBuffer(appState, indices32Size + sortedIndices32Size);
    }
    size += indices32Size;
    size += sortedIndices32Size;

    return size;
}

void Scene::Reset()
{
    aliasTextureCache.clear();
    spriteCache.clear();
    surfaceTextureCache.clear();
    textures.DisposeFront();
    for (auto& cached : surfaceRGBATextures)
    {
        cached.DisposeFront();
    }
    for (auto& cached : surfaceTextures)
    {
        cached.DisposeFront();
    }
    while (deletedLightmapRGBATextures != nullptr)
    {
        auto lightmapTexture = deletedLightmapRGBATextures;
        deletedLightmapRGBATextures = deletedLightmapRGBATextures->next;
        delete lightmapTexture;
    }
    lightmapsRGBA.DisposeFront();
    while (deletedLightmapTextures != nullptr)
    {
        auto lightmapTexture = deletedLightmapTextures;
        deletedLightmapTextures = deletedLightmapTextures->next;
        delete lightmapTexture;
    }
    lightmaps.DisposeFront();
    indexBuffers.DisposeFront();
    buffers.DisposeFront();
    for (SharedMemoryBuffer* p = neutralPalette, *next; p != nullptr; p = next)
    {
        next = p->next;
        p->next = oldPalettes;
        oldPalettes = p;
    }
    neutralPalette = nullptr;
    for (SharedMemoryBuffer* p = palette, *next; p != nullptr; p = next)
    {
        next = p->next;
        p->next = oldPalettes;
        oldPalettes = p;
    }
    palette = nullptr;
    latestTextureDescriptorSets = nullptr;
    latestIndexBuffer32 = nullptr;
    usedInLatestIndexBuffer32 = 0;
    latestIndexBuffer16 = nullptr;
    usedInLatestIndexBuffer16 = 0;
    latestIndexBuffer8 = nullptr;
    usedInLatestIndexBuffer8 = 0;
    latestMemory.clear();
    Skybox::MoveToPrevious(*this);
    aliasIndexCache.clear();
    aliasVertexCache.clear();
}
