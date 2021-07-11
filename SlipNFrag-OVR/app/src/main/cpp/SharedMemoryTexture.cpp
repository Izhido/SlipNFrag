#include "SharedMemoryTexture.h"
#include "AppState.h"
#include "VulkanCallWrappers.h"
#include "MemoryAllocateInfo.h"
#include "Constants.h"

void SharedMemoryTexture::Create(AppState& appState, uint32_t width, uint32_t height, VkFormat format, uint32_t mipCount, VkImageUsageFlags usage)
{
	this->width = width;
	this->height = height;
	this->mipCount = mipCount;
	this->layerCount = 1;
	VkImageCreateInfo imageCreateInfo { };
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = mipCount;
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
	VkDeviceSize alignmentCorrection = 0;
	VkDeviceSize alignmentExcess = appState.Scene.usedInLatestTextureSharedMemory % memoryRequirements.alignment;
	if (alignmentExcess > 0)
	{
		alignmentCorrection = memoryRequirements.alignment - alignmentExcess;
	}
	if (appState.Scene.latestTextureSharedMemory == nullptr || appState.Scene.usedInLatestTextureSharedMemory + alignmentCorrection + memoryAllocateInfo.allocationSize > MEMORY_BLOCK_SIZE)
	{
		VkDeviceSize size = MEMORY_BLOCK_SIZE;
		if (size < memoryAllocateInfo.allocationSize)
		{
			size = memoryAllocateInfo.allocationSize;
		}
		auto memoryBlockAllocateInfo = memoryAllocateInfo;
		memoryBlockAllocateInfo.allocationSize = size;
		appState.Scene.latestTextureSharedMemory = new SharedMemory { };
		VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryBlockAllocateInfo, nullptr, &appState.Scene.latestTextureSharedMemory->memory));
		appState.Scene.usedInLatestTextureSharedMemory = 0;
		alignmentCorrection = 0;
	}
	VK(appState.Device.vkBindImageMemory(appState.Device.device, image, appState.Scene.latestTextureSharedMemory->memory, appState.Scene.usedInLatestTextureSharedMemory + alignmentCorrection));
	appState.Scene.usedInLatestTextureSharedMemory += (alignmentCorrection + memoryAllocateInfo.allocationSize);
	appState.Scene.latestTextureSharedMemory->referenceCount++;
	sharedMemory = appState.Scene.latestTextureSharedMemory;
	VkImageViewCreateInfo imageViewCreateInfo { };
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = imageCreateInfo.format;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.levelCount = mipCount;
	imageViewCreateInfo.subresourceRange.layerCount = layerCount;
	VK(appState.Device.vkCreateImageView(appState.Device.device, &imageViewCreateInfo, nullptr, &view));
	if (appState.Scene.samplers.size() <= mipCount)
	{
		appState.Scene.samplers.resize(mipCount + 1);
	}
	if (appState.Scene.samplers[mipCount] == VK_NULL_HANDLE)
	{
		VkSamplerCreateInfo samplerCreateInfo { };
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.maxLod = mipCount - 1;
		VK(appState.Device.vkCreateSampler(appState.Device.device, &samplerCreateInfo, nullptr, &appState.Scene.samplers[mipCount]));
	}
	if (appState.Scene.latestTextureDescriptorSets == nullptr || appState.Scene.latestTextureDescriptorSets->used >= appState.Scene.latestTextureDescriptorSets->descriptorSets.size())
	{
		appState.Scene.latestTextureDescriptorSets = new DescriptorSets();
		VkDescriptorPoolSize poolSizes { };
		poolSizes.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes.descriptorCount = DESCRIPTOR_SET_COUNT;
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { };
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.maxSets = DESCRIPTOR_SET_COUNT;
		descriptorPoolCreateInfo.pPoolSizes = &poolSizes;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &appState.Scene.latestTextureDescriptorSets->descriptorPool));
		if (appState.Scene.descriptorSetLayouts.size() < DESCRIPTOR_SET_COUNT)
		{
			appState.Scene.descriptorSetLayouts.resize(DESCRIPTOR_SET_COUNT);
		}
		std::fill(appState.Scene.descriptorSetLayouts.begin(), appState.Scene.descriptorSetLayouts.begin() + DESCRIPTOR_SET_COUNT, appState.Scene.singleImageLayout);
		appState.Scene.latestTextureDescriptorSets->descriptorSets.resize(DESCRIPTOR_SET_COUNT);
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { };
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.descriptorPool = appState.Scene.latestTextureDescriptorSets->descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = DESCRIPTOR_SET_COUNT;
		descriptorSetAllocateInfo.pSetLayouts = appState.Scene.descriptorSetLayouts.data();
		VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, appState.Scene.latestTextureDescriptorSets->descriptorSets.data()));
	}
	descriptorSet = appState.Scene.latestTextureDescriptorSets->descriptorSets[appState.Scene.latestTextureDescriptorSets->used];
	appState.Scene.latestTextureDescriptorSets->used++;
	appState.Scene.latestTextureDescriptorSets->referenceCount++;
	descriptorSets = appState.Scene.latestTextureDescriptorSets;
	VkDescriptorImageInfo textureInfo { };
	textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	textureInfo.sampler = appState.Scene.samplers[mipCount];
	textureInfo.imageView = view;
	VkWriteDescriptorSet writes { };
	writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes.descriptorCount = 1;
	writes.dstSet = descriptorSet;
	writes.pImageInfo = &textureInfo;
	writes.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, &writes, 0, nullptr));
}

