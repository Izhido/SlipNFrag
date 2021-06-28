#include "Scene.h"
#include "VrApi_Helpers.h"
#include "AppState.h"
#include "VrApi_Vulkan.h"
#include "VulkanCallWrappers.h"
#include <android/log.h>
#include "sys_ovr.h"
#include "MemoryAllocateInfo.h"
#include "ImageAsset.h"

void Scene::CopyImage(AppState& appState, unsigned char* source, uint32_t* target, int width, int height)
{
	for (auto y = 0; y < height; y++)
	{
		for (auto x = 0; x < width; x++)
		{
			auto r = *source++;
			auto g = *source++;
			auto b = *source++;
			auto a = *source++;
			auto factor = (double)a / 255;
			r = (unsigned char)((double)r * factor);
			g = (unsigned char)((double)g * factor);
			b = (unsigned char)((double)b * factor);
			*target++ = ((uint32_t)255 << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
		}
		target += appState.ScreenWidth - width;
	}
}

void Scene::AddBorder(AppState& appState, std::vector<uint32_t>& target)
{
	for (auto b = 0; b < 5; b++)
	{
		auto i = (unsigned char)(192.0 * sin(M_PI / (double)(b - 1)));
		auto color = ((uint32_t)255 << 24) | ((uint32_t)i << 16) | ((uint32_t)i << 8) | i;
		auto texTopIndex = b * appState.ScreenWidth + b;
		auto texBottomIndex = (appState.ScreenHeight - 1 - b) * appState.ScreenWidth + b;
		for (auto x = 0; x < appState.ScreenWidth - b - b; x++)
		{
			target[texTopIndex] = color;
			texTopIndex++;
			target[texBottomIndex] = color;
			texBottomIndex++;
		}
		auto texLeftIndex = (b + 1) * appState.ScreenWidth + b;
		auto texRightIndex = (b + 1) * appState.ScreenWidth + appState.ScreenWidth - 1 - b;
		for (auto y = 0; y < appState.ScreenHeight - b - 1 - b - 1; y++)
		{
			target[texLeftIndex] = color;
			texLeftIndex += appState.ScreenWidth;
			target[texRightIndex] = color;
			texRightIndex += appState.ScreenWidth;
		}
	}
}

void Scene::Create(AppState& appState, VkCommandBufferAllocateInfo& commandBufferAllocateInfo, VkCommandBuffer& setupCommandBuffer, VkCommandBufferBeginInfo& commandBufferBeginInfo, VkSubmitInfo& setupSubmitInfo, Instance& instance, VkFenceCreateInfo& fenceCreateInfo, struct android_app* app)
{
	vrapi_WaitFrame(appState.Ovr, appState.FrameIndex);
	vrapi_BeginFrame(appState.Ovr, appState.FrameIndex);
	int frameFlags = 0;
	frameFlags |= VRAPI_FRAME_FLAG_FLUSH;
	ovrLayerProjection2 blackLayer = vrapi_DefaultLayerBlackProjection2();
	blackLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;
	ovrLayerLoadingIcon2 iconLayer = vrapi_DefaultLayerLoadingIcon2();
	iconLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;
	const ovrLayerHeader2* layers[] { &blackLayer.Header, &iconLayer.Header };
	ovrSubmitFrameDescription2 frameDesc { };
	frameDesc.Flags = frameFlags;
	frameDesc.SwapInterval = 1;
	frameDesc.FrameIndex = appState.FrameIndex;
	frameDesc.DisplayTime = appState.DisplayTime;
	frameDesc.LayerCount = 2;
	frameDesc.Layers = layers;
	vrapi_SubmitFrame2(appState.Ovr, &frameDesc);
	appState.ScreenWidth = 960;
	appState.ScreenHeight = 600;
	appState.ConsoleWidth = 320;
	appState.ConsoleHeight = 200;
	appState.Console.SwapChain = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, appState.ScreenWidth, appState.ScreenHeight, 1, 3);
	appState.Console.View.colorSwapChain.SwapChain = appState.Console.SwapChain;
	appState.Console.View.colorSwapChain.SwapChainLength = vrapi_GetTextureSwapChainLength(appState.Console.View.colorSwapChain.SwapChain);
	appState.Console.View.colorSwapChain.ColorTextures.resize(appState.Console.View.colorSwapChain.SwapChainLength);
	for (auto i = 0; i < appState.Console.View.colorSwapChain.SwapChainLength; i++)
	{
		appState.Console.View.colorSwapChain.ColorTextures[i] = vrapi_GetTextureSwapChainBufferVulkan(appState.Console.View.colorSwapChain.SwapChain, i);
	}
	uint32_t attachmentCount = 0;
	VkAttachmentDescription attachments[2] { };
	attachments[attachmentCount].format = VK_FORMAT_R8G8B8A8_UNORM;
	attachments[attachmentCount].samples = VK_SAMPLE_COUNT_4_BIT;
	attachments[attachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[attachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[attachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[attachmentCount].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[attachmentCount].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachmentCount++;
	attachments[attachmentCount].format = VK_FORMAT_R8G8B8A8_UNORM;
	attachments[attachmentCount].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[attachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[attachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[attachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[attachmentCount].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[attachmentCount].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachmentCount++;
	VkAttachmentReference colorAttachmentReference;
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference resolveAttachmentReference;
	resolveAttachmentReference.attachment = 1;
	resolveAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkSubpassDescription subpassDescription { };
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;
	subpassDescription.pResolveAttachments = &resolveAttachmentReference;
	VkRenderPassCreateInfo renderPassCreateInfo { };
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = attachmentCount;
	renderPassCreateInfo.pAttachments = attachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	VK(appState.Device.vkCreateRenderPass(appState.Device.device, &renderPassCreateInfo, nullptr, &appState.Console.RenderPass));
	appState.Console.View.framebuffer.colorTextureSwapChain = appState.Console.View.colorSwapChain.SwapChain;
	appState.Console.View.framebuffer.swapChainLength = appState.Console.View.colorSwapChain.SwapChainLength;
	appState.Console.View.framebuffer.colorTextures.resize(appState.Console.View.framebuffer.swapChainLength);
	appState.Console.View.framebuffer.startBarriers.resize(appState.Console.View.framebuffer.swapChainLength);
	appState.Console.View.framebuffer.endBarriers.resize(appState.Console.View.framebuffer.swapChainLength);
	appState.Console.View.framebuffer.framebuffers.resize(appState.Console.View.framebuffer.swapChainLength);
	appState.Console.View.framebuffer.width = appState.ScreenWidth;
	appState.Console.View.framebuffer.height = appState.ScreenHeight;
	VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &setupCommandBuffer));
	VK(appState.Device.vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
	for (auto i = 0; i < appState.Console.View.framebuffer.swapChainLength; i++)
	{
		auto& texture = appState.Console.View.framebuffer.colorTextures[i];
		texture.width = appState.ScreenWidth;
		texture.height = appState.ScreenHeight;
		texture.layerCount = 1;
		texture.image = appState.Console.View.colorSwapChain.ColorTextures[i];
		VkImageMemoryBarrier imageMemoryBarrier { };
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier.image = appState.Console.View.colorSwapChain.ColorTextures[i];
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.layerCount = texture.layerCount;
		VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
		VkImageViewCreateInfo imageViewCreateInfo { };
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = texture.image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.layerCount = texture.layerCount;
		VK(appState.Device.vkCreateImageView(appState.Device.device, &imageViewCreateInfo, nullptr, &texture.view));
		auto& startBarrier = appState.Console.View.framebuffer.startBarriers[i];
		startBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		startBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		startBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		startBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		startBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		startBarrier.image = texture.image;
		startBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		startBarrier.subresourceRange.levelCount = 1;
		startBarrier.subresourceRange.layerCount = texture.layerCount;
		auto& endBarrier = appState.Console.View.framebuffer.endBarriers[i];
		endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		endBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		endBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		endBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		endBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		endBarrier.image = texture.image;
		endBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		endBarrier.subresourceRange.levelCount = 1;
		endBarrier.subresourceRange.layerCount = texture.layerCount;
	}
	VkFormatProperties props;
	VC(instance.vkGetPhysicalDeviceFormatProperties(appState.Device.physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &props));
	if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Scene::Create(): Color attachment bit in texture is not defined.");
		vrapi_Shutdown();
		exit(0);
	}
	auto& texture = appState.Console.View.framebuffer.renderTexture;
	texture.width = appState.Console.View.framebuffer.width;
	texture.height = appState.Console.View.framebuffer.height;
	texture.layerCount = 1;
	VkImageCreateInfo imageCreateInfo { };
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.extent.width = texture.width;
	imageCreateInfo.extent.height = texture.height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = texture.layerCount;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_4_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK(appState.Device.vkCreateImage(appState.Device.device, &imageCreateInfo, nullptr, &texture.image));
	VkMemoryRequirements memoryRequirements;
	VC(appState.Device.vkGetImageMemoryRequirements(appState.Device.device, texture.image, &memoryRequirements));
	VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	if (appState.Device.supportsLazyAllocate)
	{
		memFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
	}
	VkMemoryAllocateInfo memoryAllocateInfo { };
	createMemoryAllocateInfo(appState, memoryRequirements, memFlags, memoryAllocateInfo);
	VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryAllocateInfo, nullptr, &texture.memory));
	VK(appState.Device.vkBindImageMemory(appState.Device.device, texture.image, texture.memory, 0));
	VkImageMemoryBarrier imageMemoryBarrier { };
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageMemoryBarrier.image = texture.image;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.layerCount = texture.layerCount;
	VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	VkImageViewCreateInfo imageViewCreateInfo { };
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = texture.image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.layerCount = texture.layerCount;
	VK(appState.Device.vkCreateImageView(appState.Device.device, &imageViewCreateInfo, nullptr, &texture.view));
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imageMemoryBarrier.image = texture.image;
	imageMemoryBarrier.subresourceRange.layerCount = texture.layerCount;
	VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	for (auto i = 0; i < appState.Console.View.framebuffer.swapChainLength; i++)
	{
		uint32_t attachmentCount = 0;
		VkImageView attachments[2];
		attachments[attachmentCount++] = appState.Console.View.framebuffer.renderTexture.view;
		attachments[attachmentCount++] = appState.Console.View.framebuffer.colorTextures[i].view;
		VkFramebufferCreateInfo framebufferCreateInfo { };
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = appState.Console.RenderPass;
		framebufferCreateInfo.attachmentCount = attachmentCount;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = appState.Console.View.framebuffer.width;
		framebufferCreateInfo.height = appState.Console.View.framebuffer.height;
		framebufferCreateInfo.layers = 1;
		VK(appState.Device.vkCreateFramebuffer(appState.Device.device, &framebufferCreateInfo, nullptr, &appState.Console.View.framebuffer.framebuffers[i]));
	}
	appState.Console.View.perImage.resize(appState.Console.View.framebuffer.swapChainLength);
	for (auto& perImage : appState.Console.View.perImage)
	{
		VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &perImage.commandBuffer));
		VK(appState.Device.vkCreateFence(appState.Device.device, &fenceCreateInfo, nullptr, &perImage.fence));
	}
	appState.Screen.SwapChain = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, appState.ScreenWidth, appState.ScreenHeight, 1, 1);
	appState.Screen.Data.resize(appState.ScreenWidth * appState.ScreenHeight, 255 << 24);
	appState.Screen.Image = vrapi_GetTextureSwapChainBufferVulkan(appState.Screen.SwapChain, 0);
	ImageAsset play;
	play.Open("play.png", app);
	CopyImage(appState, play.image, appState.Screen.Data.data() + ((appState.ScreenHeight - play.height) * appState.ScreenWidth + appState.ScreenWidth - play.width) / 2, play.width, play.height);
	play.Close();
	AddBorder(appState, appState.Screen.Data);
	appState.Screen.Buffer.CreateStagingBuffer(appState, appState.Screen.Data.size() * sizeof(uint32_t));
	VK(appState.Device.vkMapMemory(appState.Device.device, appState.Screen.Buffer.memory, 0, VK_WHOLE_SIZE, 0, &appState.Screen.Buffer.mapped));
	memcpy(appState.Screen.Buffer.mapped, appState.Screen.Data.data(), appState.Screen.Buffer.size);
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.image = appState.Screen.Image;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	VkBufferImageCopy region { };
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = appState.ScreenWidth;
	region.imageExtent.height = appState.ScreenHeight;
	region.imageExtent.depth = 1;
	VC(appState.Device.vkCmdCopyBufferToImage(setupCommandBuffer, appState.Screen.Buffer.buffer, appState.Screen.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
	VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &appState.Screen.CommandBuffer));
	appState.Screen.SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	appState.Screen.SubmitInfo.commandBufferCount = 1;
	appState.Screen.SubmitInfo.pCommandBuffers = &appState.Screen.CommandBuffer;
	ImageAsset floorImage;
	floorImage.Open("floor.png", app);
	appState.Scene.floorTexture.Create(appState, floorImage.width, floorImage.height, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	ImageAsset controller;
	controller.Open("controller.png", app);
	appState.Scene.controllerTexture.Create(appState, controller.width, controller.height, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	VkDeviceSize stagingBufferSize = (floorImage.width * floorImage.height + controller.width * controller.height + 2 * appState.ScreenWidth * appState.ScreenHeight) * sizeof(uint32_t);
	Buffer stagingBuffer;
	stagingBuffer.CreateStagingBuffer(appState, stagingBufferSize);
	VK(appState.Device.vkMapMemory(appState.Device.device, stagingBuffer.memory, 0, VK_WHOLE_SIZE, 0, &stagingBuffer.mapped));
	memcpy(stagingBuffer.mapped, floorImage.image, floorImage.width * floorImage.height * sizeof(uint32_t));
	floorImage.Close();
	size_t offset = floorImage.width * floorImage.height;
	memcpy((uint32_t*)stagingBuffer.mapped + offset, controller.image, controller.width * controller.height * sizeof(uint32_t));
	offset += controller.width * controller.height;
	ImageAsset leftArrows;
	leftArrows.Open("leftarrows.png", app);
	appState.LeftArrows.SwapChain = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, appState.ScreenWidth, appState.ScreenHeight, 1, 1);
	appState.LeftArrows.Image = vrapi_GetTextureSwapChainBufferVulkan(appState.LeftArrows.SwapChain, 0);
	CopyImage(appState, leftArrows.image, (uint32_t*)stagingBuffer.mapped + offset + ((appState.ScreenHeight - leftArrows.height) * appState.ScreenWidth + appState.ScreenWidth - leftArrows.width) / 2, leftArrows.width, leftArrows.height);
	leftArrows.Close();
	offset += appState.ScreenWidth * appState.ScreenHeight;
	ImageAsset rightArrows;
	rightArrows.Open("rightarrows.png", app);
	appState.RightArrows.SwapChain = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, appState.ScreenWidth, appState.ScreenHeight, 1, 1);
	appState.RightArrows.Image = vrapi_GetTextureSwapChainBufferVulkan(appState.RightArrows.SwapChain, 0);
	CopyImage(appState, rightArrows.image, (uint32_t*)stagingBuffer.mapped + offset + ((appState.ScreenHeight - rightArrows.height) * appState.ScreenWidth + appState.ScreenWidth - rightArrows.width) / 2, rightArrows.width, rightArrows.height);
	rightArrows.Close();
	offset = 0;
	appState.Scene.floorTexture.Fill(appState, &stagingBuffer, offset, setupCommandBuffer);
	offset += floorImage.width * floorImage.height * sizeof(uint32_t);
	appState.Scene.controllerTexture.Fill(appState, &stagingBuffer, offset, setupCommandBuffer);
	offset += controller.width * controller.height * sizeof(uint32_t);
	imageMemoryBarrier.image = appState.LeftArrows.Image;
	VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	region.bufferOffset = offset;
	VC(appState.Device.vkCmdCopyBufferToImage(setupCommandBuffer, stagingBuffer.buffer, appState.LeftArrows.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
	offset += appState.ScreenWidth * appState.ScreenHeight * sizeof(uint32_t);
	imageMemoryBarrier.image = appState.RightArrows.Image;
	VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	region.bufferOffset = offset;
	VC(appState.Device.vkCmdCopyBufferToImage(setupCommandBuffer, stagingBuffer.buffer, appState.RightArrows.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
	VK(appState.Device.vkEndCommandBuffer(setupCommandBuffer));
	VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
	VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
	VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &setupCommandBuffer));
	stagingBuffer.Destroy(appState);
	auto consoleBottom = (float)(appState.ConsoleHeight - SBAR_HEIGHT - 24) / appState.ConsoleHeight;
	auto statusBarTop = (float)(appState.ScreenHeight - SBAR_HEIGHT - 24) / appState.ScreenHeight;
	appState.ConsoleVertices.assign(
	{
		-1, consoleBottom * 2 - 1, 0, 0, consoleBottom,
		1, consoleBottom * 2 - 1, 0, 1, consoleBottom,
		1, -1, 0, 1, 0,
		-1, -1, 0, 0, 0,
		-1, 1, 0, 0, 1,
		-1.0 / 3.0, 1, 0, 1, 1,
		-1.0 / 3.0, statusBarTop * 2 - 1, 0, 1, consoleBottom,
		-1, statusBarTop * 2 - 1, 0, 0, consoleBottom,
		-1, 1, 0, 0, 1,
		1, 1, 0, 1, 1,
		1, 0, 0, 1, 0,
		-1, 0, 0, 0, 0
	});
	appState.ConsoleIndices.assign(
	{
		0, 1, 2,
		2, 3, 0,
		4, 5, 6,
		6, 7, 4,
		8, 9, 10,
		10, 11, 8
	});
	appState.NoGameDataData.resize(appState.ScreenWidth * appState.ScreenHeight, 255 << 24);
	ImageAsset noGameData;
	noGameData.Open("nogamedata.png", app);
	CopyImage(appState, noGameData.image, appState.NoGameDataData.data() + ((appState.ScreenHeight - noGameData.height) * appState.ScreenWidth + appState.ScreenWidth - noGameData.width) / 2, noGameData.width, noGameData.height);
	noGameData.Close();
	AddBorder(appState, appState.NoGameDataData);
	auto isMultiview = appState.Device.supportsMultiview;
	numBuffers = (isMultiview) ? VRAPI_FRAME_LAYER_EYE_MAX : 1;
	CreateShader(appState, app, (isMultiview ? "shaders/surface_multiview.vert.spv" : "shaders/surface.vert.spv"), &surfaceVertex);
	CreateShader(appState, app, "shaders/surface.frag.spv", &surfaceFragment);
	CreateShader(appState, app, "shaders/fence.frag.spv", &fenceFragment);
	CreateShader(appState, app, (isMultiview ? "shaders/sprite_multiview.vert.spv" : "shaders/sprite.vert.spv"), &spriteVertex);
	CreateShader(appState, app, "shaders/sprite.frag.spv", &spriteFragment);
	CreateShader(appState, app, (isMultiview ? "shaders/turbulent_multiview.vert.spv" : "shaders/turbulent.vert.spv"), &turbulentVertex);
	CreateShader(appState, app, "shaders/turbulent.frag.spv", &turbulentFragment);
	CreateShader(appState, app, (isMultiview ? "shaders/alias_multiview.vert.spv" : "shaders/alias.vert.spv"), &aliasVertex);
	CreateShader(appState, app, "shaders/alias.frag.spv", &aliasFragment);
	CreateShader(appState, app, (isMultiview ? "shaders/viewmodel_multiview.vert.spv" : "shaders/viewmodel.vert.spv"), &viewmodelVertex);
	CreateShader(appState, app, "shaders/viewmodel.frag.spv", &viewmodelFragment);
	CreateShader(appState, app, (isMultiview ? "shaders/colored_multiview.vert.spv" : "shaders/colored.vert.spv"), &coloredVertex);
	CreateShader(appState, app, "shaders/colored.frag.spv", &coloredFragment);
	CreateShader(appState, app, (isMultiview ? "shaders/sky_multiview.vert.spv" : "shaders/sky.vert.spv"), &skyVertex);
	CreateShader(appState, app, "shaders/sky.frag.spv", &skyFragment);
	CreateShader(appState, app, (isMultiview ? "shaders/floor_multiview.vert.spv" : "shaders/floor.vert.spv"), &floorVertex);
	CreateShader(appState, app, "shaders/floor.frag.spv", &floorFragment);
	CreateShader(appState, app, "shaders/console.vert.spv", &consoleVertex);
	CreateShader(appState, app, "shaders/console.frag.spv", &consoleFragment);
	VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo { };
	tessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo { };
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.scissorCount = 1;
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo { };
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo { };
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
	multisampleStateCreateInfo.minSampleShading = 1.0f;
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo { };
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateCreateInfo.front.failOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.front.passOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.front.depthFailOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.front.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilStateCreateInfo.back.failOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.back.passOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.back.depthFailOp = VK_STENCIL_OP_KEEP;
	depthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState { };
	colorBlendAttachmentState.blendEnable = VK_TRUE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo { };
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_CLEAR;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
	VkDynamicState dynamicStateEnables[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo { };
	pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineDynamicStateCreateInfo.dynamicStateCount = 2;
	pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStateEnables;
	surfaceAttributes.vertexAttributes.resize(5);
	surfaceAttributes.vertexBindings.resize(2);
	surfaceAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	surfaceAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	surfaceAttributes.vertexAttributes[1].location = 1;
	surfaceAttributes.vertexAttributes[1].binding = 1;
	surfaceAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceAttributes.vertexAttributes[2].location = 2;
	surfaceAttributes.vertexAttributes[2].binding = 1;
	surfaceAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceAttributes.vertexAttributes[2].offset = 4 * sizeof(float);
	surfaceAttributes.vertexAttributes[3].location = 3;
	surfaceAttributes.vertexAttributes[3].binding = 1;
	surfaceAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceAttributes.vertexAttributes[3].offset = 8 * sizeof(float);
	surfaceAttributes.vertexAttributes[4].location = 4;
	surfaceAttributes.vertexAttributes[4].binding = 1;
	surfaceAttributes.vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceAttributes.vertexAttributes[4].offset = 12 * sizeof(float);
	surfaceAttributes.vertexBindings[1].binding = 1;
	surfaceAttributes.vertexBindings[1].stride = 16 * sizeof(float);
	surfaceAttributes.vertexBindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
	surfaceAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	surfaceAttributes.vertexInputState.vertexBindingDescriptionCount = surfaceAttributes.vertexBindings.size();
	surfaceAttributes.vertexInputState.pVertexBindingDescriptions = surfaceAttributes.vertexBindings.data();
	surfaceAttributes.vertexInputState.vertexAttributeDescriptionCount = surfaceAttributes.vertexAttributes.size();
	surfaceAttributes.vertexInputState.pVertexAttributeDescriptions = surfaceAttributes.vertexAttributes.data();
	surfaceAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	surfaceAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	spriteAttributes.vertexAttributes.resize(6);
	spriteAttributes.vertexBindings.resize(3);
	spriteAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	spriteAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	spriteAttributes.vertexAttributes[1].location = 1;
	spriteAttributes.vertexAttributes[1].binding = 1;
	spriteAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	spriteAttributes.vertexBindings[1].binding = 1;
	spriteAttributes.vertexBindings[1].stride = 2 * sizeof(float);
	spriteAttributes.vertexAttributes[2].location = 2;
	spriteAttributes.vertexAttributes[2].binding = 2;
	spriteAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	spriteAttributes.vertexAttributes[3].location = 3;
	spriteAttributes.vertexAttributes[3].binding = 2;
	spriteAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	spriteAttributes.vertexAttributes[3].offset = 4 * sizeof(float);
	spriteAttributes.vertexAttributes[4].location = 4;
	spriteAttributes.vertexAttributes[4].binding = 2;
	spriteAttributes.vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	spriteAttributes.vertexAttributes[4].offset = 8 * sizeof(float);
	spriteAttributes.vertexAttributes[5].location = 5;
	spriteAttributes.vertexAttributes[5].binding = 2;
	spriteAttributes.vertexAttributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	spriteAttributes.vertexAttributes[5].offset = 12 * sizeof(float);
	spriteAttributes.vertexBindings[2].binding = 2;
	spriteAttributes.vertexBindings[2].stride = 16 * sizeof(float);
	spriteAttributes.vertexBindings[2].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
	spriteAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	spriteAttributes.vertexInputState.vertexBindingDescriptionCount = spriteAttributes.vertexBindings.size();
	spriteAttributes.vertexInputState.pVertexBindingDescriptions = spriteAttributes.vertexBindings.data();
	spriteAttributes.vertexInputState.vertexAttributeDescriptionCount = spriteAttributes.vertexAttributes.size();
	spriteAttributes.vertexInputState.pVertexAttributeDescriptions = spriteAttributes.vertexAttributes.data();
	spriteAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	spriteAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	colormappedAttributes.vertexAttributes.resize(7);
	colormappedAttributes.vertexBindings.resize(4);
	colormappedAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	colormappedAttributes.vertexBindings[0].stride = 4 * sizeof(float);
	colormappedAttributes.vertexAttributes[1].location = 1;
	colormappedAttributes.vertexAttributes[1].binding = 1;
	colormappedAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	colormappedAttributes.vertexBindings[1].binding = 1;
	colormappedAttributes.vertexBindings[1].stride = 2 * sizeof(float);
	colormappedAttributes.vertexAttributes[2].location = 2;
	colormappedAttributes.vertexAttributes[2].binding = 2;
	colormappedAttributes.vertexAttributes[2].format = VK_FORMAT_R32_SFLOAT;
	colormappedAttributes.vertexBindings[2].binding = 2;
	colormappedAttributes.vertexBindings[2].stride = sizeof(float);
	colormappedAttributes.vertexAttributes[3].location = 3;
	colormappedAttributes.vertexAttributes[3].binding = 3;
	colormappedAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	colormappedAttributes.vertexAttributes[4].location = 4;
	colormappedAttributes.vertexAttributes[4].binding = 3;
	colormappedAttributes.vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	colormappedAttributes.vertexAttributes[4].offset = 4 * sizeof(float);
	colormappedAttributes.vertexAttributes[5].location = 5;
	colormappedAttributes.vertexAttributes[5].binding = 3;
	colormappedAttributes.vertexAttributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	colormappedAttributes.vertexAttributes[5].offset = 8 * sizeof(float);
	colormappedAttributes.vertexAttributes[6].location = 6;
	colormappedAttributes.vertexAttributes[6].binding = 3;
	colormappedAttributes.vertexAttributes[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	colormappedAttributes.vertexAttributes[6].offset = 12 * sizeof(float);
	colormappedAttributes.vertexBindings[3].binding = 3;
	colormappedAttributes.vertexBindings[3].stride = 16 * sizeof(float);
	colormappedAttributes.vertexBindings[3].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
	colormappedAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	colormappedAttributes.vertexInputState.vertexBindingDescriptionCount = colormappedAttributes.vertexBindings.size();
	colormappedAttributes.vertexInputState.pVertexBindingDescriptions = colormappedAttributes.vertexBindings.data();
	colormappedAttributes.vertexInputState.vertexAttributeDescriptionCount = colormappedAttributes.vertexAttributes.size();
	colormappedAttributes.vertexInputState.pVertexAttributeDescriptions = colormappedAttributes.vertexAttributes.data();
	colormappedAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	colormappedAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	coloredAttributes.vertexAttributes.resize(6);
	coloredAttributes.vertexBindings.resize(3);
	coloredAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	coloredAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	coloredAttributes.vertexAttributes[1].location = 1;
	coloredAttributes.vertexAttributes[1].binding = 1;
	coloredAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	coloredAttributes.vertexAttributes[2].location = 2;
	coloredAttributes.vertexAttributes[2].binding = 1;
	coloredAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	coloredAttributes.vertexAttributes[2].offset = 4 * sizeof(float);
	coloredAttributes.vertexAttributes[3].location = 3;
	coloredAttributes.vertexAttributes[3].binding = 1;
	coloredAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	coloredAttributes.vertexAttributes[3].offset = 8 * sizeof(float);
	coloredAttributes.vertexAttributes[4].location = 4;
	coloredAttributes.vertexAttributes[4].binding = 1;
	coloredAttributes.vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	coloredAttributes.vertexAttributes[4].offset = 12 * sizeof(float);
	coloredAttributes.vertexBindings[1].binding = 1;
	coloredAttributes.vertexBindings[1].stride = 16 * sizeof(float);
	coloredAttributes.vertexBindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
	coloredAttributes.vertexAttributes[5].location = 5;
	coloredAttributes.vertexAttributes[5].binding = 2;
	coloredAttributes.vertexAttributes[5].format = VK_FORMAT_R32_SFLOAT;
	coloredAttributes.vertexBindings[2].binding = 2;
	coloredAttributes.vertexBindings[2].stride = sizeof(float);
	coloredAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	coloredAttributes.vertexInputState.vertexBindingDescriptionCount = coloredAttributes.vertexBindings.size();
	coloredAttributes.vertexInputState.pVertexBindingDescriptions = coloredAttributes.vertexBindings.data();
	coloredAttributes.vertexInputState.vertexAttributeDescriptionCount = coloredAttributes.vertexAttributes.size();
	coloredAttributes.vertexInputState.pVertexAttributeDescriptions = coloredAttributes.vertexAttributes.data();
	coloredAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	coloredAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	skyAttributes.vertexAttributes.resize(2);
	skyAttributes.vertexBindings.resize(2);
	skyAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	skyAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	skyAttributes.vertexAttributes[1].location = 1;
	skyAttributes.vertexAttributes[1].binding = 1;
	skyAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	skyAttributes.vertexBindings[1].binding = 1;
	skyAttributes.vertexBindings[1].stride = 2 * sizeof(float);
	skyAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	skyAttributes.vertexInputState.vertexBindingDescriptionCount = skyAttributes.vertexBindings.size();
	skyAttributes.vertexInputState.pVertexBindingDescriptions = skyAttributes.vertexBindings.data();
	skyAttributes.vertexInputState.vertexAttributeDescriptionCount = skyAttributes.vertexAttributes.size();
	skyAttributes.vertexInputState.pVertexAttributeDescriptions = skyAttributes.vertexAttributes.data();
	skyAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	skyAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	floorAttributes.vertexAttributes.resize(2);
	floorAttributes.vertexBindings.resize(2);
	floorAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	floorAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	floorAttributes.vertexAttributes[1].location = 1;
	floorAttributes.vertexAttributes[1].binding = 1;
	floorAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	floorAttributes.vertexBindings[1].binding = 1;
	floorAttributes.vertexBindings[1].stride = 2 * sizeof(float);
	floorAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	floorAttributes.vertexInputState.vertexBindingDescriptionCount = floorAttributes.vertexBindings.size();
	floorAttributes.vertexInputState.pVertexBindingDescriptions = floorAttributes.vertexBindings.data();
	floorAttributes.vertexInputState.vertexAttributeDescriptionCount = floorAttributes.vertexAttributes.size();
	floorAttributes.vertexInputState.pVertexAttributeDescriptions = floorAttributes.vertexAttributes.data();
	floorAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	floorAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	consoleAttributes.vertexAttributes.resize(2);
	consoleAttributes.vertexBindings.resize(2);
	consoleAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	consoleAttributes.vertexBindings[0].stride = 5 * sizeof(float);
	consoleAttributes.vertexAttributes[1].location = 1;
	consoleAttributes.vertexAttributes[1].binding = 1;
	consoleAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	consoleAttributes.vertexAttributes[1].offset = 3 * sizeof(float);
	consoleAttributes.vertexBindings[1].binding = 1;
	consoleAttributes.vertexBindings[1].stride = 5 * sizeof(float);
	consoleAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	consoleAttributes.vertexInputState.vertexBindingDescriptionCount = consoleAttributes.vertexBindings.size();
	consoleAttributes.vertexInputState.pVertexBindingDescriptions = consoleAttributes.vertexBindings.data();
	consoleAttributes.vertexInputState.vertexAttributeDescriptionCount = consoleAttributes.vertexAttributes.size();
	consoleAttributes.vertexInputState.pVertexAttributeDescriptions = consoleAttributes.vertexAttributes.data();
	consoleAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	consoleAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	surfaces.stages.resize(2);
	surfaces.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	surfaces.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	surfaces.stages[0].module = surfaceVertex;
	surfaces.stages[0].pName = "main";
	surfaces.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	surfaces.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	surfaces.stages[1].module = surfaceFragment;
	surfaces.stages[1].pName = "main";
	VkDescriptorSetLayoutBinding descriptorSetBindings[3] { };
	descriptorSetBindings[1].binding = 1;
	descriptorSetBindings[2].binding = 2;
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo { };
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetBindings;
	descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetBindings[0].descriptorCount = 1;
	descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &singleBufferLayout));
	descriptorSetBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetBindings[1].descriptorCount = 1;
	descriptorSetBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutCreateInfo.bindingCount = 2;
	VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &bufferAndImageLayout));
	descriptorSetBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetBindings[2].descriptorCount = 1;
	descriptorSetBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutCreateInfo.bindingCount = 3;
	VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &bufferAndTwoImagesLayout));
	descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetBindings[0].descriptorCount = 1;
	descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &singleImageLayout));
	descriptorSetBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetBindings[1].descriptorCount = 1;
	descriptorSetBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutCreateInfo.bindingCount = 2;
	VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &doubleImageLayout));
	VkDescriptorSetLayout descriptorSetLayouts[3] { };
	descriptorSetLayouts[0] = bufferAndTwoImagesLayout;
	descriptorSetLayouts[1] = singleImageLayout;
	descriptorSetLayouts[2] = singleImageLayout;
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo { };
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 3;
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
	VkPushConstantRange pushConstantInfo { };
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantInfo.size = 20 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &surfaces.pipelineLayout));
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo { };
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = surfaces.stages.size();
	graphicsPipelineCreateInfo.pStages = surfaces.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &surfaceAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.pTessellationState = &tessellationStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = surfaces.pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = appState.RenderPass;
	VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfaces.pipeline));
	fences.stages.resize(2);
	fences.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fences.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	fences.stages[0].module = surfaceVertex;
	fences.stages[0].pName = "main";
	fences.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fences.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fences.stages[1].module = fenceFragment;
	fences.stages[1].pName = "main";
	VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &fences.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = fences.stages.size();
	graphicsPipelineCreateInfo.pStages = fences.stages.data();
	graphicsPipelineCreateInfo.layout = fences.pipelineLayout;
	VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fences.pipeline));
	sprites.stages.resize(2);
	sprites.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	sprites.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	sprites.stages[0].module = spriteVertex;
	sprites.stages[0].pName = "main";
	sprites.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	sprites.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	sprites.stages[1].module = spriteFragment;
	sprites.stages[1].pName = "main";
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	descriptorSetLayouts[0] = bufferAndImageLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &sprites.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = sprites.stages.size();
	graphicsPipelineCreateInfo.pStages = sprites.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &spriteAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &spriteAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = sprites.pipelineLayout;
	VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &sprites.pipeline));
	turbulent.stages.resize(2);
	turbulent.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulent.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	turbulent.stages[0].module = turbulentVertex;
	turbulent.stages[0].pName = "main";
	turbulent.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulent.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	turbulent.stages[1].module = turbulentFragment;
	turbulent.stages[1].pName = "main";
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = 17 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &turbulent.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = turbulent.stages.size();
	graphicsPipelineCreateInfo.pStages = turbulent.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &surfaceAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = turbulent.pipelineLayout;
	VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulent.pipeline));
	alias.stages.resize(2);
	alias.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	alias.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	alias.stages[0].module = aliasVertex;
	alias.stages[0].pName = "main";
	alias.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	alias.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	alias.stages[1].module = aliasFragment;
	alias.stages[1].pName = "main";
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantInfo.size = 16 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	pipelineLayoutCreateInfo.setLayoutCount = 3;
	VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &alias.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = alias.stages.size();
	graphicsPipelineCreateInfo.pStages = alias.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &colormappedAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &colormappedAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = alias.pipelineLayout;
	VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &alias.pipeline));
	viewmodel.stages.resize(2);
	viewmodel.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	viewmodel.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	viewmodel.stages[0].module = viewmodelVertex;
	viewmodel.stages[0].pName = "main";
	viewmodel.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	viewmodel.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	viewmodel.stages[1].module = viewmodelFragment;
	viewmodel.stages[1].pName = "main";
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = 24 * sizeof(float);
	VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &viewmodel.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = viewmodel.stages.size();
	graphicsPipelineCreateInfo.pStages = viewmodel.stages.data();
	graphicsPipelineCreateInfo.layout = viewmodel.pipelineLayout;
	VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &viewmodel.pipeline));
	colored.stages.resize(2);
	colored.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	colored.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	colored.stages[0].module = coloredVertex;
	colored.stages[0].pName = "main";
	colored.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	colored.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	colored.stages[1].module = coloredFragment;
	colored.stages[1].pName = "main";
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &colored.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = colored.stages.size();
	graphicsPipelineCreateInfo.pStages = colored.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &coloredAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &coloredAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = colored.pipelineLayout;
	VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &colored.pipeline));
	sky.stages.resize(2);
	sky.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	sky.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	sky.stages[0].module = skyVertex;
	sky.stages[0].pName = "main";
	sky.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	sky.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	sky.stages[1].module = skyFragment;
	sky.stages[1].pName = "main";
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = 13 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
	VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &sky.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = sky.stages.size();
	graphicsPipelineCreateInfo.pStages = sky.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &skyAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &skyAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = sky.pipelineLayout;
	VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &sky.pipeline));
	floor.stages.resize(2);
	floor.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	floor.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	floor.stages[0].module = floorVertex;
	floor.stages[0].pName = "main";
	floor.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	floor.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	floor.stages[1].module = floorFragment;
	floor.stages[1].pName = "main";
	descriptorSetLayouts[0] = singleBufferLayout;
	descriptorSetLayouts[1] = singleImageLayout;
	VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &floor.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = floor.stages.size();
	graphicsPipelineCreateInfo.pStages = floor.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &floorAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &floorAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = floor.pipelineLayout;
	VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &floor.pipeline));
	console.stages.resize(2);
	console.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	console.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	console.stages[0].module = consoleVertex;
	console.stages[0].pName = "main";
	console.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	console.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	console.stages[1].module = consoleFragment;
	console.stages[1].pName = "main";
	pipelineLayoutCreateInfo.pSetLayouts = &doubleImageLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &console.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = console.stages.size();
	graphicsPipelineCreateInfo.pStages = console.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &consoleAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &consoleAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = console.pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = appState.Console.RenderPass;
	VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &console.pipeline));
	matrices.CreateUniformBuffer(appState, numBuffers * 2 * sizeof(ovrMatrix4f));
	VkSamplerCreateInfo samplerCreateInfo { };
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.maxLod = 1;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VK(appState.Device.vkCreateSampler(appState.Device.device, &samplerCreateInfo, nullptr, &lightmapSampler));
	appState.Keyboard.Create(appState);
}

