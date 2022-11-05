#include "AppState.h"
#include "Scene.h" // This header is specified second in the list to allow headers in AppState.h to include the core engine structs
#include "Utils.h"
#include "ImageAsset.h"
#include "PipelineAttributes.h"
#include "Constants.h"
#include "MemoryAllocateInfo.h"
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
	VkShaderModule fenceRGBANoGlowFragment;
	CreateShader(appState, app, "shaders/fence_rgba_no_glow.frag.spv", &fenceRGBANoGlowFragment);
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

	pipelineLayoutCreateInfo.setLayoutCount = 3;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRGBANoGlow.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fencesRGBANoGlow.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
	stages[0].module = surfaceVertex;
	stages[1].module = fenceRGBANoGlowFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRGBANoGlow.pipeline));

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

	pipelineLayoutCreateInfo.setLayoutCount = 3;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotatedRGBANoGlow.pipelineLayout));
	graphicsPipelineCreateInfo.layout = fencesRotatedRGBANoGlow.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceRotatedAttributes.vertexInputState;
	stages[0].module = surfaceRotatedVertex;
	stages[1].module = fenceRGBANoGlowFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotatedRGBANoGlow.pipeline));

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

	descriptorSetLayouts[0] = doubleBufferLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pushConstantInfo.size = sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotated.pipelineLayout));
	graphicsPipelineCreateInfo.layout = turbulentRotated.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &turbulentRotatedAttributes.vertexInputState;
	stages[0].module = turbulentRotatedVertex;
	stages[1].module = turbulentFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotated.pipeline));

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

	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = 20 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &viewmodel.pipelineLayout));
	graphicsPipelineCreateInfo.layout = viewmodel.pipelineLayout;
	stages[0].module = viewmodelVertex;
	stages[1].module = viewmodelFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &viewmodel.pipeline));

	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
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
	vkDestroyShaderModule(appState.Device, turbulentRGBALitFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentLitFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentRGBAFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentVertex, nullptr);
	vkDestroyShaderModule(appState.Device, fenceRGBANoGlowFragment, nullptr);
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

void Scene::Initialize()
{
	sortedVerticesSize = 0;
	sortedAttributesSize = 0;
	sortedIndicesCount = 0;
	paletteSize = 0;
	colormapSize = 0;
	controllerVerticesSize = 0;
	buffers.Initialize();
	indexBuffers.Initialize();
	lightmaps.first = nullptr;
	lightmaps.current = nullptr;
	lightmapsRGBA.first = nullptr;
	lightmapsRGBA.current = nullptr;
	for (auto& cached : surfaceTextures)
	{
		cached.first = nullptr;
		cached.current = nullptr;
	}
	for (auto& cached : surfaceRGBATextures)
	{
		cached.first = nullptr;
		cached.current = nullptr;
	}
	textures.first = nullptr;
	textures.current = nullptr;
}

