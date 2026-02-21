#include "Skybox.h"
#include "AppState.h"
#include "Constants.h"
#include "Utils.h"

void Skybox::Create(AppState &appState, int width, int height, dskybox_t& skybox, uint32_t swapchainImageIndex)
{
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

	Buffer stagingBuffer;
	stagingBuffer.CreateStagingBuffer(appState, 6 * width * height * sizeof(uint32_t));

	stagingBuffer.Map(appState);

	auto target = (uint32_t*)stagingBuffer.mapped;

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

	stagingBuffer.UnmapAndFlush(appState);

	VkBufferImageCopy region { };
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = width;
	region.imageExtent.height = height;
	region.imageExtent.depth = 1;

	CHECK_XRCMD(xrAcquireSwapchainImage(swapchain, nullptr, &swapchainImageIndex));

    XrSwapchainImageWaitInfo waitInfo { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
    waitInfo.timeout = XR_INFINITE_DURATION;
	CHECK_XRCMD(xrWaitSwapchainImage(swapchain, &waitInfo));

	VkCommandBuffer setupCommandBuffer;

	VkCommandBufferAllocateInfo commandBufferAllocateInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	commandBufferAllocateInfo.commandBufferCount = 1;
	commandBufferAllocateInfo.commandPool = appState.SetupCommandPool;
	CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &setupCommandBuffer));

	VkCommandBufferBeginInfo commandBufferBeginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	CHECK_VKCMD(vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));

	appState.CopyBarrier.image = images[swapchainImageIndex].image;
	appState.SubmitBarrier.image = appState.CopyBarrier.image;

	for (auto i = 0; i < 6; i++)
	{
		appState.CopyBarrier.subresourceRange.baseArrayLayer = i;
		vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.CopyBarrier);
		region.imageSubresource.baseArrayLayer = i;
		vkCmdCopyBufferToImage(setupCommandBuffer, stagingBuffer.buffer, images[swapchainImageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		region.bufferOffset += width * height * 4;
		appState.SubmitBarrier.subresourceRange.baseArrayLayer = i;
		vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.SubmitBarrier);
	}

	appState.CopyBarrier.subresourceRange.baseArrayLayer = 0;
	appState.SubmitBarrier.subresourceRange.baseArrayLayer = 0;

	CHECK_VKCMD(vkEndCommandBuffer(setupCommandBuffer));

	VkSubmitInfo setupSubmitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	setupSubmitInfo.commandBufferCount = 1;
	setupSubmitInfo.pCommandBuffers = &setupCommandBuffer;
	CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));

	CHECK_VKCMD(vkQueueWaitIdle(appState.Queue));

	XrSwapchainImageReleaseInfo releaseInfo { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
	CHECK_XRCMD(xrReleaseSwapchainImage(swapchain, &releaseInfo));

	vkFreeCommandBuffers(appState.Device, appState.SetupCommandPool, 1, &setupCommandBuffer);

	stagingBuffer.Delete(appState);
}

void Skybox::Delete(AppState& appState) const
{
	if (swapchain != XR_NULL_HANDLE)
	{
		xrDestroySwapchain(swapchain);
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
		if ((*s)->unusedCount >= Constants::maxUnusedCount)
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
