#include "Lightmap.h"
#include "AppState.h"
#include "Utils.h"
#include "MemoryAllocateInfo.h"
#include "Constants.h"

void Lightmap::Create(AppState& appState, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage)
{
	this->width = width;
	this->height = height;

	size_t slot0 = std::ceil(std::log2(width));
	size_t slot1 = std::ceil(std::log2(height));
	if (appState.Scene.lightmapTextures.size() <= slot0)
	{
		appState.Scene.lightmapTextures.resize(slot0 + 1);
	}
	auto& lightmapTextures = appState.Scene.lightmapTextures[slot0];
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
			if (appState.Scene.deletedLightmapTextures != nullptr)
			{
				lightmapTexture = appState.Scene.deletedLightmapTextures;
				appState.Scene.deletedLightmapTextures = appState.Scene.deletedLightmapTextures->next;
				lightmapTexture->Initialize();
			}
			else
			{
				lightmapTexture = new LightmapTexture { };
			}
			lightmapTextures[slot1] = lightmapTexture;
		}
		else
		{
			if (appState.Scene.deletedLightmapTextures != nullptr)
			{
				lightmapTexture->next = appState.Scene.deletedLightmapTextures;
				appState.Scene.deletedLightmapTextures = appState.Scene.deletedLightmapTextures->next;
				lightmapTexture->next->Initialize();
			}
			else
			{
				lightmapTexture->next = new LightmapTexture { };
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

		VkImageCreateInfo imageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent.width = texture->width;
		imageCreateInfo.extent.height = texture->height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = arrayLayers;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.usage = usage;
		CHECK_VKCMD(vkCreateImage(appState.Device, &imageCreateInfo, nullptr, &texture->image));

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(appState.Device, texture->image, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocateInfo { };
		createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo, true);

		CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &texture->memory));

		texture->allocated.resize(arrayLayers);

		VkDescriptorPoolSize poolSizes { };
		poolSizes.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes.descriptorCount = 1;

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		descriptorPoolCreateInfo.maxSets = 1;
		descriptorPoolCreateInfo.pPoolSizes = &poolSizes;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &texture->descriptorPool));

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descriptorSetAllocateInfo.descriptorPool = texture->descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
		CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &texture->descriptorSet));

		texture->allocated[0] = true;

		CHECK_VKCMD(vkBindImageMemory(appState.Device, texture->image, texture->memory, 0));

		texture->allocatedCount++;

		VkImageViewCreateInfo imageViewCreateInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		imageViewCreateInfo.image = texture->image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		imageViewCreateInfo.format = imageCreateInfo.format;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.layerCount = arrayLayers;
		CHECK_VKCMD(vkCreateImageView(appState.Device, &imageViewCreateInfo, nullptr, &texture->view));

		VkDescriptorImageInfo textureInfo { };
		textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		textureInfo.sampler = appState.Scene.lightmapSampler;
		textureInfo.imageView = texture->view;

		VkWriteDescriptorSet write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.descriptorCount = 1;
		write.dstSet = texture->descriptorSet;
		write.pImageInfo = &textureInfo;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		vkUpdateDescriptorSets(appState.Device, 1, &write, 0, nullptr);

		texture->regions.resize(arrayLayers);
		for (auto& region : texture->regions)
		{
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent.depth = 1;
		}
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
		vkDestroyImageView(appState.Device, texture->view, nullptr);
		vkDestroyImage(appState.Device, texture->image, nullptr);
		vkFreeMemory(appState.Device, texture->memory, nullptr);
		if (texture->previous != nullptr)
		{
			texture->previous->next = texture->next;
		}
		else
		{
			size_t slot0 = std::ceil(std::log2(width));
			size_t slot1 = std::ceil(std::log2(height));
			appState.Scene.lightmapTextures[slot0][slot1] = texture->next;
		}
		if (texture->next != nullptr)
		{
			texture->next->previous = texture->previous;
		}
		texture->next = appState.Scene.deletedLightmapTextures;
		appState.Scene.deletedLightmapTextures = texture;
	}
}
