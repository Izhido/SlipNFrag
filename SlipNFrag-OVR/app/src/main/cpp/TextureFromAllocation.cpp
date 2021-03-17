#include "TextureFromAllocation.h"
#include "AppState.h"
#include "VulkanCallWrappers.h"
#include "MemoryAllocateInfo.h"
#include "Constants.h"

std::vector<VkDescriptorSetLayout> TextureFromAllocation::descriptorSetLayouts;

void TextureFromAllocation::Create(AppState& appState, VkCommandBuffer commandBuffer, uint32_t width, uint32_t height, VkFormat format, uint32_t mipCount, VkImageUsageFlags usage)
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
	auto& allocationList = appState.Scene.allocations[memoryAllocateInfo.allocationSize];
	if (allocationList.blockSize == 0)
	{
		VkDeviceSize alignmentExcess = memoryAllocateInfo.allocationSize % memoryRequirements.alignment;
		VkDeviceSize alignmentCorrection = memoryRequirements.alignment - alignmentExcess;
		allocationList.blockSize = memoryAllocateInfo.allocationSize + alignmentCorrection;
	}
	for (auto j = 0; j < allocationList.allocations.size(); j++)
	{
		auto& allocation = allocationList.allocations[j];
		if (allocation.allocatedCount < allocation.allocated.size())
		{
			for (auto i = 0; i < allocation.allocated.size(); i++)
			{
				if (!allocation.allocated[i])
				{
					allocation.allocated[i] = true;
					VK(appState.Device.vkBindImageMemory(appState.Device.device, image, allocation.memory, i * allocationList.blockSize));
					this->allocationList = &allocationList;
					allocationIndex = j;
					allocatedIndex = i;
					allocation.allocatedCount++;
					descriptorSet = allocation.descriptorSets[i];
					break;
				}
			}
			break;
		}
	}
	if (descriptorSet == VK_NULL_HANDLE)
	{
		allocationList.allocations.emplace_back();
		auto& allocation = allocationList.allocations[allocationList.allocations.size() - 1];
		VkDeviceSize size = MEMORY_BLOCK_SIZE;
		if (size < allocationList.blockSize)
		{
			size = allocationList.blockSize;
		}
		auto memoryBlockAllocateInfo = memoryAllocateInfo;
		memoryBlockAllocateInfo.allocationSize = size;
		VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryBlockAllocateInfo, nullptr, &allocation.memory));
		auto count = size / allocationList.blockSize;
		allocation.allocated.resize(count);
		VkDescriptorPoolSize poolSizes { };
		poolSizes.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes.descriptorCount = count;
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { };
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.maxSets = count;
		descriptorPoolCreateInfo.pPoolSizes = &poolSizes;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &allocation.descriptorPool));
		descriptorSetLayouts.resize(count);
		allocation.descriptorSets.resize(count);
		std::fill(descriptorSetLayouts.begin(), descriptorSetLayouts.end(), appState.Scene.singleImageLayout);
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { };
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.descriptorPool = allocation.descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = count;
		descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts.data();
		VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, allocation.descriptorSets.data()));
		allocation.allocated[0] = true;
		VK(appState.Device.vkBindImageMemory(appState.Device.device, image, allocation.memory, 0));
		this->allocationList = &allocationList;
		allocationIndex = allocationList.allocations.size() - 1;
		allocation.allocatedCount++;
		descriptorSet = allocation.descriptorSets[0];
	}
	VkImageViewCreateInfo imageViewCreateInfo { };
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = imageCreateInfo.format;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.levelCount = mipCount;
	imageViewCreateInfo.subresourceRange.layerCount = layerCount;
	VK(appState.Device.vkCreateImageView(appState.Device.device, &imageViewCreateInfo, nullptr, &view));
	VkImageMemoryBarrier imageMemoryBarrier { };
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.levelCount = mipCount;
	imageMemoryBarrier.subresourceRange.layerCount = layerCount;
	VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	VkSamplerCreateInfo samplerCreateInfo { };
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.maxLod = mipCount;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	if (appState.Scene.samplers.size() <= mipCount)
	{
		appState.Scene.samplers.resize(mipCount + 1);
	}
	if (appState.Scene.samplers[mipCount] == VK_NULL_HANDLE)
	{
		VK(appState.Device.vkCreateSampler(appState.Device.device, &samplerCreateInfo, nullptr, &appState.Scene.samplers[mipCount]));
	}
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

void TextureFromAllocation::FillMipmapped(AppState& appState, Buffer* buffer, VkDeviceSize offset, VkCommandBuffer commandBuffer)
{
	VkImageMemoryBarrier imageMemoryBarrier { };
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	VkBufferImageCopy region { };
	region.bufferOffset = offset;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;
	VC(appState.Device.vkCmdCopyBufferToImage(commandBuffer, buffer->buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	auto width = this->width;
	auto height = this->height;
	VkImageBlit blit { };
	blit.srcOffsets[1].x = width;
	blit.srcOffsets[1].y = height;
	blit.srcOffsets[1].z = 1;
	blit.dstOffsets[1].z = 1;
	blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blit.srcSubresource.mipLevel = 0;
	blit.srcSubresource.layerCount = 1;
	for (auto i = 1; i < mipCount; i++)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.subresourceRange.baseMipLevel = i;
		VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
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
		blit.dstSubresource.layerCount = 1;
		VC(appState.Device.vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST));
	}
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	if (mipCount > 1)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 1;
		imageMemoryBarrier.subresourceRange.levelCount = mipCount - 1;
		VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	}
}

void TextureFromAllocation::Delete(AppState& appState)
{
	if (view != VK_NULL_HANDLE)
	{
		VC(appState.Device.vkDestroyImageView(appState.Device.device, view, nullptr));
	}
	if (image != VK_NULL_HANDLE)
	{
		VC(appState.Device.vkDestroyImage(appState.Device.device, image, nullptr));
	}
	auto& allocation = allocationList->allocations[allocationIndex];
	allocation.allocated[allocatedIndex] = false;
	allocation.allocatedCount--;
	if (allocation.allocatedCount == 0 && allocationIndex == allocationList->allocations.size() - 1)
	{
		auto index = allocationIndex;
		while (index > 0)
		{
			auto& toDelete = allocationList->allocations[index];
			if (toDelete.allocatedCount > 0)
			{
				break;
			}
			VC(appState.Device.vkDestroyDescriptorPool(appState.Device.device, toDelete.descriptorPool, nullptr));
			VC(appState.Device.vkFreeMemory(appState.Device.device, toDelete.memory, nullptr));
			allocationList->allocations.pop_back();
			index--;
		}
	}
}

void TextureFromAllocation::DeleteOld(AppState& appState, TextureFromAllocation** oldTextures)
{
	if (oldTextures != nullptr)
	{
		for (TextureFromAllocation** t = oldTextures; *t != nullptr; )
		{
			(*t)->unusedCount++;
			if ((*t)->unusedCount >= MAX_UNUSED_COUNT)
			{
				TextureFromAllocation* next = (*t)->next;
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