void Scene::CreateShader(AppState& appState, struct android_app* app, const char* filename, VkShaderModule* shaderModule)
{
	auto file = AAssetManager_open(app->activity->assetManager, filename, AASSET_MODE_BUFFER);
	size_t length = AAsset_getLength(file);
	if ((length % 4) != 0)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Scene::CreateShader(): %s is not 4-byte aligned.", filename);
		exit(0);
	}
	std::vector<unsigned char> buffer(length);
	AAsset_read(file, buffer.data(), length);
	VkShaderModuleCreateInfo moduleCreateInfo { };
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.pCode = (uint32_t *)buffer.data();
	moduleCreateInfo.codeSize = length;
	VK(appState.Device.vkCreateShaderModule(appState.Device.device, &moduleCreateInfo, nullptr, shaderModule));
}

void Scene::ClearSizes()
{
	stagingBufferSize = 0;
	verticesSize = 0;
	controllerVerticesSize = 0;
}

void Scene::Reset()
{
	D_ResetLists();
	viewmodelsPerKey.clear();
	aliasPerKey.clear();
	latestTextureSharedMemory = nullptr;
	usedInLatestTextureSharedMemory = 0;
	latestColormappedBuffer = nullptr;
	usedInLatestColormappedBuffer = 0;
	colormappedBufferList.clear();
	colormappedTexCoordsPerKey.clear();
	colormappedVerticesPerKey.clear();
	spritesPerKey.clear();
	fenceTexturesPerKey.clear();
	surfaceTexturesPerKey.clear();
	viewmodelTextures.DisposeFront();
	aliasTextures.DisposeFront();
	spriteTextures.DisposeFront();
	fenceTextures.DisposeFront();
	surfaceTextures.DisposeFront();
	for (auto entry = lightmaps.begin(); entry != lightmaps.end(); entry++)
	{
		for (auto l = &entry->second; *l != nullptr; )
		{
			auto next = (*l)->next;
			(*l)->next = oldSurfaces;
			oldSurfaces = *l;
			*l = next;
		}
	}
	lightmaps.clear();
	colormappedBuffers.DisposeFront();
	viewmodelTextureCount = 0;
	aliasTextureCount = 0;
	spriteTextureCount = 0;
	surfaceTextureCount = 0;
	resetDescriptorSetsCount++;
	if (skybox != VK_NULL_HANDLE)
	{
		vrapi_DestroyTextureSwapChain(skybox);
		skybox = VK_NULL_HANDLE;
	}
	for (auto entry = surfaceVerticesPerModel.begin(); entry != surfaceVerticesPerModel.end(); entry++)
	{
		auto b = entry->second;
		b->next = oldSurfaceVerticesPerModel;
		oldSurfaceVerticesPerModel = b;
	}
	surfaceVerticesPerModel.clear();
}
