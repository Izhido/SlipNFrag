#include "AppState.h"
#include "Scene.h" // This header is specified second in the list to allow headers in AppState.h to include the core engine structs
#include "Constants.h"
#include "Utils.h"
#include "ImageAsset.h"
#include "MemoryAllocateInfo.h"
#include "PipelineAttributes.h"
#include <android_native_app_glue.h>

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

void Scene::Create(AppState& appState, VkCommandBuffer& setupCommandBuffer, VkCommandBufferBeginInfo& commandBufferBeginInfo, VkSubmitInfo& setupSubmitInfo, struct android_app* app)
{
	appState.ConsoleWidth = 320;
	appState.ConsoleHeight = 200;
	appState.ScreenWidth = appState.ConsoleWidth * Constants::screenToConsoleMultiplier;
	appState.ScreenHeight = appState.ConsoleHeight * Constants::screenToConsoleMultiplier;

	appState.copyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	appState.copyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	appState.copyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	appState.copyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	appState.copyBarrier.subresourceRange.levelCount = 1;
	appState.copyBarrier.subresourceRange.layerCount = 1;

	appState.submitBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	appState.submitBarrier.srcAccessMask = appState.copyBarrier.dstAccessMask;
	appState.submitBarrier.oldLayout = appState.copyBarrier.newLayout;
	appState.submitBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	appState.submitBarrier.subresourceRange.aspectMask = appState.copyBarrier.subresourceRange.aspectMask;
	appState.submitBarrier.subresourceRange.levelCount = appState.copyBarrier.subresourceRange.levelCount;
	appState.submitBarrier.subresourceRange.layerCount = appState.copyBarrier.subresourceRange.layerCount;

	XrSwapchainCreateInfo swapchainCreateInfo { XR_TYPE_SWAPCHAIN_CREATE_INFO };
	swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
	swapchainCreateInfo.format = Constants::colorFormat;
	swapchainCreateInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
	swapchainCreateInfo.faceCount = 1;
	swapchainCreateInfo.arraySize = 1;
	swapchainCreateInfo.mipCount = 1;

	swapchainCreateInfo.width = appState.ScreenWidth;
	swapchainCreateInfo.height = appState.ScreenHeight;
	CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &appState.Screen.swapchain));

	uint32_t imageCount;
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Screen.swapchain, 0, &imageCount, nullptr));

	appState.Screen.swapchainImages.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Screen.swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)appState.Screen.swapchainImages.data()));

	if (imageCount > appState.CommandBuffers.size())
	{
		appState.CommandBuffers.resize(imageCount);
	}

	appState.ScreenData.assign(swapchainCreateInfo.width * swapchainCreateInfo.height, 255 << 24);
	ImageAsset play;
	play.Open("play.png", app);
	CopyImage(appState, play.image, appState.ScreenData.data() + ((appState.ScreenHeight - play.height) * appState.ScreenWidth + appState.ScreenWidth - play.width) / 2, play.width, play.height);
	play.Close();
	AddBorder(appState, appState.ScreenData);

	VkImageCreateInfo imageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = Constants::colorFormat;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VkMemoryRequirements memoryRequirements;
	VkMemoryAllocateInfo memoryAllocateInfo { };

	appState.ConsoleTextures.resize(imageCount);
	appState.StatusBarTextures.resize(imageCount);
	for (auto i = 0; i < imageCount; i++)
	{
		auto& consoleTexture = appState.ConsoleTextures[i];
		consoleTexture.width = appState.ConsoleWidth;
		consoleTexture.height = appState.ConsoleHeight;
		consoleTexture.mipCount = 1;
		consoleTexture.layerCount = 1;
		imageCreateInfo.extent.width = consoleTexture.width;
		imageCreateInfo.extent.height = consoleTexture.height;
		imageCreateInfo.mipLevels = consoleTexture.mipCount;
		imageCreateInfo.arrayLayers = consoleTexture.layerCount;
		CHECK_VKCMD(vkCreateImage(appState.Device, &imageCreateInfo, nullptr, &consoleTexture.image));
		vkGetImageMemoryRequirements(appState.Device, consoleTexture.image, &memoryRequirements);
		createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo, true);
		CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &consoleTexture.memory));
		CHECK_VKCMD(vkBindImageMemory(appState.Device, consoleTexture.image, consoleTexture.memory, 0));

		auto& statusBarTexture = appState.StatusBarTextures[i];
		statusBarTexture.width = appState.ConsoleWidth;
		statusBarTexture.height = SBAR_HEIGHT + 24;
		statusBarTexture.mipCount = 1;
		statusBarTexture.layerCount = 1;
		imageCreateInfo.extent.width = statusBarTexture.width;
		imageCreateInfo.extent.height = statusBarTexture.height;
		imageCreateInfo.mipLevels = statusBarTexture.mipCount;
		imageCreateInfo.arrayLayers = statusBarTexture.layerCount;
		CHECK_VKCMD(vkCreateImage(appState.Device, &imageCreateInfo, nullptr, &statusBarTexture.image));
		vkGetImageMemoryRequirements(appState.Device, statusBarTexture.image, &memoryRequirements);
		createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo, true);
		CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &statusBarTexture.memory));
		CHECK_VKCMD(vkBindImageMemory(appState.Device, statusBarTexture.image, statusBarTexture.memory, 0));
	}

	swapchainCreateInfo.width = appState.ScreenWidth;
	swapchainCreateInfo.height = appState.ScreenHeight / 2;
	CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &appState.Keyboard.Screen.swapchain));
	
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Keyboard.Screen.swapchain, 0, &imageCount, nullptr));

	appState.Keyboard.Screen.swapchainImages.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Keyboard.Screen.swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)appState.Keyboard.Screen.swapchainImages.data()));

	if (imageCount > appState.CommandBuffers.size())
	{
		appState.CommandBuffers.resize(imageCount);
	}

	VkCommandBufferAllocateInfo commandBufferAllocateInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	commandBufferAllocateInfo.commandPool = appState.CommandPool;
	commandBufferAllocateInfo.commandBufferCount = appState.CommandBuffers.size();
	CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, appState.CommandBuffers.data()));

	commandBufferAllocateInfo.commandBufferCount = 1;
	CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &setupCommandBuffer));
	CHECK_VKCMD(vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));

	ImageAsset floorImage;
	floorImage.Open("floor.png", app);
	floorTexture.Create(appState, floorImage.width, floorImage.height, Constants::colorFormat, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	ImageAsset controller;
	controller.Open("controller.png", app);
	controllerTexture.Create(appState, controller.width, controller.height, Constants::colorFormat, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	VkDeviceSize stagingBufferSize = (floorImage.width * floorImage.height + controller.width * controller.height + 2 * appState.ScreenWidth * appState.ScreenHeight) * sizeof(uint32_t);
	Buffer stagingBuffer;
	stagingBuffer.CreateStorageBuffer(appState, stagingBufferSize);
	CHECK_VKCMD(vkMapMemory(appState.Device, stagingBuffer.memory, 0, VK_WHOLE_SIZE, 0, &stagingBuffer.mapped));
	memcpy(stagingBuffer.mapped, floorImage.image, floorImage.width * floorImage.height * sizeof(uint32_t));
	floorImage.Close();
	size_t offset = floorImage.width * floorImage.height;
	memcpy((uint32_t*)stagingBuffer.mapped + offset, controller.image, controller.width * controller.height * sizeof(uint32_t));
	offset = 0;
	floorTexture.Fill(appState, &stagingBuffer, offset, setupCommandBuffer);
	offset += floorImage.width * floorImage.height * sizeof(uint32_t);
	controllerTexture.Fill(appState, &stagingBuffer, offset, setupCommandBuffer);
	CHECK_VKCMD(vkEndCommandBuffer(setupCommandBuffer));
	CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
	CHECK_VKCMD(vkQueueWaitIdle(appState.Queue));
	vkFreeCommandBuffers(appState.Device, appState.CommandPool, 1, &setupCommandBuffer);
	stagingBuffer.Delete(appState);
	appState.NoGameDataData.resize(appState.ScreenWidth * appState.ScreenHeight, 255 << 24);
	ImageAsset noGameData;
	noGameData.Open("nogamedata.png", app);
	CopyImage(appState, noGameData.image, appState.NoGameDataData.data() + ((appState.ScreenHeight - noGameData.height) * appState.ScreenWidth + appState.ScreenWidth - noGameData.width) / 2, noGameData.width, noGameData.height);
	noGameData.Close();
	AddBorder(appState, appState.NoGameDataData);

	swapchainCreateInfo.createFlags = XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
	swapchainCreateInfo.width = 450;
	swapchainCreateInfo.height = 150;
	CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &appState.LeftArrowsSwapchain));

	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.LeftArrowsSwapchain, 0, &imageCount, nullptr));

	std::vector<XrSwapchainImageVulkan2KHR> swapchainImages(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.LeftArrowsSwapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)swapchainImages.data()));

	VkBufferCreateInfo bufferCreateInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCreateInfo.size = swapchainCreateInfo.width * swapchainCreateInfo.height * 4;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VkBuffer buffer;

	VkDeviceMemory memoryBlock;

	VkBufferImageCopy region { };
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = swapchainCreateInfo.width;
	region.imageExtent.height = swapchainCreateInfo.height;
	region.imageExtent.depth = 1;

	uint32_t swapchainImageIndex;
	CHECK_XRCMD(xrAcquireSwapchainImage(appState.LeftArrowsSwapchain, nullptr, &swapchainImageIndex));

	XrSwapchainImageWaitInfo waitInfo { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
	waitInfo.timeout = XR_INFINITE_DURATION;
	CHECK_XRCMD(xrWaitSwapchainImage(appState.LeftArrowsSwapchain, &waitInfo));

	CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &setupCommandBuffer));
	CHECK_VKCMD(vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));

	CHECK_VKCMD(vkCreateBuffer(appState.Device, &bufferCreateInfo, nullptr, &buffer));
	vkGetBufferMemoryRequirements(appState.Device, buffer, &memoryRequirements);
	createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memoryAllocateInfo, true);
	CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &memoryBlock));
	CHECK_VKCMD(vkBindBufferMemory(appState.Device, buffer, memoryBlock, 0));
	void* mapped;
	CHECK_VKCMD(vkMapMemory(appState.Device, memoryBlock, 0, memoryRequirements.size, 0, &mapped));

	ImageAsset leftArrows;
	leftArrows.Open("leftarrows.png", app);
	memcpy(mapped, leftArrows.image, leftArrows.width * leftArrows.height * leftArrows.components);
	
	vkUnmapMemory(appState.Device, memoryBlock);

	appState.copyBarrier.image = swapchainImages[swapchainImageIndex].image;
	vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.copyBarrier);

	region.imageSubresource.baseArrayLayer = 0;
	vkCmdCopyBufferToImage(setupCommandBuffer, buffer, swapchainImages[swapchainImageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	appState.submitBarrier.image = swapchainImages[swapchainImageIndex].image;
	vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.submitBarrier);

	CHECK_VKCMD(vkEndCommandBuffer(setupCommandBuffer));
	CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));

	CHECK_VKCMD(vkQueueWaitIdle(appState.Queue));

	XrSwapchainImageReleaseInfo releaseInfo { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
	CHECK_XRCMD(xrReleaseSwapchainImage(appState.LeftArrowsSwapchain, &releaseInfo));

	vkFreeCommandBuffers(appState.Device, appState.CommandPool, 1, &setupCommandBuffer);
	vkDestroyBuffer(appState.Device, buffer, nullptr);
	vkFreeMemory(appState.Device, memoryBlock, nullptr);

	CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &appState.RightArrowsSwapchain));

	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.RightArrowsSwapchain, 0, &imageCount, nullptr));

	swapchainImages.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.RightArrowsSwapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)swapchainImages.data()));
 
 	bufferCreateInfo.size = swapchainCreateInfo.width * swapchainCreateInfo.height * 4;
	region.imageExtent.width = swapchainCreateInfo.width;
	region.imageExtent.height = swapchainCreateInfo.height;

	CHECK_XRCMD(xrAcquireSwapchainImage(appState.RightArrowsSwapchain, nullptr, &swapchainImageIndex));

	CHECK_XRCMD(xrWaitSwapchainImage(appState.RightArrowsSwapchain, &waitInfo));

	CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &setupCommandBuffer));
	CHECK_VKCMD(vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));

	CHECK_VKCMD(vkCreateBuffer(appState.Device, &bufferCreateInfo, nullptr, &buffer));
	vkGetBufferMemoryRequirements(appState.Device, buffer, &memoryRequirements);
	createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memoryAllocateInfo, true);
	CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &memoryBlock));
	CHECK_VKCMD(vkBindBufferMemory(appState.Device, buffer, memoryBlock, 0));

	CHECK_VKCMD(vkMapMemory(appState.Device, memoryBlock, 0, memoryRequirements.size, 0, &mapped));

	ImageAsset rightArrows;
	rightArrows.Open("rightarrows.png", app);
	memcpy(mapped, rightArrows.image, rightArrows.width * rightArrows.height * rightArrows.components);

	vkUnmapMemory(appState.Device, memoryBlock);

	appState.copyBarrier.image = swapchainImages[swapchainImageIndex].image;
	vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.copyBarrier);

	region.imageSubresource.baseArrayLayer = 0;
	vkCmdCopyBufferToImage(setupCommandBuffer, buffer, swapchainImages[swapchainImageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	appState.submitBarrier.image = swapchainImages[swapchainImageIndex].image;
	vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.submitBarrier);

	CHECK_VKCMD(vkEndCommandBuffer(setupCommandBuffer));
	CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));

	CHECK_VKCMD(vkQueueWaitIdle(appState.Queue));

	CHECK_XRCMD(xrReleaseSwapchainImage(appState.RightArrowsSwapchain, &releaseInfo));

	vkFreeCommandBuffers(appState.Device, appState.CommandPool, 1, &setupCommandBuffer);
	vkDestroyBuffer(appState.Device, buffer, nullptr);
	vkFreeMemory(appState.Device, memoryBlock, nullptr);

	VkShaderModule surfaceVertex;
	CreateShader(appState, app, "shaders/surface.vert.spv", &surfaceVertex);
	VkShaderModule surfaceFragment;
	CreateShader(appState, app, "shaders/surface.frag.spv", &surfaceFragment);
	VkShaderModule surfaceColoredLightsFragment;
	CreateShader(appState, app, "shaders/surface_colored_lights.frag.spv", &surfaceColoredLightsFragment);
	VkShaderModule surfaceRGBAVertex;
	CreateShader(appState, app, "shaders/surface_rgba.vert.spv", &surfaceRGBAVertex);
	VkShaderModule surfaceRGBAFragment;
	CreateShader(appState, app, "shaders/surface_rgba.frag.spv", &surfaceRGBAFragment);
	VkShaderModule surfaceRGBAColoredLightsFragment;
	CreateShader(appState, app, "shaders/surface_rgba_colored_lights.frag.spv", &surfaceRGBAColoredLightsFragment);
	VkShaderModule surfaceRGBANoGlowFragment;
	CreateShader(appState, app, "shaders/surface_rgba_no_glow.frag.spv", &surfaceRGBANoGlowFragment);
	VkShaderModule surfaceRGBANoGlowColoredLightsFragment;
	CreateShader(appState, app, "shaders/surface_rgba_no_glow_colored_lights.frag.spv", &surfaceRGBANoGlowColoredLightsFragment);
	VkShaderModule surfaceRotatedVertex;
	CreateShader(appState, app, "shaders/surface_rotated.vert.spv", &surfaceRotatedVertex);
	VkShaderModule surfaceRotatedRGBAVertex;
	CreateShader(appState, app, "shaders/surface_rotated_rgba.vert.spv", &surfaceRotatedRGBAVertex);
	VkShaderModule fenceFragment;
	CreateShader(appState, app, "shaders/fence.frag.spv", &fenceFragment);
	VkShaderModule fenceColoredLightsFragment;
	CreateShader(appState, app, "shaders/fence_colored_lights.frag.spv", &fenceColoredLightsFragment);
	VkShaderModule fenceRGBAFragment;
	CreateShader(appState, app, "shaders/fence_rgba.frag.spv", &fenceRGBAFragment);
	VkShaderModule fenceRGBAColoredLightsFragment;
	CreateShader(appState, app, "shaders/fence_rgba_colored_lights.frag.spv", &fenceRGBAColoredLightsFragment);
	VkShaderModule fenceRGBANoGlowFragment;
	CreateShader(appState, app, "shaders/fence_rgba_no_glow.frag.spv", &fenceRGBANoGlowFragment);
	VkShaderModule fenceRGBANoGlowColoredLightsFragment;
	CreateShader(appState, app, "shaders/fence_rgba_no_glow_colored_lights.frag.spv", &fenceRGBANoGlowColoredLightsFragment);
	VkShaderModule turbulentVertex;
	CreateShader(appState, app, "shaders/turbulent.vert.spv", &turbulentVertex);
	VkShaderModule turbulentFragment;
	CreateShader(appState, app, "shaders/turbulent.frag.spv", &turbulentFragment);
	VkShaderModule turbulentRGBAFragment;
	CreateShader(appState, app, "shaders/turbulent_rgba.frag.spv", &turbulentRGBAFragment);
	VkShaderModule turbulentLitFragment;
	CreateShader(appState, app, "shaders/turbulent_lit.frag.spv", &turbulentLitFragment);
	VkShaderModule turbulentColoredLightsFragment;
	CreateShader(appState, app, "shaders/turbulent_colored_lights.frag.spv", &turbulentColoredLightsFragment);
	VkShaderModule turbulentRGBALitFragment;
	CreateShader(appState, app, "shaders/turbulent_rgba_lit.frag.spv", &turbulentRGBALitFragment);
	VkShaderModule turbulentRGBAColoredLightsFragment;
	CreateShader(appState, app, "shaders/turbulent_rgba_colored_lights.frag.spv", &turbulentRGBAColoredLightsFragment);
	VkShaderModule turbulentRotatedVertex;
	CreateShader(appState, app, "shaders/turbulent_rotated.vert.spv", &turbulentRotatedVertex);
	VkShaderModule spriteVertex;
	CreateShader(appState, app, "shaders/sprite.vert.spv", &spriteVertex);
	VkShaderModule spriteFragment;
	CreateShader(appState, app, "shaders/sprite.frag.spv", &spriteFragment);
	VkShaderModule aliasVertex;
	CreateShader(appState, app, "shaders/alias.vert.spv", &aliasVertex);
	VkShaderModule aliasFragment;
	CreateShader(appState, app, "shaders/alias.frag.spv", &aliasFragment);
	VkShaderModule viewmodelVertex;
	CreateShader(appState, app, "shaders/viewmodel.vert.spv", &viewmodelVertex);
	VkShaderModule viewmodelFragment;
	CreateShader(appState, app, "shaders/viewmodel.frag.spv", &viewmodelFragment);
	VkShaderModule particleVertex;
	CreateShader(appState, app, "shaders/particle.vert.spv", &particleVertex);
	VkShaderModule skyVertex;
	CreateShader(appState, app, "shaders/sky.vert.spv", &skyVertex);
	VkShaderModule skyFragment;
	CreateShader(appState, app, "shaders/sky.frag.spv", &skyFragment);
	VkShaderModule skyRGBAFragment;
	CreateShader(appState, app, "shaders/sky_rgba.frag.spv", &skyRGBAFragment);
	VkShaderModule coloredVertex;
	CreateShader(appState, app, "shaders/colored.vert.spv", &coloredVertex);
	VkShaderModule coloredFragment;
	CreateShader(appState, app, "shaders/colored.frag.spv", &coloredFragment);
	VkShaderModule floorVertex;
	CreateShader(appState, app, "shaders/floor.vert.spv", &floorVertex);
	VkShaderModule floorFragment;
	CreateShader(appState, app, "shaders/floor.frag.spv", &floorFragment);

	VkDescriptorSetLayoutBinding descriptorSetBindings[3] { };
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetBindings;
	descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetBindings[0].descriptorCount = 1;
	descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	CHECK_VKCMD(vkCreateDescriptorSetLayout(appState.Device, &descriptorSetLayoutCreateInfo, nullptr, &singleBufferLayout));
	descriptorSetBindings[1].binding = 1;
	descriptorSetBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetBindings[1].descriptorCount = 1;
	descriptorSetBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutCreateInfo.bindingCount = 2;
	CHECK_VKCMD(vkCreateDescriptorSetLayout(appState.Device, &descriptorSetLayoutCreateInfo, nullptr, &doubleBufferLayout));
	descriptorSetBindings[2].binding = 2;
	descriptorSetBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetBindings[2].descriptorCount = 1;
	descriptorSetBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutCreateInfo.bindingCount = 3;
	CHECK_VKCMD(vkCreateDescriptorSetLayout(appState.Device, &descriptorSetLayoutCreateInfo, nullptr, &twoBuffersAndImageLayout));
	descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	CHECK_VKCMD(vkCreateDescriptorSetLayout(appState.Device, &descriptorSetLayoutCreateInfo, nullptr, &singleImageLayout));

	VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationStateCreateInfo.lineWidth = 1;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleStateCreateInfo.rasterizationSamples = (VkSampleCountFlagBits)appState.SwapchainSampleCount;
	multisampleStateCreateInfo.minSampleShading = 1;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateCreateInfo.front.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilStateCreateInfo.maxDepthBounds = 1;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState { };
	colorBlendAttachmentState.blendEnable = VK_TRUE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

	VkDynamicState dynamicStateEnables[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	pipelineDynamicStateCreateInfo.dynamicStateCount = 2;
	pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStateEnables;

	std::vector<VkPipelineShaderStageCreateInfo> stages(2);
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].pName = "main";

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	graphicsPipelineCreateInfo.pTessellationState = &tessellationStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
	graphicsPipelineCreateInfo.renderPass = appState.RenderPass;
	graphicsPipelineCreateInfo.stageCount = stages.size();
	graphicsPipelineCreateInfo.pStages = stages.data();

	VkPipelineInputAssemblyStateCreateInfo triangles { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	triangles.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineInputAssemblyStateCreateInfo triangleStrip { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	triangleStrip.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	PipelineAttributes surfaceAttributes { };
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
	surfaceAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	surfaceAttributes.vertexInputState.vertexBindingDescriptionCount = surfaceAttributes.vertexBindings.size();
	surfaceAttributes.vertexInputState.pVertexBindingDescriptions = surfaceAttributes.vertexBindings.data();
	surfaceAttributes.vertexInputState.vertexAttributeDescriptionCount = surfaceAttributes.vertexAttributes.size();
	surfaceAttributes.vertexInputState.pVertexAttributeDescriptions = surfaceAttributes.vertexAttributes.data();

	PipelineAttributes surfaceWithGlowAttributes { };
	surfaceWithGlowAttributes.vertexAttributes.resize(6);
	surfaceWithGlowAttributes.vertexBindings.resize(2);
	surfaceWithGlowAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	surfaceWithGlowAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	surfaceWithGlowAttributes.vertexAttributes[1].location = 1;
	surfaceWithGlowAttributes.vertexAttributes[1].binding = 1;
	surfaceWithGlowAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceWithGlowAttributes.vertexAttributes[2].location = 2;
	surfaceWithGlowAttributes.vertexAttributes[2].binding = 1;
	surfaceWithGlowAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceWithGlowAttributes.vertexAttributes[2].offset = 4 * sizeof(float);
	surfaceWithGlowAttributes.vertexAttributes[3].location = 3;
	surfaceWithGlowAttributes.vertexAttributes[3].binding = 1;
	surfaceWithGlowAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceWithGlowAttributes.vertexAttributes[3].offset = 8 * sizeof(float);
	surfaceWithGlowAttributes.vertexAttributes[4].location = 4;
	surfaceWithGlowAttributes.vertexAttributes[4].binding = 1;
	surfaceWithGlowAttributes.vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceWithGlowAttributes.vertexAttributes[4].offset = 12 * sizeof(float);
	surfaceWithGlowAttributes.vertexAttributes[5].location = 5;
	surfaceWithGlowAttributes.vertexAttributes[5].binding = 1;
	surfaceWithGlowAttributes.vertexAttributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceWithGlowAttributes.vertexAttributes[5].offset = 16 * sizeof(float);
	surfaceWithGlowAttributes.vertexBindings[1].binding = 1;
	surfaceWithGlowAttributes.vertexBindings[1].stride = 20 * sizeof(float);
	surfaceWithGlowAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	surfaceWithGlowAttributes.vertexInputState.vertexBindingDescriptionCount = surfaceWithGlowAttributes.vertexBindings.size();
	surfaceWithGlowAttributes.vertexInputState.pVertexBindingDescriptions = surfaceWithGlowAttributes.vertexBindings.data();
	surfaceWithGlowAttributes.vertexInputState.vertexAttributeDescriptionCount = surfaceWithGlowAttributes.vertexAttributes.size();
	surfaceWithGlowAttributes.vertexInputState.pVertexAttributeDescriptions = surfaceWithGlowAttributes.vertexAttributes.data();

	PipelineAttributes surfaceRotatedAttributes { };
	surfaceRotatedAttributes.vertexAttributes.resize(7);
	surfaceRotatedAttributes.vertexBindings.resize(2);
	surfaceRotatedAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	surfaceRotatedAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	surfaceRotatedAttributes.vertexAttributes[1].location = 1;
	surfaceRotatedAttributes.vertexAttributes[1].binding = 1;
	surfaceRotatedAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceRotatedAttributes.vertexAttributes[2].location = 2;
	surfaceRotatedAttributes.vertexAttributes[2].binding = 1;
	surfaceRotatedAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceRotatedAttributes.vertexAttributes[2].offset = 4 * sizeof(float);
	surfaceRotatedAttributes.vertexAttributes[3].location = 3;
	surfaceRotatedAttributes.vertexAttributes[3].binding = 1;
	surfaceRotatedAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceRotatedAttributes.vertexAttributes[3].offset = 8 * sizeof(float);
	surfaceRotatedAttributes.vertexAttributes[4].location = 4;
	surfaceRotatedAttributes.vertexAttributes[4].binding = 1;
	surfaceRotatedAttributes.vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceRotatedAttributes.vertexAttributes[4].offset = 12 * sizeof(float);
	surfaceRotatedAttributes.vertexAttributes[5].location = 5;
	surfaceRotatedAttributes.vertexAttributes[5].binding = 1;
	surfaceRotatedAttributes.vertexAttributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceRotatedAttributes.vertexAttributes[5].offset = 16 * sizeof(float);
	surfaceRotatedAttributes.vertexAttributes[6].location = 6;
	surfaceRotatedAttributes.vertexAttributes[6].binding = 1;
	surfaceRotatedAttributes.vertexAttributes[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceRotatedAttributes.vertexAttributes[6].offset = 20 * sizeof(float);
	surfaceRotatedAttributes.vertexBindings[1].binding = 1;
	surfaceRotatedAttributes.vertexBindings[1].stride = 24 * sizeof(float);
	surfaceRotatedAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	surfaceRotatedAttributes.vertexInputState.vertexBindingDescriptionCount = surfaceRotatedAttributes.vertexBindings.size();
	surfaceRotatedAttributes.vertexInputState.pVertexBindingDescriptions = surfaceRotatedAttributes.vertexBindings.data();
	surfaceRotatedAttributes.vertexInputState.vertexAttributeDescriptionCount = surfaceRotatedAttributes.vertexAttributes.size();
	surfaceRotatedAttributes.vertexInputState.pVertexAttributeDescriptions = surfaceRotatedAttributes.vertexAttributes.data();

	PipelineAttributes surfaceRotatedWithGlowAttributes { };
	surfaceRotatedWithGlowAttributes.vertexAttributes.resize(7);
	surfaceRotatedWithGlowAttributes.vertexBindings.resize(2);
	surfaceRotatedWithGlowAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	surfaceRotatedWithGlowAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	surfaceRotatedWithGlowAttributes.vertexAttributes[1].location = 1;
	surfaceRotatedWithGlowAttributes.vertexAttributes[1].binding = 1;
	surfaceRotatedWithGlowAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceRotatedWithGlowAttributes.vertexAttributes[2].location = 2;
	surfaceRotatedWithGlowAttributes.vertexAttributes[2].binding = 1;
	surfaceRotatedWithGlowAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceRotatedWithGlowAttributes.vertexAttributes[2].offset = 4 * sizeof(float);
	surfaceRotatedWithGlowAttributes.vertexAttributes[3].location = 3;
	surfaceRotatedWithGlowAttributes.vertexAttributes[3].binding = 1;
	surfaceRotatedWithGlowAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceRotatedWithGlowAttributes.vertexAttributes[3].offset = 8 * sizeof(float);
	surfaceRotatedWithGlowAttributes.vertexAttributes[4].location = 4;
	surfaceRotatedWithGlowAttributes.vertexAttributes[4].binding = 1;
	surfaceRotatedWithGlowAttributes.vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceRotatedWithGlowAttributes.vertexAttributes[4].offset = 12 * sizeof(float);
	surfaceRotatedWithGlowAttributes.vertexAttributes[5].location = 5;
	surfaceRotatedWithGlowAttributes.vertexAttributes[5].binding = 1;
	surfaceRotatedWithGlowAttributes.vertexAttributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceRotatedWithGlowAttributes.vertexAttributes[5].offset = 16 * sizeof(float);
	surfaceRotatedWithGlowAttributes.vertexAttributes[6].location = 6;
	surfaceRotatedWithGlowAttributes.vertexAttributes[6].binding = 1;
	surfaceRotatedWithGlowAttributes.vertexAttributes[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceRotatedWithGlowAttributes.vertexAttributes[6].offset = 20 * sizeof(float);
	surfaceRotatedWithGlowAttributes.vertexBindings[1].binding = 1;
	surfaceRotatedWithGlowAttributes.vertexBindings[1].stride = 24 * sizeof(float);
	surfaceRotatedWithGlowAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	surfaceRotatedWithGlowAttributes.vertexInputState.vertexBindingDescriptionCount = surfaceRotatedWithGlowAttributes.vertexBindings.size();
	surfaceRotatedWithGlowAttributes.vertexInputState.pVertexBindingDescriptions = surfaceRotatedWithGlowAttributes.vertexBindings.data();
	surfaceRotatedWithGlowAttributes.vertexInputState.vertexAttributeDescriptionCount = surfaceRotatedWithGlowAttributes.vertexAttributes.size();
	surfaceRotatedWithGlowAttributes.vertexInputState.pVertexAttributeDescriptions = surfaceRotatedWithGlowAttributes.vertexAttributes.data();

	PipelineAttributes turbulentAttributes { };
	turbulentAttributes.vertexAttributes.resize(4);
	turbulentAttributes.vertexBindings.resize(2);
	turbulentAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	turbulentAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	turbulentAttributes.vertexAttributes[1].location = 1;
	turbulentAttributes.vertexAttributes[1].binding = 1;
	turbulentAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	turbulentAttributes.vertexAttributes[2].location = 2;
	turbulentAttributes.vertexAttributes[2].binding = 1;
	turbulentAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	turbulentAttributes.vertexAttributes[2].offset = 4 * sizeof(float);
	turbulentAttributes.vertexAttributes[3].location = 3;
	turbulentAttributes.vertexAttributes[3].binding = 1;
	turbulentAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	turbulentAttributes.vertexAttributes[3].offset = 8 * sizeof(float);
	turbulentAttributes.vertexBindings[1].binding = 1;
	turbulentAttributes.vertexBindings[1].stride = 12 * sizeof(float);
	turbulentAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	turbulentAttributes.vertexInputState.vertexBindingDescriptionCount = turbulentAttributes.vertexBindings.size();
	turbulentAttributes.vertexInputState.pVertexBindingDescriptions = turbulentAttributes.vertexBindings.data();
	turbulentAttributes.vertexInputState.vertexAttributeDescriptionCount = turbulentAttributes.vertexAttributes.size();
	turbulentAttributes.vertexInputState.pVertexAttributeDescriptions = turbulentAttributes.vertexAttributes.data();

	PipelineAttributes turbulentRotatedAttributes { };
	turbulentRotatedAttributes.vertexAttributes.resize(6);
	turbulentRotatedAttributes.vertexBindings.resize(2);
	turbulentRotatedAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	turbulentRotatedAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	turbulentRotatedAttributes.vertexAttributes[1].location = 1;
	turbulentRotatedAttributes.vertexAttributes[1].binding = 1;
	turbulentRotatedAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	turbulentRotatedAttributes.vertexAttributes[2].location = 2;
	turbulentRotatedAttributes.vertexAttributes[2].binding = 1;
	turbulentRotatedAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	turbulentRotatedAttributes.vertexAttributes[2].offset = 4 * sizeof(float);
	turbulentRotatedAttributes.vertexAttributes[3].location = 3;
	turbulentRotatedAttributes.vertexAttributes[3].binding = 1;
	turbulentRotatedAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	turbulentRotatedAttributes.vertexAttributes[3].offset = 8 * sizeof(float);
	turbulentRotatedAttributes.vertexAttributes[4].location = 4;
	turbulentRotatedAttributes.vertexAttributes[4].binding = 1;
	turbulentRotatedAttributes.vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	turbulentRotatedAttributes.vertexAttributes[4].offset = 12 * sizeof(float);
	turbulentRotatedAttributes.vertexAttributes[5].location = 5;
	turbulentRotatedAttributes.vertexAttributes[5].binding = 1;
	turbulentRotatedAttributes.vertexAttributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	turbulentRotatedAttributes.vertexAttributes[5].offset = 16 * sizeof(float);
	turbulentRotatedAttributes.vertexBindings[1].binding = 1;
	turbulentRotatedAttributes.vertexBindings[1].stride = 20 * sizeof(float);
	turbulentRotatedAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	turbulentRotatedAttributes.vertexInputState.vertexBindingDescriptionCount = turbulentRotatedAttributes.vertexBindings.size();
	turbulentRotatedAttributes.vertexInputState.pVertexBindingDescriptions = turbulentRotatedAttributes.vertexBindings.data();
	turbulentRotatedAttributes.vertexInputState.vertexAttributeDescriptionCount = turbulentRotatedAttributes.vertexAttributes.size();
	turbulentRotatedAttributes.vertexInputState.pVertexAttributeDescriptions = turbulentRotatedAttributes.vertexAttributes.data();

	PipelineAttributes texturedAttributes { };
	texturedAttributes.vertexAttributes.resize(2);
	texturedAttributes.vertexBindings.resize(2);
	texturedAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	texturedAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	texturedAttributes.vertexAttributes[1].location = 1;
	texturedAttributes.vertexAttributes[1].binding = 1;
	texturedAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	texturedAttributes.vertexBindings[1].binding = 1;
	texturedAttributes.vertexBindings[1].stride = 2 * sizeof(float);
	texturedAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	texturedAttributes.vertexInputState.vertexBindingDescriptionCount = texturedAttributes.vertexBindings.size();
	texturedAttributes.vertexInputState.pVertexBindingDescriptions = texturedAttributes.vertexBindings.data();
	texturedAttributes.vertexInputState.vertexAttributeDescriptionCount = texturedAttributes.vertexAttributes.size();
	texturedAttributes.vertexInputState.pVertexAttributeDescriptions = texturedAttributes.vertexAttributes.data();

	PipelineAttributes aliasAttributes { };
	aliasAttributes.vertexAttributes.resize(3);
	aliasAttributes.vertexBindings.resize(3);
	aliasAttributes.vertexAttributes[0].format = VK_FORMAT_R8G8B8A8_UINT;
	aliasAttributes.vertexBindings[0].stride = 4 * sizeof(byte);
	aliasAttributes.vertexAttributes[1].location = 1;
	aliasAttributes.vertexAttributes[1].binding = 1;
	aliasAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	aliasAttributes.vertexBindings[1].binding = 1;
	aliasAttributes.vertexBindings[1].stride = 2 * sizeof(float);
	aliasAttributes.vertexAttributes[2].location = 2;
	aliasAttributes.vertexAttributes[2].binding = 2;
	aliasAttributes.vertexAttributes[2].format = VK_FORMAT_R32_SFLOAT;
	aliasAttributes.vertexBindings[2].binding = 2;
	aliasAttributes.vertexBindings[2].stride = sizeof(float);
	aliasAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	aliasAttributes.vertexInputState.vertexBindingDescriptionCount = aliasAttributes.vertexBindings.size();
	aliasAttributes.vertexInputState.pVertexBindingDescriptions = aliasAttributes.vertexBindings.data();
	aliasAttributes.vertexInputState.vertexAttributeDescriptionCount = aliasAttributes.vertexAttributes.size();
	aliasAttributes.vertexInputState.pVertexAttributeDescriptions = aliasAttributes.vertexAttributes.data();

	PipelineAttributes particleAttributes { };
	particleAttributes.vertexAttributes.resize(2);
	particleAttributes.vertexBindings.resize(2);
	particleAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	particleAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	particleAttributes.vertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
	particleAttributes.vertexAttributes[1].location = 1;
	particleAttributes.vertexAttributes[1].binding = 1;
	particleAttributes.vertexAttributes[1].format = VK_FORMAT_R32_SFLOAT;
	particleAttributes.vertexBindings[1].binding = 1;
	particleAttributes.vertexBindings[1].stride = sizeof(float);
	particleAttributes.vertexBindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
	particleAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	particleAttributes.vertexInputState.vertexBindingDescriptionCount = particleAttributes.vertexBindings.size();
	particleAttributes.vertexInputState.pVertexBindingDescriptions = particleAttributes.vertexBindings.data();
	particleAttributes.vertexInputState.vertexAttributeDescriptionCount = particleAttributes.vertexAttributes.size();
	particleAttributes.vertexInputState.pVertexAttributeDescriptions = particleAttributes.vertexAttributes.data();

	PipelineAttributes skyAttributes { };
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

	PipelineAttributes coloredAttributes { };
	coloredAttributes.vertexAttributes.resize(2);
	coloredAttributes.vertexBindings.resize(2);
	coloredAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	coloredAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	coloredAttributes.vertexAttributes[1].location = 1;
	coloredAttributes.vertexAttributes[1].binding = 1;
	coloredAttributes.vertexAttributes[1].format = VK_FORMAT_R32_SFLOAT;
	coloredAttributes.vertexBindings[1].binding = 1;
	coloredAttributes.vertexBindings[1].stride = sizeof(float);
	coloredAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	coloredAttributes.vertexInputState.vertexBindingDescriptionCount = coloredAttributes.vertexBindings.size();
	coloredAttributes.vertexInputState.pVertexBindingDescriptions = coloredAttributes.vertexBindings.data();
	coloredAttributes.vertexInputState.vertexAttributeDescriptionCount = coloredAttributes.vertexAttributes.size();
	coloredAttributes.vertexInputState.pVertexAttributeDescriptions = coloredAttributes.vertexAttributes.data();

	VkDescriptorSetLayout descriptorSetLayouts[4] { };

	VkPushConstantRange pushConstantInfo { };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

	descriptorSetLayouts[0] = twoBuffersAndImageLayout;
	descriptorSetLayouts[1] = singleImageLayout;
	descriptorSetLayouts[2] = singleImageLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 3;
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfaces.pipelineLayout));
	graphicsPipelineCreateInfo.layout = surfaces.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &triangles;
	stages[0].module = surfaceVertex;
	stages[1].module = surfaceFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfaces.pipeline));

	descriptorSetLayouts[0] = doubleBufferLayout;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = 5 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = surfacesColoredLights.pipelineLayout;
	stages[1].module = surfaceColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesColoredLights.pipeline));

	descriptorSetLayouts[0] = singleBufferLayout;
	descriptorSetLayouts[3] = singleImageLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 4;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRGBA.pipelineLayout));
	graphicsPipelineCreateInfo.layout = surfacesRGBA.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceWithGlowAttributes.vertexInputState;
	stages[0].module = surfaceRGBAVertex;
	stages[1].module = surfaceRGBAFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRGBA.pipeline));

	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRGBAColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = surfacesRGBAColoredLights.pipelineLayout;
	stages[1].module = surfaceRGBAColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRGBAColoredLights.pipeline));

	pipelineLayoutCreateInfo.setLayoutCount = 3;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRGBANoGlow.pipelineLayout));
	graphicsPipelineCreateInfo.layout = surfacesRGBANoGlow.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
	stages[0].module = surfaceVertex;
	stages[1].module = surfaceRGBANoGlowFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRGBANoGlow.pipeline));

	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRGBANoGlowColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = surfacesRGBANoGlowColoredLights.pipelineLayout;
	stages[1].module = surfaceRGBANoGlowColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRGBANoGlowColoredLights.pipeline));

	descriptorSetLayouts[0] = twoBuffersAndImageLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotated.pipelineLayout));
	graphicsPipelineCreateInfo.layout = surfacesRotated.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceRotatedAttributes.vertexInputState;
	stages[0].module = surfaceRotatedVertex;
	stages[1].module = surfaceFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotated.pipeline));

	descriptorSetLayouts[0] = doubleBufferLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotatedColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = surfacesRotatedColoredLights.pipelineLayout;
	stages[1].module = surfaceColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotatedColoredLights.pipeline));

	descriptorSetLayouts[0] = singleBufferLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 4;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotatedRGBA.pipelineLayout));
	graphicsPipelineCreateInfo.layout = surfacesRotatedRGBA.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceRotatedWithGlowAttributes.vertexInputState;
	stages[0].module = surfaceRotatedRGBAVertex;
	stages[1].module = surfaceRGBAFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotatedRGBA.pipeline));

	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotatedRGBAColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = surfacesRotatedRGBAColoredLights.pipelineLayout;
	stages[1].module = surfaceRGBAColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotatedRGBAColoredLights.pipeline));

	pipelineLayoutCreateInfo.setLayoutCount = 3;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotatedRGBANoGlow.pipelineLayout));
	graphicsPipelineCreateInfo.layout = surfacesRotatedRGBANoGlow.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceRotatedAttributes.vertexInputState;
	stages[0].module = surfaceRotatedVertex;
	stages[1].module = surfaceRGBANoGlowFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotatedRGBANoGlow.pipeline));

	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotatedRGBANoGlowColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = surfacesRotatedRGBANoGlowColoredLights.pipelineLayout;
	stages[1].module = surfaceRGBANoGlowColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotatedRGBANoGlowColoredLights.pipeline));

	descriptorSetLayouts[0] = twoBuffersAndImageLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fences.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fences.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
	stages[0].module = surfaceVertex;
	stages[1].module = fenceFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fences.pipeline));

	descriptorSetLayouts[0] = doubleBufferLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fencesColoredLights.pipelineLayout;
	stages[1].module = fenceColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesColoredLights.pipeline));

	descriptorSetLayouts[0] = singleBufferLayout;
	descriptorSetLayouts[3] = singleImageLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 4;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRGBA.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fencesRGBA.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceWithGlowAttributes.vertexInputState;
	stages[0].module = surfaceRGBAVertex;
	stages[1].module = fenceRGBAFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRGBA.pipeline));

	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRGBAColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fencesRGBAColoredLights.pipelineLayout;
	stages[1].module = fenceRGBAColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRGBAColoredLights.pipeline));

	pipelineLayoutCreateInfo.setLayoutCount = 3;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRGBANoGlow.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fencesRGBANoGlow.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
	stages[0].module = surfaceVertex;
	stages[1].module = fenceRGBANoGlowFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRGBANoGlow.pipeline));

	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRGBANoGlowColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fencesRGBANoGlowColoredLights.pipelineLayout;
	stages[1].module = fenceRGBANoGlowColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRGBANoGlowColoredLights.pipeline));

	descriptorSetLayouts[0] = twoBuffersAndImageLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotated.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fencesRotated.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceRotatedAttributes.vertexInputState;
	stages[0].module = surfaceRotatedVertex;
	stages[1].module = fenceFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotated.pipeline));

	descriptorSetLayouts[0] = doubleBufferLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotatedColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fencesRotatedColoredLights.pipelineLayout;
	stages[1].module = fenceColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotatedColoredLights.pipeline));

	descriptorSetLayouts[0] = singleBufferLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 4;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotatedRGBA.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fencesRotatedRGBA.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceRotatedWithGlowAttributes.vertexInputState;
	stages[0].module = surfaceRotatedRGBAVertex;
	stages[1].module = fenceRGBAFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotatedRGBA.pipeline));

	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotatedRGBAColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fencesRotatedRGBAColoredLights.pipelineLayout;
	stages[1].module = fenceRGBAColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotatedRGBAColoredLights.pipeline));

	pipelineLayoutCreateInfo.setLayoutCount = 3;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotatedRGBANoGlow.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fencesRotatedRGBANoGlow.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceRotatedAttributes.vertexInputState;
	stages[0].module = surfaceRotatedVertex;
	stages[1].module = fenceRGBANoGlowFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotatedRGBANoGlow.pipeline));

	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotatedRGBANoGlowColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fencesRotatedRGBANoGlowColoredLights.pipelineLayout;
	stages[1].module = fenceRGBANoGlowColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotatedRGBANoGlowColoredLights.pipeline));

	descriptorSetLayouts[0] = doubleBufferLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pushConstantInfo.size = sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulent.pipelineLayout));
	graphicsPipelineCreateInfo.layout = turbulent.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &turbulentAttributes.vertexInputState;
	stages[0].module = turbulentVertex;
	stages[1].module = turbulentFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulent.pipeline));

	descriptorSetLayouts[0] = singleBufferLayout;
	pushConstantInfo.size = 6 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRGBA.pipelineLayout));
	graphicsPipelineCreateInfo.layout = turbulentRGBA.pipelineLayout;
	stages[0].module = turbulentVertex;
	stages[1].module = turbulentRGBAFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRGBA.pipeline));

	descriptorSetLayouts[0] = twoBuffersAndImageLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 3;
	pushConstantInfo.size = sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentLit.pipelineLayout));
	graphicsPipelineCreateInfo.layout = turbulentLit.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
	stages[0].module = surfaceVertex;
	stages[1].module = turbulentLitFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentLit.pipeline));

	descriptorSetLayouts[0] = doubleBufferLayout;
	pushConstantInfo.size = 6 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = turbulentColoredLights.pipelineLayout;
	stages[1].module = turbulentColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentColoredLights.pipeline));

	descriptorSetLayouts[0] = singleBufferLayout;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRGBALit.pipelineLayout));
	graphicsPipelineCreateInfo.layout = turbulentRGBALit.pipelineLayout;
	stages[0].module = surfaceVertex;
	stages[1].module = turbulentRGBALitFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRGBALit.pipeline));

	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRGBAColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = turbulentRGBAColoredLights.pipelineLayout;
	stages[1].module = turbulentRGBAColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRGBAColoredLights.pipeline));

	descriptorSetLayouts[0] = doubleBufferLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pushConstantInfo.size = sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotated.pipelineLayout));
	graphicsPipelineCreateInfo.layout = turbulentRotated.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &turbulentRotatedAttributes.vertexInputState;
	stages[0].module = turbulentRotatedVertex;
	stages[1].module = turbulentFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotated.pipeline));

	descriptorSetLayouts[0] = singleBufferLayout;
	pushConstantInfo.size = 6 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotatedRGBA.pipelineLayout));
	graphicsPipelineCreateInfo.layout = turbulentRotatedRGBA.pipelineLayout;
	stages[0].module = turbulentRotatedVertex;
	stages[1].module = turbulentRGBAFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotatedRGBA.pipeline));

	descriptorSetLayouts[0] = twoBuffersAndImageLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 3;
	pushConstantInfo.size = sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotatedLit.pipelineLayout));
	graphicsPipelineCreateInfo.layout = turbulentRotatedLit.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceRotatedAttributes.vertexInputState;
	stages[0].module = surfaceRotatedVertex;
	stages[1].module = turbulentLitFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotatedLit.pipeline));

	descriptorSetLayouts[0] = doubleBufferLayout;
	pushConstantInfo.size = 6 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotatedColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = turbulentRotatedColoredLights.pipelineLayout;
	stages[1].module = turbulentColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotatedColoredLights.pipeline));

	descriptorSetLayouts[0] = singleBufferLayout;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotatedRGBALit.pipelineLayout));
	graphicsPipelineCreateInfo.layout = turbulentRotatedRGBALit.pipelineLayout;
	stages[0].module = surfaceRotatedVertex;
	stages[1].module = turbulentRGBALitFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotatedRGBALit.pipeline));

	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotatedRGBAColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = turbulentRotatedRGBAColoredLights.pipelineLayout;
	stages[1].module = turbulentRGBAColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotatedRGBAColoredLights.pipeline));

	descriptorSetLayouts[0] = doubleBufferLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &sprites.pipelineLayout));
	graphicsPipelineCreateInfo.layout = sprites.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &texturedAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &triangleStrip;
	stages[0].module = spriteVertex;
	stages[1].module = spriteFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &sprites.pipeline));

	pipelineLayoutCreateInfo.setLayoutCount = 3;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantInfo.size = 16 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &alias.pipelineLayout));
	graphicsPipelineCreateInfo.layout = alias.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &aliasAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &triangles;
	stages[0].module = aliasVertex;
	stages[1].module = aliasFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &alias.pipeline));

	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &viewmodels.pipelineLayout));
	graphicsPipelineCreateInfo.layout = viewmodels.pipelineLayout;
	stages[0].module = viewmodelVertex;
	stages[1].module = viewmodelFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &viewmodels.pipeline));

	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pushConstantInfo.size = 8 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &particle.pipelineLayout));
	graphicsPipelineCreateInfo.layout = particle.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &particleAttributes.vertexInputState;
	stages[0].module = particleVertex;
	stages[1].module = coloredFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &particle.pipeline));

	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &colored.pipelineLayout));
	graphicsPipelineCreateInfo.layout = colored.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &coloredAttributes.vertexInputState;
	stages[0].module = coloredVertex;
	stages[1].module = coloredFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &colored.pipeline));

	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = 13 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &sky.pipelineLayout));
	depthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
	graphicsPipelineCreateInfo.layout = sky.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &skyAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &triangleStrip;
	stages[0].module = skyVertex;
	stages[1].module = skyFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &sky.pipeline));

	descriptorSetLayouts[0] = singleBufferLayout;
	pushConstantInfo.size = 15 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &skyRGBA.pipelineLayout));
	graphicsPipelineCreateInfo.layout = skyRGBA.pipelineLayout;
	stages[0].module = skyVertex;
	stages[1].module = skyRGBAFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &skyRGBA.pipeline));

	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &floor.pipelineLayout));
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	graphicsPipelineCreateInfo.layout = floor.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &texturedAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &triangles;
	stages[0].module = floorVertex;
	stages[1].module = floorFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &floor.pipeline));

	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &floorStrip.pipelineLayout));
	graphicsPipelineCreateInfo.layout = floorStrip.pipelineLayout;
	graphicsPipelineCreateInfo.pInputAssemblyState = &triangleStrip;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &floorStrip.pipeline));

	vkDestroyShaderModule(appState.Device, floorFragment, nullptr);
	vkDestroyShaderModule(appState.Device, floorVertex, nullptr);
	vkDestroyShaderModule(appState.Device, coloredFragment, nullptr);
	vkDestroyShaderModule(appState.Device, coloredVertex, nullptr);
	vkDestroyShaderModule(appState.Device, skyRGBAFragment, nullptr);
	vkDestroyShaderModule(appState.Device, skyFragment, nullptr);
	vkDestroyShaderModule(appState.Device, skyVertex, nullptr);
	vkDestroyShaderModule(appState.Device, particleVertex, nullptr);
	vkDestroyShaderModule(appState.Device, viewmodelFragment, nullptr);
	vkDestroyShaderModule(appState.Device, viewmodelVertex, nullptr);
	vkDestroyShaderModule(appState.Device, aliasFragment, nullptr);
	vkDestroyShaderModule(appState.Device, aliasVertex, nullptr);
	vkDestroyShaderModule(appState.Device, spriteFragment, nullptr);
	vkDestroyShaderModule(appState.Device, spriteVertex, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentRotatedVertex, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentRGBAColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentRGBALitFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentLitFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentRGBAFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentVertex, nullptr);
	vkDestroyShaderModule(appState.Device, fenceRGBANoGlowColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, fenceRGBANoGlowFragment, nullptr);
	vkDestroyShaderModule(appState.Device, fenceRGBAColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, fenceRGBAFragment, nullptr);
	vkDestroyShaderModule(appState.Device, fenceColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, fenceFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRotatedRGBAVertex, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRotatedVertex, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRGBANoGlowColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRGBANoGlowFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRGBAColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRGBAFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRGBAVertex, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceVertex, nullptr);

	VkSamplerCreateInfo samplerCreateInfo { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	CHECK_VKCMD(vkCreateSampler(appState.Device, &samplerCreateInfo, nullptr, &lightmapSampler));

	appState.Keyboard.Create(appState);

	created = true;
}

void Scene::CreateShader(AppState& appState, struct android_app* app, const char* filename, VkShaderModule* shaderModule)
{
	auto file = AAssetManager_open(app->activity->assetManager, filename, AASSET_MODE_BUFFER);
	size_t length = AAsset_getLength(file);
	if ((length % 4) != 0)
	{
		THROW(Fmt("Scene::CreateShader(): %s is not 4-byte aligned.", filename));
	}

	std::vector<unsigned char> buffer(length);
	AAsset_read(file, buffer.data(), length);

	VkShaderModuleCreateInfo moduleCreateInfo { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	moduleCreateInfo.pCode = (uint32_t *)buffer.data();
	moduleCreateInfo.codeSize = length;
	CHECK_VKCMD(vkCreateShaderModule(appState.Device, &moduleCreateInfo, nullptr, shaderModule));
}
