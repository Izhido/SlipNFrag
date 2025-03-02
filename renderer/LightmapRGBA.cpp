#include "LightmapRGBA.h"
#include "AppState.h"
#include "Utils.h"
#include "MemoryAllocateInfo.h"
#include "Constants.h"

void LightmapRGBA::Create(AppState& appState, uint32_t width, uint32_t height)
{
	this->width = width;
	this->height = height;

	size_t slot0 = std::ceil(std::log2(width));
	size_t slot1 = std::ceil(std::log2(height));
	if (appState.Scene.lightmapRGBATextures.size() <= slot0)
	{
		appState.Scene.lightmapRGBATextures.resize(slot0 + 1);
	}
	auto& lightmapTextures = appState.Scene.lightmapRGBATextures[slot0];
	if (lightmapTextures.size() <= slot1)
	{
		lightmapTextures.resize(slot1 + 1);
	}
	auto lightmapTexture = lightmapTextures[slot1];
	bool found = false;
	while (lightmapTexture != nullptr)
	{
		if (lightmapTexture->allocatedCount < lightmapTexture->allocated.size())
		{
			for (auto i = lightmapTexture->firstFreeCandidate; i < lightmapTexture->allocated.size(); i++)
			{
				if (!lightmapTexture->allocated[i])
				{
					lightmapTexture->allocated[i] = true;
					texture = lightmapTexture;
					allocatedIndex = i;
					offset = allocatedIndex * lightmapTexture->size;
					lightmapTexture->allocatedCount++;
					lightmapTexture->firstFreeCandidate = i + 1;
					found = true;
					break;
				}
			}
			break;
		}
		if (lightmapTexture->next == nullptr)
		{
			break;
		}
		lightmapTexture = lightmapTexture->next;
	}

	if (!found)
	{
		if (lightmapTexture == nullptr)
		{
			if (appState.Scene.deletedLightmapRGBATextures != nullptr)
			{
				lightmapTexture = appState.Scene.deletedLightmapRGBATextures;
				appState.Scene.deletedLightmapRGBATextures = appState.Scene.deletedLightmapRGBATextures->next;
				lightmapTexture->Initialize();
			}
			else
			{
				lightmapTexture = new LightmapRGBATexture { };
			}
			lightmapTextures[slot1] = lightmapTexture;
		}
		else
		{
			if (appState.Scene.deletedLightmapRGBATextures != nullptr)
			{
				lightmapTexture->next = appState.Scene.deletedLightmapRGBATextures;
				appState.Scene.deletedLightmapRGBATextures = appState.Scene.deletedLightmapRGBATextures->next;
				lightmapTexture->next->Initialize();
			}
			else
			{
				lightmapTexture->next = new LightmapRGBATexture { };
			}
			lightmapTexture->next->previous = lightmapTexture;
			lightmapTexture = lightmapTexture->next;
		}
		texture = lightmapTexture;

		texture->width = (int)std::pow(2, slot0);
		texture->height = (int)std::pow(2, slot1);

		auto maxDimension = std::max(texture->width, texture->height);
		uint32_t arrayLayers;
		if (maxDimension <= 4)
		{
			arrayLayers = 256;
		}
		else if (maxDimension >= 1024)
		{
			arrayLayers = 1;
		}
		else
		{
			auto maxSlot = (int)std::max(slot0, slot1);
			arrayLayers = (int)std::pow(2, 10 - maxSlot);
		}

		texture->size = texture->width * texture->height * 3;

		texture->buffer.CreateStorageBuffer(appState, texture->size * arrayLayers * sizeof(uint32_t));

		texture->allocated.resize(arrayLayers);

		VkDescriptorPoolSize poolSizes { };
		poolSizes.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes.descriptorCount = 1;

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		descriptorPoolCreateInfo.maxSets = 1;
		descriptorPoolCreateInfo.pPoolSizes = &poolSizes;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &texture->descriptorPool));

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descriptorSetAllocateInfo.descriptorPool = texture->descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleFragmentStorageBufferLayout;
		CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &texture->descriptorSet));

		texture->allocated[0] = true;

		texture->allocatedCount++;

		VkDescriptorBufferInfo bufferInfo { };
		bufferInfo.range = VK_WHOLE_SIZE;
		bufferInfo.buffer = texture->buffer.buffer;

		VkWriteDescriptorSet write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.descriptorCount = 1;
		write.dstSet = texture->descriptorSet;
		write.pBufferInfo = &bufferInfo;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vkUpdateDescriptorSets(appState.Device, 1, &write, 0, nullptr);
	}
}

void LightmapRGBA::Delete(AppState& appState) const
{
	texture->allocated[allocatedIndex] = false;
	texture->allocatedCount--;
	if (texture->firstFreeCandidate > allocatedIndex)
	{
		texture->firstFreeCandidate = allocatedIndex;
	}
	if (texture->allocatedCount == 0)
	{
		vkDestroyDescriptorPool(appState.Device, texture->descriptorPool, nullptr);
		texture->buffer.Delete(appState);
		if (texture->previous != nullptr)
		{
			texture->previous->next = texture->next;
		}
		else
		{
			size_t slot0 = std::ceil(std::log2(width));
			size_t slot1 = std::ceil(std::log2(height));
			appState.Scene.lightmapRGBATextures[slot0][slot1] = texture->next;
		}
		if (texture->next != nullptr)
		{
			texture->next->previous = texture->previous;
		}
		texture->next = appState.Scene.deletedLightmapRGBATextures;
		appState.Scene.deletedLightmapRGBATextures = texture;
	}
}
