#include "SharedMemoryTexture.h"
#include "AppState.h"
#include "Utils.h"
#include "MemoryAllocateInfo.h"
#include "Constants.h"

void SharedMemoryTexture::Create(AppState& appState, uint32_t width, uint32_t height, VkFormat format, uint32_t mipCount, uint32_t layerCount, VkImageUsageFlags usage)
{
	this->width = width;
	this->height = height;
	this->mipCount = mipCount;
	this->layerCount = layerCount;

	VkImageCreateInfo imageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = mipCount;
	imageCreateInfo.arrayLayers = layerCount;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.usage = usage;
	CHECK_VKCMD(vkCreateImage(appState.Device, &imageCreateInfo, nullptr, &image));

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(appState.Device, image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo { };
	createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo, true);

	auto create = true;
	auto& latestMemory = appState.Scene.latestMemory[memoryAllocateInfo.memoryTypeIndex];
	for (auto entry = latestMemory.begin(); entry != latestMemory.end(); entry++)
	{
		VkDeviceSize alignmentCorrection = 0;
		VkDeviceSize alignmentExcess = entry->used % memoryRequirements.alignment;
		if (alignmentExcess > 0)
		{
			alignmentCorrection = memoryRequirements.alignment - alignmentExcess;
		}
		if (entry->used + alignmentCorrection + memoryAllocateInfo.allocationSize <= entry->memory->size)
		{
			create = false;
			sharedMemory = entry->memory;
			CHECK_VKCMD(vkBindImageMemory(appState.Device, image, sharedMemory->memory, entry->used + alignmentCorrection));
			entry->used += (alignmentCorrection + memoryAllocateInfo.allocationSize);
			if (entry->used >= entry->memory->size)
			{
				latestMemory.erase(entry);
			}
			break;
		}
	}

	if (create)
	{
		sharedMemory = new SharedMemory { };
		sharedMemory->size = Constants::memoryBlockSize;
		auto add = true;
		if (sharedMemory->size < memoryAllocateInfo.allocationSize)
		{
			sharedMemory->size = memoryAllocateInfo.allocationSize;
			add = false;
		}
		auto memoryBlockAllocateInfo = memoryAllocateInfo;
		memoryBlockAllocateInfo.allocationSize = sharedMemory->size;
		CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryBlockAllocateInfo, nullptr, &sharedMemory->memory));
		if (add)
		{
			latestMemory.push_back({ sharedMemory, memoryAllocateInfo.allocationSize });
		}
		CHECK_VKCMD(vkBindImageMemory(appState.Device, image, sharedMemory->memory, 0));
	}

	sharedMemory->referenceCount++;

	VkImageViewCreateInfo imageViewCreateInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = (layerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D);
	imageViewCreateInfo.format = imageCreateInfo.format;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.levelCount = mipCount;
	imageViewCreateInfo.subresourceRange.layerCount = layerCount;
	CHECK_VKCMD(vkCreateImageView(appState.Device, &imageViewCreateInfo, nullptr, &view));

	if (appState.Scene.samplers.size() <= mipCount)
	{
		appState.Scene.samplers.resize(mipCount + 1);
	}
	if (appState.Scene.samplers[mipCount] == VK_NULL_HANDLE)
	{
		VkSamplerCreateInfo samplerCreateInfo { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerCreateInfo.maxLod = mipCount - 1;
		CHECK_VKCMD(vkCreateSampler(appState.Device, &samplerCreateInfo, nullptr, &appState.Scene.samplers[mipCount]));
	}

	if (appState.Scene.latestTextureDescriptorSets == nullptr || appState.Scene.latestTextureDescriptorSets->used >= appState.Scene.latestTextureDescriptorSets->descriptorSets.size())
	{
		appState.Scene.latestTextureDescriptorSets = new DescriptorSets { };

		VkDescriptorPoolSize poolSizes { };
		poolSizes.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes.descriptorCount = Constants::descriptorSetCount;

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		descriptorPoolCreateInfo.maxSets = Constants::descriptorSetCount;
		descriptorPoolCreateInfo.pPoolSizes = &poolSizes;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &appState.Scene.latestTextureDescriptorSets->descriptorPool));

		if (appState.Scene.descriptorSetLayouts.size() < Constants::descriptorSetCount)
		{
			appState.Scene.descriptorSetLayouts.resize(Constants::descriptorSetCount);
		}
		std::fill(appState.Scene.descriptorSetLayouts.begin(), appState.Scene.descriptorSetLayouts.begin() + Constants::descriptorSetCount, appState.Scene.singleImageLayout);
		appState.Scene.latestTextureDescriptorSets->descriptorSets.resize(Constants::descriptorSetCount);

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descriptorSetAllocateInfo.descriptorPool = appState.Scene.latestTextureDescriptorSets->descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = Constants::descriptorSetCount;
		descriptorSetAllocateInfo.pSetLayouts = appState.Scene.descriptorSetLayouts.data();
		CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, appState.Scene.latestTextureDescriptorSets->descriptorSets.data()));
	}

	descriptorSet = appState.Scene.latestTextureDescriptorSets->descriptorSets[appState.Scene.latestTextureDescriptorSets->used];

	appState.Scene.latestTextureDescriptorSets->used++;
	appState.Scene.latestTextureDescriptorSets->referenceCount++;

	descriptorSets = appState.Scene.latestTextureDescriptorSets;

	VkDescriptorImageInfo textureInfo { };
	textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	textureInfo.sampler = appState.Scene.samplers[mipCount];
	textureInfo.imageView = view;

	VkWriteDescriptorSet write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.dstSet = descriptorSet;
	write.pImageInfo = &textureInfo;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	vkUpdateDescriptorSets(appState.Device, 1, &write, 0, nullptr);
}

