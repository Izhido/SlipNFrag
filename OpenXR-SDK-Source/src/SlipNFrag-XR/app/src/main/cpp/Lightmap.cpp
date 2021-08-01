#include "Lightmap.h"
#include "AppState.h"
#include "Utils.h"
#include "MemoryAllocateInfo.h"
#include "Constants.h"

void Lightmap::Create(AppState& appState, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage)
{
	this->width = width;
	this->height = height;
	this->layerCount = 1;
	VkImageCreateInfo imageCreateInfo { };
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = layerCount;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	CHECK_VKCMD(vkCreateImage(appState.Device, &imageCreateInfo, nullptr, &image));
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(appState.Device, image, &memoryRequirements);
	VkMemoryAllocateInfo memoryAllocateInfo { };
	createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo);
	VkDeviceSize blockSize = memoryAllocateInfo.allocationSize;
	if (blockSize == 0)
	{
		VkDeviceSize alignmentCorrection = 0;
		VkDeviceSize alignmentExcess = blockSize % memoryRequirements.alignment;
		if (alignmentExcess > 0)
		{
			alignmentCorrection = memoryRequirements.alignment - alignmentExcess;
		}
		blockSize = memoryAllocateInfo.allocationSize + alignmentCorrection;
	}
	auto& allocationList = appState.Scene.allocations[blockSize];
	for (auto allocation = allocationList.allocations.begin(); allocation != allocationList.allocations.end(); allocation++)
	{
		if (allocation->allocatedCount < allocation->allocated.size())
		{
			for (auto i = allocation->firstFreeCandidate; i < allocation->allocated.size(); i++)
			{
				if (!allocation->allocated[i])
				{
					allocation->allocated[i] = true;
					CHECK_VKCMD(vkBindImageMemory(appState.Device, image, allocation->memory, i * blockSize));
					this->allocationList = &allocationList;
					this->allocation = allocation;
					allocatedIndex = i;
					allocation->allocatedCount++;
					descriptorSet = allocation->descriptorSets[i];
					allocation->firstFreeCandidate = i + 1;
					break;
				}
			}
			break;
		}
	}
	if (descriptorSet == VK_NULL_HANDLE)
	{
		allocationList.allocations.emplace_back();
		allocation = allocationList.allocations.end();
		allocation--;
		VkDeviceSize size = MEMORY_BLOCK_SIZE;
		if (size < blockSize)
		{
			size = blockSize;
		}
		memoryAllocateInfo.allocationSize = size;
		CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &allocation->memory));
		auto count = size / blockSize;
		allocation->allocated.resize(count);
		VkDescriptorPoolSize poolSizes { };
		poolSizes.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes.descriptorCount = count;
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { };
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.maxSets = count;
		descriptorPoolCreateInfo.pPoolSizes = &poolSizes;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &allocation->descriptorPool));
		if (appState.Scene.descriptorSetLayouts.size() < count)
		{
			appState.Scene.descriptorSetLayouts.resize(count);
		}
		std::fill(appState.Scene.descriptorSetLayouts.begin(), appState.Scene.descriptorSetLayouts.begin() + count, appState.Scene.singleImageLayout);
		allocation->descriptorSets.resize(count);
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { };
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.descriptorPool = allocation->descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = count;
		descriptorSetAllocateInfo.pSetLayouts = appState.Scene.descriptorSetLayouts.data();
		CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, allocation->descriptorSets.data()));
		allocation->allocated[0] = true;
		CHECK_VKCMD(vkBindImageMemory(appState.Device, image, allocation->memory, 0));
		this->allocationList = &allocationList;
		allocation->allocatedCount++;
		descriptorSet = allocation->descriptorSets[0];
	}
	VkImageViewCreateInfo imageViewCreateInfo { };
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = imageCreateInfo.format;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.layerCount = layerCount;
	CHECK_VKCMD(vkCreateImageView(appState.Device, &imageViewCreateInfo, nullptr, &view));
	VkDescriptorImageInfo textureInfo { };
	textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	textureInfo.sampler = appState.Scene.lightmapSampler;
	textureInfo.imageView = view;
	VkWriteDescriptorSet writes { };
	writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes.descriptorCount = 1;
	writes.dstSet = descriptorSet;
	writes.pImageInfo = &textureInfo;
	writes.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	vkUpdateDescriptorSets(appState.Device, 1, &writes, 0, nullptr);
}

void Lightmap::Fill(AppState& appState, StagingBuffer& buffer)
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
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = layerCount;
	vkCmdPipelineBarrier(buffer.commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	VkBufferImageCopy region { };
	region.bufferOffset = buffer.offset;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;
	vkCmdCopyBufferToImage(buffer.commandBuffer, buffer.buffer->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	filled = true;
}

void Lightmap::Delete(AppState& appState)
{
	if (view != VK_NULL_HANDLE)
	{
		vkDestroyImageView(appState.Device, view, nullptr);
	}
	if (image != VK_NULL_HANDLE)
	{
		vkDestroyImage(appState.Device, image, nullptr);
	}
	allocation->allocated[allocatedIndex] = false;
	allocation->allocatedCount--;
	if (allocation->firstFreeCandidate > allocatedIndex)
	{
		allocation->firstFreeCandidate = allocatedIndex;
	}
	if (allocation->allocatedCount == 0)
	{
		vkDestroyDescriptorPool(appState.Device, allocation->descriptorPool, nullptr);
		vkFreeMemory(appState.Device, allocation->memory, nullptr);
		allocationList->allocations.erase(allocation);
	}
}
