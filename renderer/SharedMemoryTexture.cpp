#include "SharedMemoryTexture.h"
#include "AppState.h"
#include "Utils.h"
#include "Constants.h"

void SharedMemoryTexture::Create(AppState& appState, uint32_t width, uint32_t height, VkFormat format, uint32_t mipCount, uint32_t layerCount, VkImageUsageFlags usage)
{
	this->width = width;
	this->height = height;
	this->format = format;
	this->mipCount = mipCount;
	this->layerCount = layerCount;

	VkImageCreateInfo imageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = this->format;
	imageCreateInfo.extent.width = this->width;
	imageCreateInfo.extent.height = this->height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.mipLevels = this->mipCount;
	imageCreateInfo.arrayLayers = this->layerCount;
	imageCreateInfo.usage = usage;

	VmaAllocationCreateInfo allocInfo { };
	allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

	CHECK_VKCMD(vmaCreateImage(appState.Allocator, &imageCreateInfo, &allocInfo, &image, &allocation, nullptr));

	VkImageViewCreateInfo imageViewCreateInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = (layerCount > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D);
	imageViewCreateInfo.format = imageCreateInfo.format;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.levelCount = mipCount;
	imageViewCreateInfo.subresourceRange.layerCount = layerCount;
	CHECK_VKCMD(vkCreateImageView(appState.Device, &imageViewCreateInfo, nullptr, &view));

	if (appState.Scene.latestTextureDescriptorSets == nullptr || appState.Scene.latestTextureDescriptorSets->used >= appState.Scene.latestTextureDescriptorSets->descriptorSets.size())
	{
		if (appState.Scene.latestTextureDescriptorSets != nullptr)
		{
			appState.Scene.latestTextureDescriptorSets->referenceCount--;
			if (appState.Scene.latestTextureDescriptorSets->referenceCount == 0)
			{
				vkDestroyDescriptorPool(appState.Device, appState.Scene.latestTextureDescriptorSets->descriptorPool, nullptr);
				delete appState.Scene.latestTextureDescriptorSets;
			}
		}

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
	textureInfo.sampler = appState.Scene.sampler;
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

	auto offset = 0;
	auto mipWidth = width;
	auto mipHeight = height;
	auto componentCount = (format == VK_FORMAT_R8_UINT ? 1 : 4);
	auto previousMipWidth = mipWidth;
	auto previousMipHeight = mipHeight;
	std::vector<VkBufferImageCopy> regions(mips);
	for (auto i = 0; i < mips; i++)
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

		offset += (mipWidth * mipHeight * componentCount);

		previousMipWidth = mipWidth;
		previousMipHeight = mipHeight;
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
	vkCmdCopyBufferToImage(buffer.commandBuffer, buffer.buffer->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mips, regions.data());

	auto remaining = mipCount - mips;

	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	if (mips > 1 || remaining == 0)
	{
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.subresourceRange.levelCount = (remaining == 0 ? mips : mips - 1);
		vkCmdPipelineBarrier(buffer.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	if (remaining > 0)
	{
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.subresourceRange.baseMipLevel = mips - 1;
		barrier.subresourceRange.levelCount = 1;
		vkCmdPipelineBarrier(buffer.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkImageBlit blit { };
		blit.srcOffsets[1].z = 1;
		blit.dstOffsets[1].z = blit.srcOffsets[1].z;
		blit.srcSubresource.aspectMask = barrier.subresourceRange.aspectMask;
		blit.srcSubresource.baseArrayLayer = layer;
		blit.srcSubresource.layerCount = 1;
		blit.dstSubresource.aspectMask = blit.srcSubresource.aspectMask;
		blit.dstSubresource.baseArrayLayer = blit.srcSubresource.baseArrayLayer;
		blit.dstSubresource.layerCount = blit.srcSubresource.layerCount;

		barrier.subresourceRange.levelCount = 1;

		for (auto i = 0; i < remaining; i++)
		{
			blit.srcOffsets[1].x = previousMipWidth;
			blit.srcOffsets[1].y = previousMipHeight;
			blit.srcSubresource.mipLevel = mips + i - 1;
			blit.dstOffsets[1].x = mipWidth;
			blit.dstOffsets[1].y = mipHeight;
			blit.dstSubresource.mipLevel = blit.srcSubresource.mipLevel + 1;
			vkCmdBlitImage(buffer.commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);

			barrier.subresourceRange.baseMipLevel = blit.dstSubresource.mipLevel;
			vkCmdPipelineBarrier(buffer.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

			previousMipWidth = mipWidth;
			previousMipHeight = mipHeight;
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
		barrier.subresourceRange.baseMipLevel = mips - 1;
		barrier.subresourceRange.levelCount = remaining + 1;
		vkCmdPipelineBarrier(buffer.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	filled = true;
}

void SharedMemoryTexture::FillMipmapped(AppState& appState, StagingBuffer& buffer, int layer)
{
	std::vector<VkBufferImageCopy> regions(mipCount);
	auto offset = 0;
	auto mipWidth = width;
	auto mipHeight = height;
	auto componentCount = (format == VK_FORMAT_R8_UINT ? 1 : 4);
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
		offset += (mipWidth * mipHeight * componentCount);
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
		buffer.lastEndBarrier++;
		if (buffer.imageEndBarriers.size() <= buffer.lastEndBarrier)
		{
			buffer.imageEndBarriers.emplace_back();
		}

		auto& barrier = buffer.imageEndBarriers[buffer.lastEndBarrier];
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
		delete descriptorSets;
	}

	if (view != VK_NULL_HANDLE)
	{
		vkDestroyImageView(appState.Device, view, nullptr);
	}

	vmaDestroyImage(appState.Allocator, image, allocation);
}