void Scene::AddSampler(AppState& appState, uint32_t mipCount)
{
	if (samplers.size() <= mipCount)
	{
		samplers.resize(mipCount + 1);
	}
	if (samplers[mipCount] == VK_NULL_HANDLE)
	{
		VkSamplerCreateInfo samplerCreateInfo { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerCreateInfo.maxLod = mipCount - 1;
		CHECK_VKCMD(vkCreateSampler(appState.Device, &samplerCreateInfo, nullptr, &samplers[mipCount]));
	}
}

void Scene::AddToBufferBarrier(VkBuffer buffer)
{
	stagingBuffer.lastEndBarrier++;
	if (stagingBuffer.bufferBarriers.size() <= stagingBuffer.lastEndBarrier)
	{
		stagingBuffer.bufferBarriers.emplace_back();
	}

	auto& barrier = stagingBuffer.bufferBarriers[stagingBuffer.lastEndBarrier];
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.buffer = buffer;
	barrier.size = VK_WHOLE_SIZE;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
}

VkDeviceSize Scene::GetAllocatedFor(int width, int height)
{
	VkDeviceSize allocated = 0;
	while (width > 1 || height > 1)
	{
		allocated += (width * height);
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
	}
	return allocated;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedLightmap& loaded, VkDeviceSize& size)
{
	auto lightmapEntry = lightmaps.lightmaps.find(surface.face);
	if (lightmapEntry == lightmaps.lightmaps.end())
	{
		auto lightmap = new Lightmap { };
		lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R16_UINT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		lightmap->createdFrameCount = surface.created;
		loaded.lightmap = lightmap;
		loaded.size = surface.lightmap_size * sizeof(uint16_t);
		size += loaded.size;
		loaded.source = d_lists.lightmap_texels.data() + surface.lightmap_texels;
		lightmaps.Setup(loaded);
		lightmaps.lightmaps.insert({ surface.face, lightmap });
	}
	else
	{
		auto lightmap = lightmapEntry->second;
		if (lightmap->createdFrameCount != surface.created)
		{
			lightmap->next = lightmaps.oldLightmaps;
			lightmaps.oldLightmaps = lightmap;
			lightmap = new Lightmap { };
			lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R16_UINT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			lightmap->createdFrameCount = surface.created;
			lightmapEntry->second = lightmap;
			loaded.lightmap = lightmap;
			loaded.size = surface.lightmap_size * sizeof(uint16_t);
			size += loaded.size;
			loaded.source = d_lists.lightmap_texels.data() + surface.lightmap_texels;
			lightmaps.Setup(loaded);
		}
		else
		{
			loaded.lightmap = lightmap;
		}
	}
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedLightmapRGBA& loaded, VkDeviceSize& size)
{
	auto lightmapEntry = lightmapsRGBA.lightmaps.find(surface.face);
	if (lightmapEntry == lightmapsRGBA.lightmaps.end())
	{
		auto lightmap = new LightmapRGBA { };
		lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R16G16B16A16_UINT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		lightmap->createdFrameCount = surface.created;
		loaded.lightmap = lightmap;
		loaded.size = surface.lightmap_size * sizeof(uint16_t);
		size += loaded.size;
		loaded.source = d_lists.lightmap_texels.data() + surface.lightmap_texels;
		lightmapsRGBA.Setup(loaded);
		lightmapsRGBA.lightmaps.insert({ surface.face, lightmap });
	}
	else
	{
		auto lightmap = lightmapEntry->second;
		if (lightmap->createdFrameCount != surface.created)
		{
			lightmap->next = lightmapsRGBA.oldLightmaps;
			lightmapsRGBA.oldLightmaps = lightmap;
			lightmap = new LightmapRGBA { };
			lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R16G16B16A16_UINT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			lightmap->createdFrameCount = surface.created;
			lightmapEntry->second = lightmap;
			loaded.lightmap = lightmap;
			loaded.size = surface.lightmap_size * sizeof(uint16_t);
			size += loaded.size;
			loaded.source = d_lists.lightmap_texels.data() + surface.lightmap_texels;
			lightmapsRGBA.Setup(loaded);
		}
		else
		{
			loaded.lightmap = lightmap;
		}
	}
}

void Scene::GetStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size)
{
	loaded.vertexes = ((model_t*)turbulent.model)->vertexes;
	loaded.face = turbulent.face;
	loaded.model = turbulent.model;
	if (previousTexture != turbulent.data)
	{
		auto entry = surfaceTextureCache.find(turbulent.data);
		if (entry == surfaceTextureCache.end())
		{
			CachedSharedMemoryTextures* cached = nullptr;
			for (auto& entry : surfaceTextures)
			{
				if (entry.width == turbulent.width && entry.height == turbulent.height)
				{
					cached = &entry;
					break;
				}
			}
			if (cached == nullptr)
			{
				surfaceTextures.push_back({ turbulent.width, turbulent.height });
				cached = &surfaceTextures.back();
			}
			if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount)
			{
				uint32_t layerCount;
				if (cached->textures == nullptr)
				{
					layerCount = 4;
				}
				else
				{
					layerCount = 64;
				}
				auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, turbulent.width, turbulent.height, VK_FORMAT_R8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				loaded.texture.texture = texture;
				cached->currentIndex = 0;
			}
			else
			{
				loaded.texture.texture = cached->textures;
			}
			cached->Setup(loaded.texture);
			loaded.texture.index = cached->currentIndex;
			loaded.texture.size = turbulent.size;
			loaded.texture.allocated = GetAllocatedFor(turbulent.width, turbulent.height);
			size += loaded.texture.allocated;
			loaded.texture.source = turbulent.data;
			loaded.texture.mips = turbulent.mips;
			surfaceTextureCache.insert({ turbulent.data, { loaded.texture.texture, loaded.texture.index } });
			cached->currentIndex++;
		}
		else
		{
			loaded.texture.texture = entry->second.texture;
			loaded.texture.index = entry->second.index;
		}
		previousTexture = turbulent.data;
		previousSharedMemoryTexture = loaded.texture.texture;
		previousSharedMemoryTextureIndex = loaded.texture.index;
	}
	else
	{
		loaded.texture.texture = previousSharedMemoryTexture;
		loaded.texture.index = previousSharedMemoryTextureIndex;
	}
	loaded.count = turbulent.count;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size)
{
	loaded.vertexes = ((model_t*)turbulent.model)->vertexes;
	loaded.face = turbulent.face;
	loaded.model = turbulent.model;
	if (previousTexture != turbulent.data)
	{
		auto entry = surfaceTextureCache.find(turbulent.data);
		if (entry == surfaceTextureCache.end())
		{
			CachedSharedMemoryTextures* cached = nullptr;
			for (auto& entry : surfaceRGBATextures)
			{
				if (entry.width == turbulent.width && entry.height == turbulent.height)
				{
					cached = &entry;
					break;
				}
			}
			if (cached == nullptr)
			{
				surfaceRGBATextures.push_back({ turbulent.width, turbulent.height });
				cached = &surfaceRGBATextures.back();
			}
			if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount)
			{
				uint32_t layerCount;
				if (cached->textures == nullptr)
				{
					layerCount = 4;
				}
				else
				{
					layerCount = 64;
				}
				auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, turbulent.width, turbulent.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				loaded.texture.texture = texture;
				cached->currentIndex = 0;
			}
			else
			{
				loaded.texture.texture = cached->textures;
			}
			cached->Setup(loaded.texture);
			loaded.texture.index = cached->currentIndex;
			loaded.texture.size = turbulent.size;
			loaded.texture.allocated = GetAllocatedFor(turbulent.width, turbulent.height) * sizeof(unsigned);
			size += loaded.texture.allocated;
			loaded.texture.source = turbulent.data;
			loaded.texture.mips = turbulent.mips;
			surfaceTextureCache.insert({ turbulent.data, { loaded.texture.texture, loaded.texture.index } });
			cached->currentIndex++;
		}
		else
		{
			loaded.texture.texture = entry->second.texture;
			loaded.texture.index = entry->second.index;
		}
		previousTexture = turbulent.data;
		previousSharedMemoryTexture = loaded.texture.texture;
		previousSharedMemoryTextureIndex = loaded.texture.index;
	}
	else
	{
		loaded.texture.texture = previousSharedMemoryTexture;
		loaded.texture.index = previousSharedMemoryTextureIndex;
	}
	loaded.count = turbulent.count;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size)
{
	GetStagingBufferSize(appState, (const dturbulent_t&)surface, loaded, size);
	GetStagingBufferSize(appState, surface, loaded.lightmap, size);
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurfaceColoredLights& loaded, VkDeviceSize& size)
{
	GetStagingBufferSize(appState, (const dturbulent_t&)surface, loaded, size);
	GetStagingBufferSize(appState, surface, loaded.lightmap, size);
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacewithglow_t& surface, LoadedSurface2Textures& loaded, VkDeviceSize& size)
{
	loaded.vertexes = ((model_t*)surface.model)->vertexes;
	loaded.face = surface.face;
	loaded.model = surface.model;
	if (previousTexture != surface.data)
	{
		auto entry = surfaceTextureCache.find(surface.data);
		if (entry == surfaceTextureCache.end())
		{
			CachedSharedMemoryTextures* cached = nullptr;
			for (auto& entry : surfaceRGBATextures)
			{
				if (entry.width == surface.width && entry.height == surface.height)
				{
					cached = &entry;
					break;
				}
			}
			if (cached == nullptr)
			{
				surfaceRGBATextures.push_back({ surface.width, surface.height });
				cached = &surfaceRGBATextures.back();
			}
			// By doing this, we guarantee that the next texture allocations (color and glow) will be done
			// in the same texture array, which will allow the sorted surfaces algorithm to work as expected:
			auto extra = (surface.glow_data != nullptr ? 1 : 0);
			if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount - extra)
			{
				uint32_t layerCount;
				if (cached->textures == nullptr)
				{
					layerCount = 4;
				}
				else
				{
					layerCount = 64;
				}
				auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				loaded.texture.texture = texture;
				cached->currentIndex = 0;
			}
			else
			{
				loaded.texture.texture = cached->textures;
			}
			cached->Setup(loaded.texture);
			loaded.texture.index = cached->currentIndex;
			loaded.texture.size = surface.size;
			loaded.texture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
			size += loaded.texture.allocated;
			loaded.texture.source = surface.data;
			loaded.texture.mips = surface.mips;
			surfaceTextureCache.insert({ surface.data, { loaded.texture.texture, loaded.texture.index } });
			cached->currentIndex++;
		}
		else
		{
			loaded.texture.texture = entry->second.texture;
			loaded.texture.index = entry->second.index;
		}
		previousTexture = surface.data;
		previousSharedMemoryTexture = loaded.texture.texture;
		previousSharedMemoryTextureIndex = loaded.texture.index;
	}
	else
	{
		loaded.texture.texture = previousSharedMemoryTexture;
		loaded.texture.index = previousSharedMemoryTextureIndex;
	}
	if (previousGlowTexture != surface.glow_data)
	{
		auto entry = surfaceTextureCache.find(surface.glow_data);
		if (entry == surfaceTextureCache.end())
		{
			CachedSharedMemoryTextures* cached = nullptr;
			for (auto& entry : surfaceRGBATextures)
			{
				if (entry.width == surface.width && entry.height == surface.height)
				{
					cached = &entry;
					break;
				}
			}
			if (cached == nullptr)
			{
				surfaceRGBATextures.push_back({ surface.width, surface.height });
				cached = &surfaceRGBATextures.back();
			}
			if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount)
			{
				uint32_t layerCount;
				if (cached->textures == nullptr)
				{
					layerCount = 4;
				}
				else
				{
					layerCount = 64;
				}
				auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				loaded.glowTexture.texture = texture;
				cached->currentIndex = 0;
			}
			else
			{
				loaded.glowTexture.texture = cached->textures;
			}
			cached->Setup(loaded.glowTexture);
			loaded.glowTexture.index = cached->currentIndex;
			loaded.glowTexture.size = surface.size;
			loaded.glowTexture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
			size += loaded.glowTexture.allocated;
			loaded.glowTexture.source = surface.glow_data;
			loaded.glowTexture.mips = surface.mips;
			surfaceTextureCache.insert({ surface.glow_data, { loaded.glowTexture.texture, loaded.glowTexture.index } });
			cached->currentIndex++;
		}
		else
		{
			loaded.glowTexture.texture = entry->second.texture;
			loaded.glowTexture.index = entry->second.index;
		}
		previousGlowTexture = surface.glow_data;
		previousGlowSharedMemoryTexture = loaded.glowTexture.texture;
		previousGlowSharedMemoryTextureIndex = loaded.glowTexture.index;
	}
	else
	{
		loaded.glowTexture.texture = previousGlowSharedMemoryTexture;
		loaded.glowTexture.index = previousGlowSharedMemoryTextureIndex;
	}
	GetStagingBufferSize(appState, surface, loaded.lightmap, size);
	loaded.count = surface.count;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacewithglow_t& surface, LoadedSurface2TexturesColoredLights& loaded, VkDeviceSize& size)
{
	loaded.vertexes = ((model_t*)surface.model)->vertexes;
	loaded.face = surface.face;
	loaded.model = surface.model;
	if (previousTexture != surface.data)
	{
		auto entry = surfaceTextureCache.find(surface.data);
		if (entry == surfaceTextureCache.end())
		{
			CachedSharedMemoryTextures* cached = nullptr;
			for (auto& entry : surfaceRGBATextures)
			{
				if (entry.width == surface.width && entry.height == surface.height)
				{
					cached = &entry;
					break;
				}
			}
			if (cached == nullptr)
			{
				surfaceRGBATextures.push_back({ surface.width, surface.height });
				cached = &surfaceRGBATextures.back();
			}
			// By doing this, we guarantee that the next texture allocations (color and glow) will be done
			// in the same texture array, which will allow the sorted surfaces algorithm to work as expected:
			auto extra = (surface.glow_data != nullptr ? 1 : 0);
			if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount - extra)
			{
				uint32_t layerCount;
				if (cached->textures == nullptr)
				{
					layerCount = 4;
				}
				else
				{
					layerCount = 64;
				}
				auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				loaded.texture.texture = texture;
				cached->currentIndex = 0;
			}
			else
			{
				loaded.texture.texture = cached->textures;
			}
			cached->Setup(loaded.texture);
			loaded.texture.index = cached->currentIndex;
			loaded.texture.size = surface.size;
			loaded.texture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
			size += loaded.texture.allocated;
			loaded.texture.source = surface.data;
			loaded.texture.mips = surface.mips;
			surfaceTextureCache.insert({ surface.data, { loaded.texture.texture, loaded.texture.index } });
			cached->currentIndex++;
		}
		else
		{
			loaded.texture.texture = entry->second.texture;
			loaded.texture.index = entry->second.index;
		}
		previousTexture = surface.data;
		previousSharedMemoryTexture = loaded.texture.texture;
		previousSharedMemoryTextureIndex = loaded.texture.index;
	}
	else
	{
		loaded.texture.texture = previousSharedMemoryTexture;
		loaded.texture.index = previousSharedMemoryTextureIndex;
	}
	if (previousGlowTexture != surface.glow_data)
	{
		auto entry = surfaceTextureCache.find(surface.glow_data);
		if (entry == surfaceTextureCache.end())
		{
			CachedSharedMemoryTextures* cached = nullptr;
			for (auto& entry : surfaceRGBATextures)
			{
				if (entry.width == surface.width && entry.height == surface.height)
				{
					cached = &entry;
					break;
				}
			}
			if (cached == nullptr)
			{
				surfaceRGBATextures.push_back({ surface.width, surface.height });
				cached = &surfaceRGBATextures.back();
			}
			if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount)
			{
				uint32_t layerCount;
				if (cached->textures == nullptr)
				{
					layerCount = 4;
				}
				else
				{
					layerCount = 64;
				}
				auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				loaded.glowTexture.texture = texture;
				cached->currentIndex = 0;
			}
			else
			{
				loaded.glowTexture.texture = cached->textures;
			}
			cached->Setup(loaded.glowTexture);
			loaded.glowTexture.index = cached->currentIndex;
			loaded.glowTexture.size = surface.size;
			loaded.glowTexture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
			size += loaded.glowTexture.allocated;
			loaded.glowTexture.source = surface.glow_data;
			loaded.glowTexture.mips = surface.mips;
			surfaceTextureCache.insert({ surface.glow_data, { loaded.glowTexture.texture, loaded.glowTexture.index } });
			cached->currentIndex++;
		}
		else
		{
			loaded.glowTexture.texture = entry->second.texture;
			loaded.glowTexture.index = entry->second.index;
		}
		previousGlowTexture = surface.glow_data;
		previousGlowSharedMemoryTexture = loaded.glowTexture.texture;
		previousGlowSharedMemoryTextureIndex = loaded.glowTexture.index;
	}
	else
	{
		loaded.glowTexture.texture = previousGlowSharedMemoryTexture;
		loaded.glowTexture.index = previousGlowSharedMemoryTextureIndex;
	}
	GetStagingBufferSize(appState, surface, loaded.lightmap, size);
	loaded.count = surface.count;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size)
{
	loaded.vertexes = ((model_t*)surface.model)->vertexes;
	loaded.face = surface.face;
	loaded.model = surface.model;
	if (previousTexture != surface.data)
	{
		auto entry = surfaceTextureCache.find(surface.data);
		if (entry == surfaceTextureCache.end())
		{
			CachedSharedMemoryTextures* cached = nullptr;
			for (auto& entry : surfaceRGBATextures)
			{
				if (entry.width == surface.width && entry.height == surface.height)
				{
					cached = &entry;
					break;
				}
			}
			if (cached == nullptr)
			{
				surfaceRGBATextures.push_back({ surface.width, surface.height });
				cached = &surfaceRGBATextures.back();
			}
			if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount)
			{
				uint32_t layerCount;
				if (cached->textures == nullptr)
				{
					layerCount = 4;
				}
				else
				{
					layerCount = 64;
				}
				auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				loaded.texture.texture = texture;
				cached->currentIndex = 0;
			}
			else
			{
				loaded.texture.texture = cached->textures;
			}
			cached->Setup(loaded.texture);
			loaded.texture.index = cached->currentIndex;
			loaded.texture.size = surface.size;
			loaded.texture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
			size += loaded.texture.allocated;
			loaded.texture.source = surface.data;
			loaded.texture.mips = surface.mips;
			surfaceTextureCache.insert({ surface.data, { loaded.texture.texture, loaded.texture.index } });
			cached->currentIndex++;
		}
		else
		{
			loaded.texture.texture = entry->second.texture;
			loaded.texture.index = entry->second.index;
		}
		previousTexture = surface.data;
		previousSharedMemoryTexture = loaded.texture.texture;
		previousSharedMemoryTextureIndex = loaded.texture.index;
	}
	else
	{
		loaded.texture.texture = previousSharedMemoryTexture;
		loaded.texture.index = previousSharedMemoryTextureIndex;
	}
	GetStagingBufferSize(appState, surface, loaded.lightmap, size);
	loaded.count = surface.count;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurface_t& surface, LoadedSurfaceColoredLights& loaded, VkDeviceSize& size)
{
	loaded.vertexes = ((model_t*)surface.model)->vertexes;
	loaded.face = surface.face;
	loaded.model = surface.model;
	if (previousTexture != surface.data)
	{
		auto entry = surfaceTextureCache.find(surface.data);
		if (entry == surfaceTextureCache.end())
		{
			CachedSharedMemoryTextures* cached = nullptr;
			for (auto& entry : surfaceRGBATextures)
			{
				if (entry.width == surface.width && entry.height == surface.height)
				{
					cached = &entry;
					break;
				}
			}
			if (cached == nullptr)
			{
				surfaceRGBATextures.push_back({ surface.width, surface.height });
				cached = &surfaceRGBATextures.back();
			}
			if (cached->textures == nullptr || cached->currentIndex >= cached->textures->layerCount)
			{
				uint32_t layerCount;
				if (cached->textures == nullptr)
				{
					layerCount = 4;
				}
				else
				{
					layerCount = 64;
				}
				auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				loaded.texture.texture = texture;
				cached->currentIndex = 0;
			}
			else
			{
				loaded.texture.texture = cached->textures;
			}
			cached->Setup(loaded.texture);
			loaded.texture.index = cached->currentIndex;
			loaded.texture.size = surface.size;
			loaded.texture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
			size += loaded.texture.allocated;
			loaded.texture.source = surface.data;
			loaded.texture.mips = surface.mips;
			surfaceTextureCache.insert({ surface.data, { loaded.texture.texture, loaded.texture.index } });
			cached->currentIndex++;
		}
		else
		{
			loaded.texture.texture = entry->second.texture;
			loaded.texture.index = entry->second.index;
		}
		previousTexture = surface.data;
		previousSharedMemoryTexture = loaded.texture.texture;
		previousSharedMemoryTextureIndex = loaded.texture.index;
	}
	else
	{
		loaded.texture.texture = previousSharedMemoryTexture;
		loaded.texture.index = previousSharedMemoryTextureIndex;
	}
	GetStagingBufferSize(appState, surface, loaded.lightmap, size);
	loaded.count = surface.count;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size)
{
	GetStagingBufferSize(appState, (const dsurface_t&)surface, loaded, size);
	loaded.originX = surface.origin_x;
	loaded.originY = surface.origin_y;
	loaded.originZ = surface.origin_z;
	loaded.yaw = surface.yaw;
	loaded.pitch = surface.pitch;
	loaded.roll = surface.roll;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotatedColoredLights& loaded, VkDeviceSize& size)
{
	GetStagingBufferSize(appState, (const dsurface_t&)surface, (LoadedSurfaceColoredLights&)loaded, size);
	loaded.originX = surface.origin_x;
	loaded.originY = surface.origin_y;
	loaded.originZ = surface.origin_z;
	loaded.yaw = surface.yaw;
	loaded.pitch = surface.pitch;
	loaded.roll = surface.roll;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacerotatedwithglow_t& surface, LoadedSurfaceRotated2Textures& loaded, VkDeviceSize& size)
{
	GetStagingBufferSize(appState, (const dsurfacewithglow_t&)surface, loaded, size);
	loaded.originX = surface.origin_x;
	loaded.originY = surface.origin_y;
	loaded.originZ = surface.origin_z;
	loaded.yaw = surface.yaw;
	loaded.pitch = surface.pitch;
	loaded.roll = surface.roll;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacerotatedwithglow_t& surface, LoadedSurfaceRotated2TexturesColoredLights& loaded, VkDeviceSize& size)
{
	GetStagingBufferSize(appState, (const dsurfacewithglow_t&)surface, loaded, size);
	loaded.originX = surface.origin_x;
	loaded.originY = surface.origin_y;
	loaded.originZ = surface.origin_z;
	loaded.yaw = surface.yaw;
	loaded.pitch = surface.pitch;
	loaded.roll = surface.roll;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size)
{
	GetStagingBufferSizeRGBANoGlow(appState, (const dsurface_t&)surface, loaded, size);
	loaded.originX = surface.origin_x;
	loaded.originY = surface.origin_y;
	loaded.originZ = surface.origin_z;
	loaded.yaw = surface.yaw;
	loaded.pitch = surface.pitch;
	loaded.roll = surface.roll;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotatedColoredLights& loaded, VkDeviceSize& size)
{
	GetStagingBufferSizeRGBANoGlow(appState, (const dsurface_t&)surface, loaded, size);
	loaded.originX = surface.origin_x;
	loaded.originY = surface.origin_y;
	loaded.originZ = surface.origin_z;
	loaded.yaw = surface.yaw;
	loaded.pitch = surface.pitch;
	loaded.roll = surface.roll;
}

