#include "Skybox.h"
#include "AppState.h"
#include "Constants.h"
#include "Utils.h"

void Skybox::Initialize(AppState &appState, int width, int height, dskybox_t& skybox, uint32_t swapchainImageIndex)
{
	this->width = width;
	this->height = height;

	sources.resize(6);

	for (size_t i = 0; i < 6; i++)
	{
		sources[i] = skybox.textures[i].texture;
	}

	XrSwapchainCreateInfo swapchainCreateInfo { XR_TYPE_SWAPCHAIN_CREATE_INFO };
	swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
	swapchainCreateInfo.format = Constants::colorFormat;
	swapchainCreateInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
	swapchainCreateInfo.width = (uint32_t)width;
	swapchainCreateInfo.height = (uint32_t)height;
	swapchainCreateInfo.faceCount = 6;
	swapchainCreateInfo.arraySize = 1;
	swapchainCreateInfo.mipCount = 1;
	swapchainCreateInfo.createFlags = XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
	CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &swapchain));

	uint32_t skyboxImageCount;
	CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain, 0, &skyboxImageCount, nullptr));

	images.resize(skyboxImageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
	CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain, skyboxImageCount, &skyboxImageCount, (XrSwapchainImageBaseHeader*)images.data()));

	CHECK_XRCMD(xrAcquireSwapchainImage(swapchain, nullptr, &uploadSwapchainImageIndex));

	XrSwapchainImageWaitInfo waitInfo { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
	waitInfo.timeout = XR_INFINITE_DURATION;
	CHECK_XRCMD(xrWaitSwapchainImage(swapchain, &waitInfo));

	needsUpload = true;
	needsRelease = false;
}

void Skybox::Initialize(AppState &appState, int width, int height, dskybox_t& skybox)
{
	this->width = width;
	this->height = height;

	sources.resize(6);

	for (size_t i = 0; i < 6; i++)
	{
		sources[i] = skybox.textures[i].texture;
	}

	texture = new SharedMemoryTexture { };
	texture->Create(appState, width, height, Constants::colorFormat, 1, 6, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, true);

	needsUpload = true;
	needsRelease = false;
}

void Skybox::CopyPixels(AppState &appState, Buffer* stagingBuffer, dskybox_t& skybox)
{
	auto target = (uint32_t*)((byte*)stagingBuffer->mapped + stagingBufferOffset);

	if (swapchain != XR_NULL_HANDLE)
	{
		for (size_t i = 0; i < 6; i++)
		{
			std::string name;
			switch (i)
			{
				case 0:
					name = "bk";
					break;
				case 1:
					name = "ft";
					break;
				case 2:
					name = "up";
					break;
				case 3:
					name = "dn";
					break;
				case 4:
					name = "rt";
					break;
				default:
					name = "lf";
					break;
			}
			auto index = 0;
			while (index < 5)
			{
				if (name == std::string(skybox.textures[index].texture->name))
				{
					break;
				}
				index++;
			}
			auto source = (uint32_t*)(((byte*)skybox.textures[index].texture) + sizeof(texture_t) + width * height) + width - 1;
			auto y = 0;
			while (y < height)
			{
				auto x = 0;
				while (x < width)
				{
					*target++ = *source--;
					x++;
				}
				y++;
				source += width;
				source += width;
			}
		}
	}
	else
	{
		for (size_t i = 0; i < 6; i++)
		{
			std::string name;
			switch (i)
			{
				case 0:
					name = "bk";
					break;
				case 1:
					name = "ft";
					break;
				case 2:
					name = "up";
					break;
				case 3:
					name = "dn";
					break;
				case 4:
					name = "rt";
					break;
				default:
					name = "lf";
					break;
			}
			auto index = 0;
			while (index < 5)
			{
				if (name == std::string(skybox.textures[index].texture->name))
				{
					break;
				}
				index++;
			}
			auto source = (uint32_t*)(((byte*)skybox.textures[index].texture) + sizeof(texture_t) + width * height);
			auto size = width * height;
			memcpy(target, source, size * sizeof(uint32_t));
			target += size;
		}
	}
}

