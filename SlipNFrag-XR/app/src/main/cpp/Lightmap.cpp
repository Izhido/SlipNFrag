#include "Lightmap.h"
#include "AppState.h"
#include "Utils.h"
#include "MemoryAllocateInfo.h"
#include "Constants.h"

void Lightmap::Create(AppState& appState, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage)
{
	this->width = width;
	this->height = height;

	VkDeviceSize dimension = std::pow(2, std::ceil(std::log2(std::max(width, height))));
	auto& list = appState.Scene.lightmapTextures[dimension];
	bool found = false;
	for (auto texture = list.begin(); texture != list.end(); texture++)
	{
		if (texture->allocatedCount < texture->allocated.size())
		{
			for (auto i = texture->firstFreeCandidate; i < texture->allocated.size(); i++)
			{
				if (!texture->allocated[i])
				{
					texture->allocated[i] = true;
					this->textureList = &list;
					this->texture = texture;
					allocatedIndex = i;
					texture->allocatedCount++;
					texture->firstFreeCandidate = i + 1;
					found = true;
					break;
				}
			}
			break;
		}
	}

	if (!found)
	{
		list.emplace_back();
		texture = list.end();
		texture--;
		size_t count;
		texture->width = dimension;
		texture->height = dimension;
		if (dimension > 1024)
		{
			count = 1;
		}
		else
		{
			count = 1024 / dimension;
		}

		VkImageCreateInfo imageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent.width = texture->width;
		imageCreateInfo.extent.height = texture->height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = count;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.usage = usage;
		CHECK_VKCMD(vkCreateImage(appState.Device, &imageCreateInfo, nullptr, &texture->image));

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(appState.Device, texture->image, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocateInfo { };
		createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo, true);

		CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &texture->memory));

		texture->allocated.resize(count);

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

		this->textureList = &list;
		texture->allocatedCount++;

		VkImageViewCreateInfo imageViewCreateInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		imageViewCreateInfo.image = texture->image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		imageViewCreateInfo.format = imageCreateInfo.format;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.layerCount = count;
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

		texture->regions.resize(count);
		for (auto& region : texture->regions)
		{
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent.depth = 1;
		}
	}
}

void Lightmap::Fill(AppState& appState, uint32_t regions, StagingBuffer& buffer)
{
	if (buffer.descriptorSetsInUse.find(texture->descriptorSet) == buffer.descriptorSetsInUse.end())
	{
		buffer.lastBarrier++;
		if (buffer.imageBarriers.size() <= buffer.lastBarrier)
		{
			buffer.imageBarriers.emplace_back();
		}

		auto& barrier = buffer.imageBarriers[buffer.lastBarrier];
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		if (filled)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}
		else
		{
			barrier.srcAccessMask = 0;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.image = texture->image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = texture->allocated.size();
		vkCmdPipelineBarrier(buffer.commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		vkCmdCopyBufferToImage(buffer.commandBuffer, buffer.buffer->buffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions, texture->regions.data());

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		buffer.descriptorSetsInUse.insert(texture->descriptorSet);
	}
	else
	{
		vkCmdCopyBufferToImage(buffer.commandBuffer, buffer.buffer->buffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions, texture->regions.data());
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
		textureList->erase(texture);
	}
}