void SharedMemoryTexture::FillMipmapped(AppState& appState, StagingBuffer& buffer, int mips, int layer)
{
	VkImageMemoryBarrier barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = mipCount;
	barrier.subresourceRange.layerCount = layerCount;
	vkCmdPipelineBarrier(buffer.commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	auto toCopy = std::max(mips, 1);
	auto offset = 0;
	VkImageBlit blit { };
	auto mipWidth = width;
	auto mipHeight = height;
	std::vector<VkBufferImageCopy> regions(toCopy);
	for (auto i = 0; i < toCopy; i++)
	{
		auto& region = regions[i];
		region.bufferOffset = buffer.offset + offset;
		region.imageSubresource.aspectMask = barrier.subresourceRange.aspectMask;
		region.imageSubresource.mipLevel = i;
		region.imageSubresource.baseArrayLayer = layer;
		region.imageSubresource.layerCount = 1;
		region.imageExtent.width = mipWidth;
		region.imageExtent.height = mipHeight;
		region.imageExtent.depth = 1;
		offset += mipWidth * mipHeight;
		blit.srcOffsets[1].x = mipWidth;
		blit.srcOffsets[1].y = mipHeight;
		mipWidth /= 2;
		if (mipWidth < 1)
		{
			mipWidth = 1;
		}
		mipHeight /= 2;
		if (mipHeight < 1)
		{
			mipHeight = 1;
		}
	}
	vkCmdCopyBufferToImage(buffer.commandBuffer, buffer.buffer->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, toCopy, regions.data());
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.subresourceRange.levelCount = toCopy;
	vkCmdPipelineBarrier(buffer.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	blit.srcOffsets[1].z = 1;
	blit.srcSubresource.aspectMask = barrier.subresourceRange.aspectMask;
	blit.srcSubresource.mipLevel = toCopy - 1;
	blit.srcSubresource.baseArrayLayer = layer;
	blit.srcSubresource.layerCount = 1;
	blit.dstOffsets[1].z = blit.srcOffsets[1].z;
	blit.dstSubresource.aspectMask = blit.srcSubresource.aspectMask;
	blit.dstSubresource.baseArrayLayer = blit.srcSubresource.baseArrayLayer;
	blit.dstSubresource.layerCount = blit.srcSubresource.layerCount;
	for (auto i = toCopy; i < mipCount; i++)
	{
		blit.dstOffsets[1].x = mipWidth;
		blit.dstOffsets[1].y = mipHeight;
		blit.dstSubresource.mipLevel = i;
		vkCmdBlitImage(buffer.commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);
		mipWidth /= 2;
		if (mipWidth < 1)
		{
			mipWidth = 1;
		}
		mipHeight /= 2;
		if (mipHeight < 1)
		{
			mipHeight = 1;
		}
	}
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = toCopy;
	buffer.lastBarrier++;
	if (buffer.imageBarriers.size() <= buffer.lastBarrier)
	{
		buffer.imageBarriers.emplace_back();
	}
	buffer.imageBarriers[buffer.lastBarrier] = barrier;
	if (mipCount > 1)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.subresourceRange.baseMipLevel = toCopy;
		barrier.subresourceRange.levelCount = mipCount - toCopy;
		buffer.lastBarrier++;
		if (buffer.imageBarriers.size() <= buffer.lastBarrier)
		{
			buffer.imageBarriers.emplace_back();
		}
		buffer.imageBarriers[buffer.lastBarrier] = barrier;
	}
	filled = true;
}

void SharedMemoryTexture::FillMipmapped(AppState& appState, StagingBuffer& buffer, int layer)
{
	std::vector<VkBufferImageCopy> regions(mipCount);
	auto offset = 0;
	auto mipWidth = width;
	auto mipHeight = height;
	for (int i = 0; i < regions.size(); i++)
	{
		auto& region = regions[i];
		region.bufferOffset = buffer.offset + offset;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.baseArrayLayer = layer;
		region.imageSubresource.layerCount = 1;
		region.imageSubresource.mipLevel = i;
		region.imageExtent.width = mipWidth;
		region.imageExtent.height = mipHeight;
		region.imageExtent.depth = 1;
		offset += mipWidth * mipHeight;
		mipWidth /= 2;
		if (mipWidth < 1)
		{
			mipWidth = 1;
		}
		mipHeight /= 2;
		if (mipHeight < 1)
		{
			mipHeight = 1;
		}
	}

	if (buffer.descriptorSetsInUse.find(descriptorSet) == buffer.descriptorSetsInUse.end())
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
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = layerCount;
		vkCmdPipelineBarrier(buffer.commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		vkCmdCopyBufferToImage(buffer.commandBuffer, buffer.buffer->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		buffer.descriptorSetsInUse.insert(descriptorSet);
	}
	else
	{
		vkCmdCopyBufferToImage(buffer.commandBuffer, buffer.buffer->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());
	}

	filled = true;
}

void SharedMemoryTexture::Delete(AppState& appState) const
{
	descriptorSets->referenceCount--;
	if (descriptorSets->referenceCount == 0)
	{
		vkDestroyDescriptorPool(appState.Device, descriptorSets->descriptorPool, nullptr);
	}
	if (view != VK_NULL_HANDLE)
	{
		vkDestroyImageView(appState.Device, view, nullptr);
	}
	if (image != VK_NULL_HANDLE)
	{
		vkDestroyImage(appState.Device, image, nullptr);
	}
	sharedMemory->referenceCount--;
	if (sharedMemory->referenceCount == 0)
	{
		vkFreeMemory(appState.Device, sharedMemory->memory, nullptr);
		delete sharedMemory;
	}
}