void Skybox::Upload(AppState &appState, VkCommandBuffer commandBuffer)
{
	VkBufferImageCopy region { };
	region.bufferOffset = stagingBufferOffset;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;

	if (swapchain != XR_NULL_HANDLE)
	{
		appState.CopyBarrier.image = images[uploadSwapchainImageIndex].image;
		appState.SubmitBarrier.image = appState.CopyBarrier.image;

		for (auto i = 0; i < 6; i++)
		{
			appState.CopyBarrier.subresourceRange.baseArrayLayer = i;
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.CopyBarrier);
			region.imageSubresource.baseArrayLayer = i;
			vkCmdCopyBufferToImage(commandBuffer, appState.Scene.stagingBuffer.buffer->buffer, images[uploadSwapchainImageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			region.bufferOffset += width * height * 4;
			appState.SubmitBarrier.subresourceRange.baseArrayLayer = i;
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.SubmitBarrier);
		}

		appState.CopyBarrier.subresourceRange.baseArrayLayer = 0;
		appState.SubmitBarrier.subresourceRange.baseArrayLayer = 0;

		needsRelease = true;
	}
	else
	{
		VkImageMemoryBarrier copyBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		copyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		copyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		copyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyBarrier.subresourceRange.levelCount = 1;
		copyBarrier.subresourceRange.layerCount = 1;
		copyBarrier.image = texture->image;

		VkImageMemoryBarrier submitBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		submitBarrier.srcAccessMask = copyBarrier.dstAccessMask;
		submitBarrier.oldLayout = copyBarrier.newLayout;
		submitBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		submitBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		submitBarrier.subresourceRange.aspectMask = copyBarrier.subresourceRange.aspectMask;
		submitBarrier.subresourceRange.levelCount = copyBarrier.subresourceRange.levelCount;
		submitBarrier.subresourceRange.layerCount = copyBarrier.subresourceRange.layerCount;
		submitBarrier.image = copyBarrier.image;

		for (auto i = 0; i < 6; i++)
		{
			copyBarrier.subresourceRange.baseArrayLayer = i;
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &copyBarrier);
			region.imageSubresource.baseArrayLayer = i;
			vkCmdCopyBufferToImage(commandBuffer, appState.Scene.stagingBuffer.buffer->buffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
			region.bufferOffset += width * height * 4;
			submitBarrier.subresourceRange.baseArrayLayer = i;
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &submitBarrier);
		}
	}

	needsUpload = false;
}

void Skybox::Delete(AppState& appState) const
{
	if (swapchain != XR_NULL_HANDLE)
	{
		xrDestroySwapchain(swapchain);
	}
	if (texture != nullptr)
	{
		texture->Delete(appState);
		delete texture;
	}
}

void Skybox::MoveToPrevious(Scene& scene)
{
	if (scene.skybox != nullptr)
	{
		scene.skybox->next = scene.previousSkyboxes;
		scene.previousSkyboxes = scene.skybox;
		scene.skybox = nullptr;
	}
}

void Skybox::DeleteOld(AppState& appState)
{
	for (Skybox** s = &appState.Scene.previousSkyboxes; *s != nullptr; )
	{
		(*s)->unusedCount++;
		if ((*s)->unusedCount >= Constants::framesToLive)
		{
			auto next = (*s)->next;
			(*s)->Delete(appState);
			delete *s;
			*s = next;
		}
		else
		{
			s = &(*s)->next;
		}
	}
	if (appState.Scene.skybox != nullptr)
	{
		appState.Scene.skybox->unusedCount = 0;
	}
}

void Skybox::DeleteAll(AppState& appState)
{
	MoveToPrevious(appState.Scene);
	for (Skybox** s = &appState.Scene.previousSkyboxes; *s != nullptr; )
	{
		auto next = (*s)->next;
		(*s)->Delete(appState);
		delete *s;
		*s = next;
	}
}