void SharedMemoryTexture::FillMipmapped(AppState& appState, StagingBuffer& buffer)
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
	barrier.subresourceRange.levelCount = mipCount;
	barrier.subresourceRange.layerCount = layerCount;
	VC(appState.Device.vkCmdPipelineBarrier(buffer.commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier));
	VkBufferImageCopy region { };
	region.bufferOffset = buffer.offset;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = layerCount;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;
	VC(appState.Device.vkCmdCopyBufferToImage(buffer.commandBuffer, buffer.buffer->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.subresourceRange.levelCount = 1;
	VC(appState.Device.vkCmdPipelineBarrier(buffer.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier));
	auto width = this->width;
	auto height = this->height;
	VkImageBlit blit { };
	blit.srcOffsets[1].x = width;
	blit.srcOffsets[1].y = height;
	blit.srcOffsets[1].z = 1;
	blit.dstOffsets[1].z = 1;
	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.mipLevel = 0;
	blit.srcSubresource.layerCount = layerCount;
	for (auto i = 1; i < mipCount; i++)
	{
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
		blit.dstOffsets[1].x = width;
		blit.dstOffsets[1].y = height;
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.layerCount = layerCount;
		VC(appState.Device.vkCmdBlitImage(buffer.commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST));
	}
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.subresourceRange.levelCount = 1;
	if (mipCount > 1)
	{
		buffer.lastBarrier++;
		if (buffer.imageBarriers.size() <= buffer.lastBarrier)
		{
			buffer.imageBarriers.emplace_back();
		}
		auto& writeBarrier = buffer.imageBarriers[buffer.lastBarrier];
		writeBarrier = barrier;
		writeBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		writeBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		writeBarrier.subresourceRange.baseMipLevel = 1;
		writeBarrier.subresourceRange.levelCount = mipCount - 1;
	}
	filled = true;
}

void SharedMemoryTexture::Delete(AppState& appState)
{
	descriptorSets->referenceCount--;
	if (descriptorSets->referenceCount == 0)
	{
		VC(appState.Device.vkDestroyDescriptorPool(appState.Device.device, descriptorSets->descriptorPool, nullptr));
	}
	if (view != VK_NULL_HANDLE)
	{
		VC(appState.Device.vkDestroyImageView(appState.Device.device, view, nullptr));
	}
	if (image != VK_NULL_HANDLE)
	{
		VC(appState.Device.vkDestroyImage(appState.Device.device, image, nullptr));
	}
	sharedMemory->referenceCount--;
	if (sharedMemory->referenceCount == 0)
	{
		VC(appState.Device.vkFreeMemory(appState.Device.device, sharedMemory->memory, nullptr));
		delete sharedMemory;
	}
}

void SharedMemoryTexture::DeleteOld(AppState& appState, SharedMemoryTexture** old)
{
	if (old != nullptr)
	{
		for (auto t = old; *t != nullptr; )
		{
			(*t)->unusedCount++;
			if ((*t)->unusedCount >= MAX_UNUSED_COUNT)
			{
				auto next = (*t)->next;
				(*t)->Delete(appState);
				delete *t;
				*t = next;
			}
			else
			{
				t = &(*t)->next;
			}
		}
	}
}