void Scene::GetStagingBufferSize(AppState& appState, const dturbulentrotated_t& turbulent, LoadedTurbulentRotated& loaded, VkDeviceSize& size)
{
	GetStagingBufferSize(appState, (const dturbulent_t&)turbulent, loaded, size);
	loaded.originX = turbulent.origin_x;
	loaded.originY = turbulent.origin_y;
	loaded.originZ = turbulent.origin_z;
	loaded.yaw = turbulent.yaw;
	loaded.pitch = turbulent.pitch;
	loaded.roll = turbulent.roll;
}

void Scene::GetStagingBufferSize(AppState& appState, const dspritedata_t& sprite, LoadedSprite& loaded, VkDeviceSize& size)
{
	if (previousTexture != sprite.data)
	{
		auto entry = spriteCache.find(sprite.data);
		if (entry == spriteCache.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(sprite.width, sprite.height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, sprite.width, sprite.height, VK_FORMAT_R8_UINT, mipCount, 1, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			textures.MoveToFront(texture);
			loaded.texture.size = sprite.size;
			size += loaded.texture.size;
			loaded.texture.texture = texture;
			loaded.texture.source = sprite.data;
			loaded.texture.mips = 1;
			textures.Setup(loaded.texture);
			spriteCache.insert({ sprite.data, texture });
		}
		else
		{
			loaded.texture.texture = entry->second;
			loaded.texture.index = 0;
		}
		previousTexture = sprite.data;
		previousSharedMemoryTexture = loaded.texture.texture;
	}
	else
	{
		loaded.texture.texture = previousSharedMemoryTexture;
		loaded.texture.index = 0;
	}
	loaded.count = sprite.count;
	loaded.firstVertex = sprite.first_vertex;
}

void Scene::GetStagingBufferSize(AppState& appState, const dalias_t& alias, LoadedAlias& loaded, Texture* host_colormap, VkDeviceSize& size)
{
	if (previousApverts != alias.apverts)
	{
		auto entry = aliasVertexCache.find(alias.apverts);
		if (entry == aliasVertexCache.end())
		{
			auto vertexSize = 4 * sizeof(byte);
			loaded.vertices.size = alias.vertex_count * 2 * vertexSize;
			loaded.vertices.buffer = new SharedMemoryBuffer { };
			loaded.vertices.buffer->CreateVertexBuffer(appState, loaded.vertices.size);
			buffers.MoveToFront(loaded.vertices.buffer);
			size += loaded.vertices.size;
			loaded.vertices.source = alias.apverts;
			buffers.SetupAliasVertices(loaded.vertices);
			loaded.texCoords.size = alias.vertex_count * 2 * 2 * sizeof(float);
			loaded.texCoords.buffer = new SharedMemoryBuffer { };
			loaded.texCoords.buffer->CreateVertexBuffer(appState, loaded.texCoords.size);
			buffers.MoveToFront(loaded.texCoords.buffer);
			size += loaded.texCoords.size;
			loaded.texCoords.source = alias.texture_coordinates;
			loaded.texCoords.width = alias.width;
			loaded.texCoords.height = alias.height;
			buffers.SetupAliasTexCoords(loaded.texCoords);
			aliasVertexCache.insert({ alias.apverts, { loaded.vertices.buffer, loaded.texCoords.buffer } });
		}
		else
		{
			loaded.vertices.buffer = entry->second.vertices;
			loaded.texCoords.buffer = entry->second.texCoords;
		}
		previousApverts = alias.apverts;
		previousVertexBuffer = loaded.vertices.buffer;
		previousTexCoordsBuffer = loaded.texCoords.buffer;
	}
	else
	{
		loaded.vertices.buffer = previousVertexBuffer;
		loaded.texCoords.buffer = previousTexCoordsBuffer;
	}
	if (alias.is_host_colormap)
	{
		loaded.colormapped.colormap.size = 0;
		loaded.colormapped.colormap.texture = host_colormap;
	}
	else
	{
		loaded.colormapped.colormap.size = 16384;
		size += loaded.colormapped.colormap.size;
	}
	if (previousTexture != alias.data)
	{
		auto entry = aliasTextureCache.find(alias.data);
		if (entry == aliasTextureCache.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(alias.width, alias.height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, alias.width, alias.height, VK_FORMAT_R8_UINT, mipCount, 1, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			textures.MoveToFront(texture);
			loaded.colormapped.texture.size = alias.size;
			size += loaded.colormapped.texture.size;
			loaded.colormapped.texture.texture = texture;
			loaded.colormapped.texture.source = alias.data;
			loaded.colormapped.texture.mips = 1;
			textures.Setup(loaded.colormapped.texture);
			aliasTextureCache.insert({ alias.data, texture });
		}
		else
		{
			loaded.colormapped.texture.texture = entry->second;
			loaded.colormapped.texture.index = 0;
		}
		previousTexture = alias.data;
		previousSharedMemoryTexture = loaded.colormapped.texture.texture;
	}
	else
	{
		loaded.colormapped.texture.texture = previousSharedMemoryTexture;
		loaded.colormapped.texture.index = 0;
	}
	auto entry = aliasIndexCache.find(alias.aliashdr);
	if (entry == aliasIndexCache.end())
	{
		auto aliashdr = (aliashdr_t *)alias.aliashdr;
		auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
		auto triangle = (mtriangle_t *)((byte *)aliashdr + aliashdr->triangles);
		auto stverts = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);
		unsigned int maxIndex = 0;
		for (auto i = 0; i < mdl->numtris; i++)
		{
			auto v0 = triangle->vertindex[0];
			auto v1 = triangle->vertindex[1];
			auto v2 = triangle->vertindex[2];
			auto v0back = (((stverts[v0].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v1back = (((stverts[v1].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v2back = (((stverts[v2].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			maxIndex = std::max(maxIndex, (unsigned int)(v0 * 2 + (v0back ? 1 : 0)));
			maxIndex = std::max(maxIndex, (unsigned int)(v1 * 2 + (v1back ? 1 : 0)));
			maxIndex = std::max(maxIndex, (unsigned int)(v2 * 2 + (v2back ? 1 : 0)));
			triangle++;
		}
		if (maxIndex < UPPER_8BIT_LIMIT && appState.IndexTypeUInt8Enabled)
		{
			loaded.indices.size = alias.count;
			if (latestIndexBuffer8 == nullptr || usedInLatestIndexBuffer8 + loaded.indices.size > latestIndexBuffer8->size)
			{
				loaded.indices.indices.buffer = new SharedMemoryBuffer { };
				loaded.indices.indices.buffer->CreateIndexBuffer(appState, Constants::indexBuffer8BitSize);
				indexBuffers.MoveToFront(loaded.indices.indices.buffer);
				latestIndexBuffer8 = loaded.indices.indices.buffer;
				usedInLatestIndexBuffer8 = 0;
			}
			else
			{
				loaded.indices.indices.buffer = latestIndexBuffer8;
			}
			loaded.indices.indices.offset = usedInLatestIndexBuffer8;
			usedInLatestIndexBuffer8 += loaded.indices.size;
			size += loaded.indices.size;
			loaded.indices.source = alias.aliashdr;
			loaded.indices.indices.indexType = VK_INDEX_TYPE_UINT8_EXT;
			loaded.indices.indices.firstIndex = loaded.indices.indices.offset;
			indexBuffers.SetupAliasIndices8(loaded.indices);
		}
		else if (maxIndex < UPPER_16BIT_LIMIT)
		{
			loaded.indices.size = alias.count * sizeof(uint16_t);
			if (latestIndexBuffer16 == nullptr || usedInLatestIndexBuffer16 + loaded.indices.size > latestIndexBuffer16->size)
			{
				loaded.indices.indices.buffer = new SharedMemoryBuffer { };
				loaded.indices.indices.buffer->CreateIndexBuffer(appState, Constants::indexBuffer16BitSize);
				indexBuffers.MoveToFront(loaded.indices.indices.buffer);
				latestIndexBuffer16 = loaded.indices.indices.buffer;
				usedInLatestIndexBuffer16 = 0;
			}
			else
			{
				loaded.indices.indices.buffer = latestIndexBuffer16;
			}
			loaded.indices.indices.offset = usedInLatestIndexBuffer16;
			usedInLatestIndexBuffer16 += loaded.indices.size;
			size += loaded.indices.size;
			loaded.indices.source = alias.aliashdr;
			loaded.indices.indices.indexType = VK_INDEX_TYPE_UINT16;
			loaded.indices.indices.firstIndex = loaded.indices.indices.offset / 2;
			indexBuffers.SetupAliasIndices16(loaded.indices);
		}
		else
		{
			loaded.indices.size = alias.count * sizeof(uint32_t);
			if (latestIndexBuffer32 == nullptr || usedInLatestIndexBuffer32 + loaded.indices.size > latestIndexBuffer32->size)
			{
				loaded.indices.indices.buffer = new SharedMemoryBuffer { };
				loaded.indices.indices.buffer->CreateIndexBuffer(appState, Constants::indexBuffer32BitSize);
				indexBuffers.MoveToFront(loaded.indices.indices.buffer);
				latestIndexBuffer32 = loaded.indices.indices.buffer;
				usedInLatestIndexBuffer32 = 0;
			}
			else
			{
				loaded.indices.indices.buffer = latestIndexBuffer32;
			}
			loaded.indices.indices.offset = usedInLatestIndexBuffer32;
			usedInLatestIndexBuffer32 += loaded.indices.size;
			size += loaded.indices.size;
			loaded.indices.source = alias.aliashdr;
			loaded.indices.indices.indexType = VK_INDEX_TYPE_UINT32;
			loaded.indices.indices.firstIndex = loaded.indices.indices.offset / 4;
			indexBuffers.SetupAliasIndices32(loaded.indices);
		}
		aliasIndexCache.insert({ alias.aliashdr, loaded.indices.indices });
	}
	else
	{
		loaded.indices.indices = entry->second;
	}
	loaded.firstAttribute = alias.first_attribute;
	loaded.isHostColormap = alias.is_host_colormap;
	loaded.count = alias.count;
	for (auto j = 0; j < 3; j++)
	{
		for (auto i = 0; i < 4; i++)
		{
			loaded.transform[j][i] = alias.transform[j][i];
		}
	}
}

VkDeviceSize Scene::GetStagingBufferSize(AppState& appState, PerFrame& perFrame)
{
	surfaces.Allocate(d_lists.last_surface);
	surfacesColoredLights.Allocate(d_lists.last_surface_colored_lights);
	surfacesRGBA.Allocate(d_lists.last_surface_rgba);
	surfacesRGBAColoredLights.Allocate(d_lists.last_surface_rgba_colored_lights);
	surfacesRGBANoGlow.Allocate(d_lists.last_surface_rgba_no_glow);
	surfacesRGBANoGlowColoredLights.Allocate(d_lists.last_surface_rgba_no_glow_colored_lights);
	surfacesRotated.Allocate(d_lists.last_surface_rotated);
	surfacesRotatedColoredLights.Allocate(d_lists.last_surface_rotated_colored_lights);
	surfacesRotatedRGBA.Allocate(d_lists.last_surface_rotated_rgba);
	surfacesRotatedRGBAColoredLights.Allocate(d_lists.last_surface_rotated_rgba_colored_lights);
	surfacesRotatedRGBANoGlow.Allocate(d_lists.last_surface_rotated_rgba_no_glow);
	surfacesRotatedRGBANoGlowColoredLights.Allocate(d_lists.last_surface_rotated_rgba_no_glow_colored_lights);
	fences.Allocate(d_lists.last_fence);
	fencesColoredLights.Allocate(d_lists.last_fence_colored_lights);
	fencesRGBA.Allocate(d_lists.last_fence_rgba);
	fencesRGBANoGlow.Allocate(d_lists.last_fence_rgba_no_glow);
	fencesRotated.Allocate(d_lists.last_fence_rotated);
	fencesRotatedColoredLights.Allocate(d_lists.last_fence_rotated_colored_lights);
	fencesRotatedRGBA.Allocate(d_lists.last_fence_rotated_rgba);
	fencesRotatedRGBANoGlow.Allocate(d_lists.last_fence_rotated_rgba_no_glow);
	turbulent.Allocate(d_lists.last_turbulent);
	turbulentRGBA.Allocate(d_lists.last_turbulent_rgba);
	turbulentLit.Allocate(d_lists.last_turbulent_lit);
	turbulentColoredLights.Allocate(d_lists.last_turbulent_colored_lights);
	turbulentRGBALit.Allocate(d_lists.last_turbulent_rgba_lit);
	turbulentRotated.Allocate(d_lists.last_turbulent_rotated);
	sprites.Allocate(d_lists.last_sprite);
	alias.Allocate(d_lists.last_alias);
	viewmodel.Allocate(d_lists.last_viewmodel);
	lastParticle = d_lists.last_particle_color;
	lastColoredIndex8 = d_lists.last_colored_index8;
	lastColoredIndex16 = d_lists.last_colored_index16;
	lastColoredIndex32 = d_lists.last_colored_index32;
	lastSky = d_lists.last_sky;
	lastSkyRGBA = d_lists.last_sky_rgba;
	appState.FromEngine.vieworg0 = d_lists.vieworg0;
	appState.FromEngine.vieworg1 = d_lists.vieworg1;
	appState.FromEngine.vieworg2 = d_lists.vieworg2;
	appState.FromEngine.vright0 = d_lists.vright0;
	appState.FromEngine.vright1 = d_lists.vright1;
	appState.FromEngine.vright2 = d_lists.vright2;
	appState.FromEngine.vup0 = d_lists.vup0;
	appState.FromEngine.vup1 = d_lists.vup1;
	appState.FromEngine.vup2 = d_lists.vup2;
	VkDeviceSize size = 0;
	if (palette == nullptr || paletteChangedFrame != pal_changed)
	{
		SharedMemoryBuffer* newPalette = nullptr;
		bool skip = true;
		for (auto p = &palette; *p != nullptr; )
		{
			if (skip)
			{
				p = &(*p)->next;
				skip = false;
			}
			else
			{
				(*p)->unusedCount++;
				if ((*p)->unusedCount >= Constants::maxUnusedCount)
				{
					auto next = (*p)->next;
					newPalette = (*p);
					newPalette->unusedCount = 0;
					(*p) = next;
					break;
				}
				else
				{
					p = &(*p)->next;
				}
			}
		}
		if (newPalette == nullptr)
		{
			newPalette = new SharedMemoryBuffer { };
			newPalette->CreateUniformBuffer(appState, 256 * 4 * sizeof(float));
		}
		newPalette->next = palette;
		palette = newPalette;
		paletteSize = palette->size;
		size += paletteSize;
		newPalette = nullptr;
		skip = true;
		for (auto p = &neutralPalette; *p != nullptr; )
		{
			if (skip)
			{
				p = &(*p)->next;
				skip = false;
			}
			else
			{
				(*p)->unusedCount++;
				if ((*p)->unusedCount >= Constants::maxUnusedCount)
				{
					auto next = (*p)->next;
					newPalette = (*p);
					newPalette->unusedCount = 0;
					(*p) = next;
					break;
				}
				else
				{
					p = &(*p)->next;
				}
			}
		}
		if (newPalette == nullptr)
		{
			newPalette = new SharedMemoryBuffer { };
			newPalette->CreateUniformBuffer(appState, 256 * 4 * sizeof(float));
		}
		newPalette->next = neutralPalette;
		neutralPalette = newPalette;
		size += paletteSize;
		paletteChangedFrame = pal_changed;
	}
	if (!host_colormap.empty() && perFrame.colormap == nullptr)
	{
		perFrame.colormap = new Texture();
		perFrame.colormap->Create(appState, 256, 64, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		colormapSize = 16384;
		size += colormapSize;
	}
	surfaces.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(surfaces.sorted);
	for (auto i = 0; i <= surfaces.last; i++)
	{
		auto& loaded = surfaces.loaded[i];
		GetStagingBufferSize(appState, d_lists.surfaces[i], loaded, size);
		sorted.Sort(appState, loaded, i, surfaces.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(surfaces.sorted);
	surfacesColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(surfacesColoredLights.sorted);
	for (auto i = 0; i <= surfacesColoredLights.last; i++)
	{
		auto& loaded = surfacesColoredLights.loaded[i];
		GetStagingBufferSize(appState, d_lists.surfaces_colored_lights[i], loaded, size);
		sorted.Sort(appState, loaded, i, surfacesColoredLights.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(surfacesColoredLights.sorted);
	surfacesRGBA.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	previousGlowTexture = nullptr;
	sorted.Initialize(surfacesRGBA.sorted);
	for (auto i = 0; i <= surfacesRGBA.last; i++)
	{
		auto& loaded = surfacesRGBA.loaded[i];
		GetStagingBufferSize(appState, d_lists.surfaces_rgba[i], loaded, size);
		sorted.Sort(appState, loaded, i, surfacesRGBA.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 20 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(surfacesRGBA.sorted);
	surfacesRGBAColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	previousGlowTexture = nullptr;
	sorted.Initialize(surfacesRGBAColoredLights.sorted);
	for (auto i = 0; i <= surfacesRGBAColoredLights.last; i++)
	{
		auto& loaded = surfacesRGBAColoredLights.loaded[i];
		GetStagingBufferSize(appState, d_lists.surfaces_rgba_colored_lights[i], loaded, size);
		sorted.Sort(appState, loaded, i, surfacesRGBAColoredLights.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 20 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(surfacesRGBAColoredLights.sorted);
	surfacesRGBANoGlow.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(surfacesRGBANoGlow.sorted);
	for (auto i = 0; i <= surfacesRGBANoGlow.last; i++)
	{
		auto& loaded = surfacesRGBANoGlow.loaded[i];
		GetStagingBufferSizeRGBANoGlow(appState, d_lists.surfaces_rgba_no_glow[i], loaded, size);
		sorted.Sort(appState, loaded, i, surfacesRGBANoGlow.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(surfacesRGBANoGlow.sorted);
	surfacesRGBANoGlowColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(surfacesRGBANoGlowColoredLights.sorted);
	for (auto i = 0; i <= surfacesRGBANoGlowColoredLights.last; i++)
	{
		auto& loaded = surfacesRGBANoGlowColoredLights.loaded[i];
		GetStagingBufferSizeRGBANoGlow(appState, d_lists.surfaces_rgba_no_glow_colored_lights[i], loaded, size);
		sorted.Sort(appState, loaded, i, surfacesRGBANoGlowColoredLights.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(surfacesRGBANoGlowColoredLights.sorted);
	surfacesRotated.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(surfacesRotated.sorted);
	for (auto i = 0; i <= surfacesRotated.last; i++)
	{
		auto& loaded = surfacesRotated.loaded[i];
		GetStagingBufferSize(appState, d_lists.surfaces_rotated[i], loaded, size);
		sorted.Sort(appState, loaded, i, surfacesRotated.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 24 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(surfacesRotated.sorted);
	surfacesRotatedColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(surfacesRotatedColoredLights.sorted);
	for (auto i = 0; i <= surfacesRotatedColoredLights.last; i++)
	{
		auto& loaded = surfacesRotatedColoredLights.loaded[i];
		GetStagingBufferSize(appState, d_lists.surfaces_rotated_colored_lights[i], loaded, size);
		sorted.Sort(appState, loaded, i, surfacesRotatedColoredLights.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 24 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(surfacesRotatedColoredLights.sorted);
	surfacesRotatedRGBA.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	previousGlowTexture = nullptr;
	sorted.Initialize(surfacesRotatedRGBA.sorted);
	for (auto i = 0; i <= surfacesRotatedRGBA.last; i++)
	{
		auto& loaded = surfacesRotatedRGBA.loaded[i];
		GetStagingBufferSize(appState, d_lists.surfaces_rotated_rgba[i], loaded, size);
		sorted.Sort(appState, loaded, i, surfacesRotatedRGBA.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 24 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(surfacesRotatedRGBA.sorted);
	surfacesRotatedRGBAColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	previousGlowTexture = nullptr;
	sorted.Initialize(surfacesRotatedRGBAColoredLights.sorted);
	for (auto i = 0; i <= surfacesRotatedRGBAColoredLights.last; i++)
	{
		auto& loaded = surfacesRotatedRGBAColoredLights.loaded[i];
		GetStagingBufferSize(appState, d_lists.surfaces_rotated_rgba_colored_lights[i], loaded, size);
		sorted.Sort(appState, loaded, i, surfacesRotatedRGBAColoredLights.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 24 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(surfacesRotatedRGBAColoredLights.sorted);
	surfacesRotatedRGBANoGlow.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(surfacesRotatedRGBANoGlow.sorted);
	for (auto i = 0; i <= surfacesRotatedRGBANoGlow.last; i++)
	{
		auto& loaded = surfacesRotatedRGBANoGlow.loaded[i];
		GetStagingBufferSizeRGBANoGlow(appState, d_lists.surfaces_rotated_rgba_no_glow[i], loaded, size);
		sorted.Sort(appState, loaded, i, surfacesRotatedRGBANoGlow.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 24 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(surfacesRotatedRGBANoGlow.sorted);
	surfacesRotatedRGBANoGlowColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(surfacesRotatedRGBANoGlowColoredLights.sorted);
	for (auto i = 0; i <= surfacesRotatedRGBANoGlowColoredLights.last; i++)
	{
		auto& loaded = surfacesRotatedRGBANoGlowColoredLights.loaded[i];
		GetStagingBufferSizeRGBANoGlow(appState, d_lists.surfaces_rotated_rgba_no_glow_colored_lights[i], loaded, size);
		sorted.Sort(appState, loaded, i, surfacesRotatedRGBANoGlowColoredLights.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 24 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(surfacesRotatedRGBANoGlowColoredLights.sorted);
	fences.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(fences.sorted);
	for (auto i = 0; i <= fences.last; i++)
	{
		auto& loaded = fences.loaded[i];
		GetStagingBufferSize(appState, d_lists.fences[i], loaded, size);
		sorted.Sort(appState, loaded, i, fences.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(fences.sorted);
	fencesColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(fencesColoredLights.sorted);
	for (auto i = 0; i <= fencesColoredLights.last; i++)
	{
		auto& loaded = fencesColoredLights.loaded[i];
		GetStagingBufferSize(appState, d_lists.fences_colored_lights[i], loaded, size);
		sorted.Sort(appState, loaded, i, fencesColoredLights.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(fencesColoredLights.sorted);
	fencesRGBA.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	previousGlowTexture = nullptr;
	sorted.Initialize(fencesRGBA.sorted);
	for (auto i = 0; i <= fencesRGBA.last; i++)
	{
		auto& loaded = fencesRGBA.loaded[i];
		GetStagingBufferSize(appState, d_lists.fences_rgba[i], loaded, size);
		sorted.Sort(appState, loaded, i, fencesRGBA.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 20 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(fencesRGBA.sorted);
	fencesRGBANoGlow.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(fencesRGBANoGlow.sorted);
	for (auto i = 0; i <= fencesRGBANoGlow.last; i++)
	{
		auto& loaded = fencesRGBANoGlow.loaded[i];
		GetStagingBufferSizeRGBANoGlow(appState, d_lists.fences_rgba_no_glow[i], loaded, size);
		sorted.Sort(appState, loaded, i, fencesRGBANoGlow.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(fencesRGBANoGlow.sorted);
	fencesRotated.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(fencesRotated.sorted);
	for (auto i = 0; i <= fencesRotated.last; i++)
	{
		auto& loaded = fencesRotated.loaded[i];
		GetStagingBufferSize(appState, d_lists.fences_rotated[i], loaded, size);
		sorted.Sort(appState, loaded, i, fencesRotated.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 24 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(fencesRotated.sorted);
	fencesRotatedColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(fencesRotatedColoredLights.sorted);
	for (auto i = 0; i <= fencesRotatedColoredLights.last; i++)
	{
		auto& loaded = fencesRotatedColoredLights.loaded[i];
		GetStagingBufferSize(appState, d_lists.fences_rotated_colored_lights[i], loaded, size);
		sorted.Sort(appState, loaded, i, fencesRotatedColoredLights.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 24 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(fencesRotatedColoredLights.sorted);
	fencesRotatedRGBA.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	previousGlowTexture = nullptr;
	sorted.Initialize(fencesRotatedRGBA.sorted);
	for (auto i = 0; i <= fencesRotatedRGBA.last; i++)
	{
		auto& loaded = fencesRotatedRGBA.loaded[i];
		GetStagingBufferSize(appState, d_lists.fences_rotated_rgba[i], loaded, size);
		sorted.Sort(appState, loaded, i, fencesRotatedRGBA.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 24 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(fencesRotatedRGBA.sorted);
	fencesRotatedRGBANoGlow.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(fencesRotatedRGBANoGlow.sorted);
	for (auto i = 0; i <= fencesRotatedRGBANoGlow.last; i++)
	{
		auto& loaded = fencesRotatedRGBANoGlow.loaded[i];
		GetStagingBufferSizeRGBANoGlow(appState, d_lists.fences_rotated_rgba_no_glow[i], loaded, size);
		sorted.Sort(appState, loaded, i, fencesRotatedRGBANoGlow.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 24 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(fencesRotatedRGBANoGlow.sorted);
	turbulent.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(turbulent.sorted);
	for (auto i = 0; i <= turbulent.last; i++)
	{
		auto& loaded = turbulent.loaded[i];
		GetStagingBufferSize(appState, d_lists.turbulent[i], loaded, size);
		sorted.Sort(appState, loaded, i, turbulent.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 12 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(turbulent.sorted);
	turbulentRGBA.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(turbulentRGBA.sorted);
	for (auto i = 0; i <= turbulentRGBA.last; i++)
	{
		auto& loaded = turbulentRGBA.loaded[i];
		GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rgba[i], loaded, size);
		sorted.Sort(appState, loaded, i, turbulentRGBA.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 12 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(turbulentRGBA.sorted);
	turbulentLit.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(turbulentLit.sorted);
	for (auto i = 0; i <= turbulentLit.last; i++)
	{
		auto& loaded = turbulentLit.loaded[i];
		GetStagingBufferSize(appState, d_lists.turbulent_lit[i], loaded, size);
		sorted.Sort(appState, loaded, i, turbulentLit.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(turbulentLit.sorted);
	turbulentColoredLights.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(turbulentColoredLights.sorted);
	for (auto i = 0; i <= turbulentColoredLights.last; i++)
	{
		auto& loaded = turbulentColoredLights.loaded[i];
		GetStagingBufferSize(appState, d_lists.turbulent_colored_lights[i], loaded, size);
		sorted.Sort(appState, loaded, i, turbulentColoredLights.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(turbulentColoredLights.sorted);
	turbulentRGBALit.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(turbulentRGBALit.sorted);
	for (auto i = 0; i <= turbulentRGBALit.last; i++)
	{
		auto& loaded = turbulentRGBALit.loaded[i];
		GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rgba_lit[i], loaded, size);
		sorted.Sort(appState, loaded, i, turbulentRGBALit.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(turbulentRGBALit.sorted);
	turbulentRotated.SetBases(sortedVerticesSize, sortedAttributesSize, sortedIndicesCount);
	previousTexture = nullptr;
	sorted.Initialize(turbulentRotated.sorted);
	for (auto i = 0; i <= turbulentRotated.last; i++)
	{
		auto& loaded = turbulentRotated.loaded[i];
		GetStagingBufferSize(appState, d_lists.turbulent_rotated[i], loaded, size);
		sorted.Sort(appState, loaded, i, turbulentRotated.sorted);
		sortedVerticesSize += (loaded.count * 3 * sizeof(float));
		sortedAttributesSize += (loaded.count * 20 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(turbulentRotated.sorted);
	previousTexture = nullptr;
	for (auto i = 0; i <= sprites.last; i++)
	{
		GetStagingBufferSize(appState, d_lists.sprites[i], sprites.loaded[i], size);
	}
	previousApverts = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= alias.last; i++)
	{
		GetStagingBufferSize(appState, d_lists.alias[i], alias.loaded[i], perFrame.colormap, size);
	}
	previousApverts = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= viewmodel.last; i++)
	{
		GetStagingBufferSize(appState, d_lists.viewmodels[i], viewmodel.loaded[i], perFrame.colormap, size);
	}
	if (lastSky >= 0)
	{
		loadedSky.width = d_lists.sky[0].width;
		loadedSky.height = d_lists.sky[0].height;
		loadedSky.size = d_lists.sky[0].size;
		loadedSky.data = d_lists.sky[0].data;
		loadedSky.firstVertex = d_lists.sky[0].first_vertex;
		loadedSky.count = d_lists.sky[0].count;
		if (perFrame.sky == nullptr)
		{
			perFrame.sky = new Texture();
			perFrame.sky->Create(appState, loadedSky.width, loadedSky.height, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		size += loadedSky.size;
	}
	if (lastSkyRGBA >= 0)
	{
		loadedSkyRGBA.width = d_lists.sky_rgba[0].width;
		loadedSkyRGBA.height = d_lists.sky_rgba[0].height;
		loadedSkyRGBA.size = d_lists.sky_rgba[0].size;
		loadedSkyRGBA.data = d_lists.sky_rgba[0].data;
		loadedSkyRGBA.firstVertex = d_lists.sky_rgba[0].first_vertex;
		loadedSkyRGBA.count = d_lists.sky_rgba[0].count;
		if (perFrame.skyRGBA == nullptr)
		{
			perFrame.skyRGBA = new Texture();
			perFrame.skyRGBA->Create(appState, loadedSkyRGBA.width * 2, loadedSkyRGBA.height, VK_FORMAT_R8G8B8A8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		size += loadedSkyRGBA.size;
	}
	floorVerticesSize = 0;
	if (appState.Mode != AppWorldMode)
	{
		floorVerticesSize += 3 * 4 * sizeof(float);
	}
	if (appState.Focused && (key_dest == key_console || key_dest == key_menu || appState.Mode != AppWorldMode))
	{
		if (appState.LeftController.PoseIsValid)
		{
			controllerVerticesSize += 2 * 8 * 3 * sizeof(float);
		}
		if (appState.RightController.PoseIsValid)
		{
			controllerVerticesSize += 2 * 8 * 3 * sizeof(float);
		}
	}
	texturedVerticesSize = (d_lists.last_textured_vertex + 1) * sizeof(float);
	particlePositionsSize = (d_lists.last_particle_position + 1) * sizeof(float);
	coloredVerticesSize = (d_lists.last_colored_vertex + 1) * sizeof(float);
	verticesSize = floorVerticesSize + controllerVerticesSize + texturedVerticesSize + particlePositionsSize + coloredVerticesSize;
	if (verticesSize + sortedVerticesSize > 0)
	{
		perFrame.vertices = perFrame.cachedVertices.GetVertexBuffer(appState, verticesSize + sortedVerticesSize);
	}
	size += verticesSize;
	size += sortedVerticesSize;
	floorAttributesSize = 0;
	if (appState.Mode != AppWorldMode)
	{
		floorAttributesSize += 2 * 4 * sizeof(float);
	}
	controllerAttributesSize = 0;
	if (controllerVerticesSize > 0)
	{
		if (appState.LeftController.PoseIsValid)
		{
			controllerAttributesSize += 2 * 8 * 2 * sizeof(float);
		}
		if (appState.RightController.PoseIsValid)
		{
			controllerAttributesSize += 2 * 8 * 2 * sizeof(float);
		}
	}
	texturedAttributesSize = (d_lists.last_textured_attribute + 1) * sizeof(float);
	colormappedLightsSize = (d_lists.last_colormapped_attribute + 1) * sizeof(float);
	attributesSize = floorAttributesSize + controllerAttributesSize + texturedAttributesSize + colormappedLightsSize;
	if (attributesSize + sortedAttributesSize> 0)
	{
		perFrame.attributes = perFrame.cachedAttributes.GetVertexBuffer(appState, attributesSize + sortedAttributesSize);
	}
	size += attributesSize;
	size += sortedAttributesSize;
	particleColorsSize = (d_lists.last_particle_color + 1) * sizeof(float);
	coloredColorsSize = (d_lists.last_colored_color + 1) * sizeof(float);
	colorsSize = particleColorsSize + coloredColorsSize;
	if (colorsSize > 0)
	{
		perFrame.colors = perFrame.cachedColors.GetVertexBuffer(appState, colorsSize);
	}
	size += colorsSize;
	floorIndicesSize = 0;
	if (appState.Mode != AppWorldMode)
	{
		floorIndicesSize = 4;
	}
	controllerIndicesSize = 0;
	if (controllerVerticesSize > 0)
	{
		if (appState.LeftController.PoseIsValid)
		{
			controllerIndicesSize += 2 * 36;
		}
		if (appState.RightController.PoseIsValid)
		{
			controllerIndicesSize += 2 * 36;
		}
	}
	coloredIndices8Size = lastColoredIndex8 + 1;
	coloredIndices16Size = (lastColoredIndex16 + 1) * sizeof(uint16_t);

	// This is only intended as an approximation - real 16-bit index limits are not even close to being reached before this happens:
	if (sortedIndicesCount < UPPER_16BIT_LIMIT)
	{
		sortedIndices16Size = sortedIndicesCount * sizeof(uint16_t);
		sortedIndices32Size = 0;
		surfaces.ScaleIndexBase(sizeof(uint16_t));
		surfacesColoredLights.ScaleIndexBase(sizeof(uint16_t));
		surfacesRGBA.ScaleIndexBase(sizeof(uint16_t));
		surfacesRGBAColoredLights.ScaleIndexBase(sizeof(uint16_t));
		surfacesRGBANoGlow.ScaleIndexBase(sizeof(uint16_t));
		surfacesRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint16_t));
		surfacesRotated.ScaleIndexBase(sizeof(uint16_t));
		surfacesRotatedColoredLights.ScaleIndexBase(sizeof(uint16_t));
		surfacesRotatedRGBA.ScaleIndexBase(sizeof(uint16_t));
		surfacesRotatedRGBAColoredLights.ScaleIndexBase(sizeof(uint16_t));
		surfacesRotatedRGBANoGlow.ScaleIndexBase(sizeof(uint16_t));
		surfacesRotatedRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint16_t));
		fences.ScaleIndexBase(sizeof(uint16_t));
		fencesColoredLights.ScaleIndexBase(sizeof(uint16_t));
		fencesRGBA.ScaleIndexBase(sizeof(uint16_t));
		fencesRGBANoGlow.ScaleIndexBase(sizeof(uint16_t));
		fencesRotated.ScaleIndexBase(sizeof(uint16_t));
		fencesRotatedColoredLights.ScaleIndexBase(sizeof(uint16_t));
		fencesRotatedRGBA.ScaleIndexBase(sizeof(uint16_t));
		fencesRotatedRGBANoGlow.ScaleIndexBase(sizeof(uint16_t));
		turbulent.ScaleIndexBase(sizeof(uint16_t));
		turbulentRGBA.ScaleIndexBase(sizeof(uint16_t));
		turbulentLit.ScaleIndexBase(sizeof(uint16_t));
		turbulentColoredLights.ScaleIndexBase(sizeof(uint16_t));
		turbulentRGBALit.ScaleIndexBase(sizeof(uint16_t));
		turbulentRotated.ScaleIndexBase(sizeof(uint16_t));
	}
	else
	{
		sortedIndices16Size = 0;
		sortedIndices32Size = sortedIndicesCount * sizeof(uint32_t);
		surfaces.ScaleIndexBase(sizeof(uint32_t));
		surfacesColoredLights.ScaleIndexBase(sizeof(uint32_t));
		surfacesRGBA.ScaleIndexBase(sizeof(uint32_t));
		surfacesRGBAColoredLights.ScaleIndexBase(sizeof(uint32_t));
		surfacesRGBANoGlow.ScaleIndexBase(sizeof(uint32_t));
		surfacesRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint32_t));
		surfacesRotated.ScaleIndexBase(sizeof(uint32_t));
		surfacesRotatedColoredLights.ScaleIndexBase(sizeof(uint32_t));
		surfacesRotatedRGBA.ScaleIndexBase(sizeof(uint32_t));
		surfacesRotatedRGBAColoredLights.ScaleIndexBase(sizeof(uint32_t));
		surfacesRotatedRGBANoGlow.ScaleIndexBase(sizeof(uint32_t));
		surfacesRotatedRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint32_t));
		fences.ScaleIndexBase(sizeof(uint32_t));
		fencesColoredLights.ScaleIndexBase(sizeof(uint32_t));
		fencesRGBA.ScaleIndexBase(sizeof(uint32_t));
		fencesRGBANoGlow.ScaleIndexBase(sizeof(uint32_t));
		fencesRotated.ScaleIndexBase(sizeof(uint32_t));
		fencesRotatedColoredLights.ScaleIndexBase(sizeof(uint32_t));
		fencesRotatedRGBA.ScaleIndexBase(sizeof(uint32_t));
		fencesRotatedRGBANoGlow.ScaleIndexBase(sizeof(uint32_t));
		turbulent.ScaleIndexBase(sizeof(uint32_t));
		turbulentRGBA.ScaleIndexBase(sizeof(uint32_t));
		turbulentLit.ScaleIndexBase(sizeof(uint32_t));
		turbulentColoredLights.ScaleIndexBase(sizeof(uint32_t));
		turbulentRGBALit.ScaleIndexBase(sizeof(uint32_t));
		turbulentRotated.ScaleIndexBase(sizeof(uint32_t));
	}

	if (appState.IndexTypeUInt8Enabled)
	{
		indices8Size = floorIndicesSize + controllerIndicesSize + coloredIndices8Size;
		indices16Size = coloredIndices16Size;
	}
	else
	{
		floorIndicesSize *= sizeof(uint16_t);
		controllerIndicesSize *= sizeof(uint16_t);
		indices8Size = 0;
		indices16Size = floorIndicesSize + controllerIndicesSize + coloredIndices8Size * sizeof(uint16_t) + coloredIndices16Size;
	}

	if (indices8Size > 0)
	{
		perFrame.indices8 = perFrame.cachedIndices8.GetIndexBuffer(appState, indices8Size);
	}
	size += indices8Size;

	if (indices16Size + sortedIndices16Size > 0)
	{
		perFrame.indices16 = perFrame.cachedIndices16.GetIndexBuffer(appState, indices16Size + sortedIndices16Size);
	}
	size += indices16Size;
	size += sortedIndices16Size;

	indices32Size = (lastColoredIndex32 + 1) * sizeof(uint32_t);
	if (indices32Size + sortedIndices32Size > 0)
	{
		perFrame.indices32 = perFrame.cachedIndices32.GetIndexBuffer(appState, indices32Size + sortedIndices32Size);
	}
	size += indices32Size;
	size += sortedIndices32Size;

	// Add extra space (and also realign to a 4-byte boundary), due to potential alignment issues among 8, 16 and 32-bit index data:
	if (size > 0)
	{
		size += 32;
		while (size % 4 != 0)
		{
			size++;
		}
	}

	return size;
}

void Scene::Reset()
{
	D_ResetLists();
	aliasTextureCache.clear();
	spriteCache.clear();
	surfaceTextureCache.clear();
	textures.DisposeFront();
	for (auto& cached : surfaceRGBATextures)
	{
		cached.DisposeFront();
	}
	for (auto& cached : surfaceTextures)
	{
		cached.DisposeFront();
	}
	while (deletedLightmapRGBATextures != nullptr)
	{
		auto lightmapTexture = deletedLightmapRGBATextures;
		deletedLightmapRGBATextures = deletedLightmapRGBATextures->next;
		delete lightmapTexture;
	}
	lightmapsRGBA.DisposeFront();
	while (deletedLightmapTextures != nullptr)
	{
		auto lightmapTexture = deletedLightmapTextures;
		deletedLightmapTextures = deletedLightmapTextures->next;
		delete lightmapTexture;
	}
	lightmaps.DisposeFront();
	indexBuffers.DisposeFront();
	buffers.DisposeFront();
	for (SharedMemoryBuffer* p = neutralPalette, *next; p != nullptr; p = next)
	{
		next = p->next;
		p->next = oldPalettes;
		oldPalettes = p;
	}
	neutralPalette = nullptr;
	for (SharedMemoryBuffer* p = palette, *next; p != nullptr; p = next)
	{
		next = p->next;
		p->next = oldPalettes;
		oldPalettes = p;
	}
	palette = nullptr;
	latestTextureDescriptorSets = nullptr;
	latestIndexBuffer32 = nullptr;
	usedInLatestIndexBuffer32 = 0;
	latestIndexBuffer16 = nullptr;
	usedInLatestIndexBuffer16 = 0;
	latestIndexBuffer8 = nullptr;
	usedInLatestIndexBuffer8 = 0;
	latestMemory.clear();
	Skybox::MoveToPrevious(*this);
	aliasIndexCache.clear();
	aliasVertexCache.clear();
	surfaceVertexCache.clear();
}
