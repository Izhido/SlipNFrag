#include "Texture.h"
#include "AppState.h"
#include "Utils.h"
#include "MemoryAllocateInfo.h"

void Texture::Create(AppState& appState, uint32_t width, uint32_t height, VkFormat format, uint32_t mipCount, VkImageUsageFlags usage)
{
	this->width = width;
	this->height = height;
	this->mipCount = mipCount;
	this->layerCount = 1;

	VkImageCreateInfo imageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent.width = this->width;
	imageCreateInfo.extent.height = this->height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = this->mipCount;
	imageCreateInfo.arrayLayers = this->layerCount;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.usage = usage;
	CHECK_VKCMD(vkCreateImage(appState.Device, &imageCreateInfo, nullptr, &image));

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(appState.Device, image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAllocateInfo { };
	properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	updateMemoryAllocateInfo(appState, memoryRequirements, properties, memoryAllocateInfo, true);

	CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &memory));
	CHECK_VKCMD(vkBindImageMemory(appState.Device, image, memory, 0));

	VkImageViewCreateInfo imageViewCreateInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = imageCreateInfo.format;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.levelCount = mipCount;
	imageViewCreateInfo.subresourceRange.layerCount = layerCount;
	CHECK_VKCMD(vkCreateImageView(appState.Device, &imageViewCreateInfo, nullptr, &view));
}

void Texture::Fill(AppState& appState, Buffer& buffer, VkDeviceSize offset, VkCommandBuffer commandBuffer)
{
	VkImageMemoryBarrier imageMemoryBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	if (filled)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.layerCount = layerCount;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

	VkBufferImageCopy region { };
	region.bufferOffset = offset;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = layerCount;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;
	vkCmdCopyBufferToImage(commandBuffer, buffer.buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

	filled = true;
}

void Texture::Fill(AppState& appState, StagingBuffer& buffer)
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
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = layerCount;
	vkCmdPipelineBarrier(buffer.commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	VkBufferImageCopy region { };
	region.bufferOffset = buffer.offset;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = layerCount;
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

void Texture::Delete(AppState& appState)
{
	filled = false;
	if (view != VK_NULL_HANDLE)
	{
		vkDestroyImageView(appState.Device, view, nullptr);
        view = VK_NULL_HANDLE;
	}
	if (image != VK_NULL_HANDLE)
	{
		vkDestroyImage(appState.Device, image, nullptr);
        image = VK_NULL_HANDLE;
	}
	if (memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(appState.Device, memory, nullptr);
        memory = VK_NULL_HANDLE;
	}
}
