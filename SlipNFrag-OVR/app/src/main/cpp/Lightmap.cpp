#include "Lightmap.h"
#include "AppState.h"
#include "VulkanCallWrappers.h"
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
	VK(appState.Device.vkCreateImage(appState.Device.device, &imageCreateInfo, nullptr, &image));
	VkMemoryRequirements memoryRequirements;
	VC(appState.Device.vkGetImageMemoryRequirements(appState.Device.device, image, &memoryRequirements));
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
					VK(appState.Device.vkBindImageMemory(appState.Device.device, image, allocation->memory, i * blockSize));
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
		VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryAllocateInfo, nullptr, &allocation->memory));
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
		VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &allocation->descriptorPool));
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
		VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, allocation->descriptorSets.data()));
		allocation->allocated[0] = true;
		VK(appState.Device.vkBindImageMemory(appState.Device.device, image, allocation->memory, 0));
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
	VK(appState.Device.vkCreateImageView(appState.Device.device, &imageViewCreateInfo, nullptr, &view));
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
	VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, &writes, 0, nullptr));
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
	VC(appState.Device.vkCmdPipelineBarrier(buffer.commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier));
	VkBufferImageCopy region { };
	region.bufferOffset = buffer.offset;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;
	VC(appState.Device.vkCmdCopyBufferToImage(buffer.commandBuffer, buffer.buffer->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
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
		VC(appState.Device.vkDestroyImageView(appState.Device.device, view, nullptr));
	}
	if (image != VK_NULL_HANDLE)
	{
		VC(appState.Device.vkDestroyImage(appState.Device.device, image, nullptr));
	}
	allocation->allocated[allocatedIndex] = false;
	allocation->allocatedCount--;
	if (allocation->firstFreeCandidate > allocatedIndex)
	{
		allocation->firstFreeCandidate = allocatedIndex;
	}
	if (allocation->allocatedCount == 0)
	{
		VC(appState.Device.vkDestroyDescriptorPool(appState.Device.device, allocation->descriptorPool, nullptr));
		VC(appState.Device.vkFreeMemory(appState.Device.device, allocation->memory, nullptr));
		allocationList->allocations.erase(allocation);
	}
}
