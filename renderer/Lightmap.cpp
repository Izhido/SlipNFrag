#include "Lightmap.h"
#include "AppState.h"
#include "Utils.h"
#include "MemoryAllocateInfo.h"
#include "Constants.h"

void Lightmap::Create(AppState& appState, uint32_t width, uint32_t height)
{
	this->width = width;
	this->height = height;

	auto slot0 = std::ceil(std::log2(width));
	auto slot1 = std::ceil(std::log2(height));
	auto slot = (size_t)std::log2(std::pow(2, slot0) * std::pow(2, slot1));
	if (appState.Scene.lightmapTextures.size() <= slot)
	{
		appState.Scene.lightmapTextures.resize(slot + 1);
	}
	auto lightmapTexture = appState.Scene.lightmapTextures[slot];
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
			lightmapTexture = new LightmapTexture { };
			appState.Scene.lightmapTextures[slot] = lightmapTexture;
		}
		else
		{
			lightmapTexture->next = new LightmapTexture { };
			lightmapTexture->next->previous = lightmapTexture;
			lightmapTexture = lightmapTexture->next;
		}
		texture = lightmapTexture;

		texture->width = (int)std::pow(2, slot0);
		texture->height = (int)std::pow(2, slot1);

		auto maxSlot = std::max(slot0, slot1);
		size_t arrayLayers;
		if (maxSlot <= 3)
		{
			arrayLayers = 256;
		}
		else if (maxSlot >= 11)
		{
			arrayLayers = 1;
		}
		else
		{
			arrayLayers = (size_t)std::pow(2, 11 - maxSlot);
		}

		texture->size = texture->width * texture->height;

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

void Lightmap::Delete(AppState& appState) const
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
			auto slot0 = std::ceil(std::log2(width));
			auto slot1 = std::ceil(std::log2(height));
			auto slot = (size_t)std::log2(std::pow(2, slot0) * std::pow(2, slot1));
			appState.Scene.lightmapTextures[slot] = texture->next;
		}
		if (texture->next != nullptr)
		{
			texture->next->previous = texture->previous;
		}
		delete texture;
	}
}
