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

void Scene::Create(AppState& appState, VkCommandBufferAllocateInfo& commandBufferAllocateInfo, VkCommandBuffer& setupCommandBuffer, VkCommandBufferBeginInfo& commandBufferBeginInfo, VkSubmitInfo& setupSubmitInfo, struct android_app* app)
{
	appState.ConsoleWidth = 320;
	appState.ConsoleHeight = 200;
	appState.ScreenWidth = appState.ConsoleWidth * Constants::screenToConsoleMultiplier;
	appState.ScreenHeight = appState.ConsoleHeight * Constants::screenToConsoleMultiplier;
	
	XrSwapchainCreateInfo swapchainCreateInfo { XR_TYPE_SWAPCHAIN_CREATE_INFO };
	swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
	swapchainCreateInfo.format = Constants::colorFormat;
	swapchainCreateInfo.sampleCount = appState.SwapchainSampleCount;
	swapchainCreateInfo.faceCount = 1;
	swapchainCreateInfo.arraySize = 1;
	swapchainCreateInfo.mipCount = 1;

	swapchainCreateInfo.width = appState.ScreenWidth;
	swapchainCreateInfo.height = appState.ScreenHeight;
	CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &appState.Screen.Swapchain));

	uint32_t imageCount;
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Screen.Swapchain, 0, &imageCount, nullptr));

	std::vector<XrSwapchainImageVulkan2KHR> images(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Screen.Swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)images.data()));
	
	appState.Screen.PerImage.resize(imageCount);
	for (auto i = 0; i < imageCount; i++)
	{
		auto& perImage = appState.Screen.PerImage[i];
		perImage.image = images[i].image;

		CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &perImage.commandBuffer));

		perImage.submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		perImage.submitInfo.commandBufferCount = 1;
		perImage.submitInfo.pCommandBuffers = &perImage.commandBuffer;
	}

	appState.ScreenData.resize(swapchainCreateInfo.width * swapchainCreateInfo.height, 255 << 24);
	ImageAsset play;
	play.Open("play.png", app);
	CopyImage(appState, play.image, appState.ScreenData.data() + ((appState.ScreenHeight - play.height) * appState.ScreenWidth + appState.ScreenWidth - play.width) / 2, play.width, play.height);
	play.Close();
	AddBorder(appState, appState.ScreenData);

	for (auto& perImage : appState.Screen.PerImage)
	{
		perImage.stagingBuffer.CreateStagingBuffer(appState, appState.ScreenData.size() * sizeof(uint32_t));
		CHECK_VKCMD(vkMapMemory(appState.Device, perImage.stagingBuffer.memory, 0, VK_WHOLE_SIZE, 0, &perImage.stagingBuffer.mapped));
	}

	VkImageCreateInfo imageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = Constants::colorFormat;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

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
		createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo);
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
		createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo);
		CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &statusBarTexture.memory));
		CHECK_VKCMD(vkBindImageMemory(appState.Device, statusBarTexture.image, statusBarTexture.memory, 0));
	}

	swapchainCreateInfo.width = appState.ScreenWidth;
	swapchainCreateInfo.height = appState.ScreenHeight / 2;
	CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &appState.Keyboard.Screen.Swapchain));
	
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Keyboard.Screen.Swapchain, 0, &imageCount, nullptr));
	
	images.resize(imageCount,  { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Keyboard.Screen.Swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)images.data()));

	appState.Keyboard.Screen.PerImage.resize(imageCount);
	appState.KeyboardTextures.resize(imageCount);
	for (auto i = 0; i < imageCount; i++)
	{
		auto& perImage = appState.Keyboard.Screen.PerImage[i];
		perImage.image = images[i].image;

		CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &perImage.commandBuffer));

		perImage.submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		perImage.submitInfo.commandBufferCount = 1;
		perImage.submitInfo.pCommandBuffers = &perImage.commandBuffer;

		perImage.stagingBuffer.CreateStagingBuffer(appState, appState.ConsoleWidth * appState.ConsoleHeight / 2 * sizeof(uint32_t));

		CHECK_VKCMD(vkMapMemory(appState.Device, perImage.stagingBuffer.memory, 0, VK_WHOLE_SIZE, 0, &perImage.stagingBuffer.mapped));
	
		auto& keyboardTexture = appState.KeyboardTextures[i];
		keyboardTexture.width = appState.ConsoleWidth;
		keyboardTexture.height = appState.ConsoleHeight / 2;
		keyboardTexture.mipCount = 1;
		keyboardTexture.layerCount = 1;
		imageCreateInfo.extent.width = keyboardTexture.width;
		imageCreateInfo.extent.height = keyboardTexture.height;
		imageCreateInfo.mipLevels = keyboardTexture.mipCount;
		imageCreateInfo.arrayLayers = keyboardTexture.layerCount;
		CHECK_VKCMD(vkCreateImage(appState.Device, &imageCreateInfo, nullptr, &keyboardTexture.image));

		vkGetImageMemoryRequirements(appState.Device, keyboardTexture.image, &memoryRequirements);

		createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo);

		CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &keyboardTexture.memory));
		CHECK_VKCMD(vkBindImageMemory(appState.Device, keyboardTexture.image, keyboardTexture.memory, 0));
	}

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
	stagingBuffer.CreateStagingBuffer(appState, stagingBufferSize);
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
	swapchainCreateInfo.sampleCount = appState.SwapchainSampleCount;
	swapchainCreateInfo.width = 450;
	swapchainCreateInfo.height = 150;
	CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &appState.LeftArrowsSwapchain));

	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.LeftArrowsSwapchain, 0, &imageCount, nullptr));

	images.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.LeftArrowsSwapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)images.data()));

	VkBufferCreateInfo bufferCreateInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCreateInfo.size = swapchainCreateInfo.width * swapchainCreateInfo.height * 4;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkBuffer buffer;
	VkDeviceMemory memoryBlock;
	VkImageMemoryBarrier imageMemoryBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
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

	imageMemoryBarrier.image = images[swapchainImageIndex].image;

	CHECK_VKCMD(vkCreateBuffer(appState.Device, &bufferCreateInfo, nullptr, &buffer));
	vkGetBufferMemoryRequirements(appState.Device, buffer, &memoryRequirements);
	createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memoryAllocateInfo);
	CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &memoryBlock));
	CHECK_VKCMD(vkBindBufferMemory(appState.Device, buffer, memoryBlock, 0));
	void* mapped;
	CHECK_VKCMD(vkMapMemory(appState.Device, memoryBlock, 0, memoryRequirements.size, 0, &mapped));

	ImageAsset leftArrows;
	leftArrows.Open("leftarrows.png", app);
	memcpy(mapped, leftArrows.image, leftArrows.width * leftArrows.height * leftArrows.components);
	
	vkUnmapMemory(appState.Device, memoryBlock);
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	region.imageSubresource.baseArrayLayer = 0;
	vkCmdCopyBufferToImage(setupCommandBuffer, buffer, images[swapchainImageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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

	images.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.RightArrowsSwapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)images.data()));
 
 	bufferCreateInfo.size = swapchainCreateInfo.width * swapchainCreateInfo.height * 4;
	region.imageExtent.width = swapchainCreateInfo.width;
	region.imageExtent.height = swapchainCreateInfo.height;

	CHECK_XRCMD(xrAcquireSwapchainImage(appState.RightArrowsSwapchain, nullptr, &swapchainImageIndex));

	CHECK_XRCMD(xrWaitSwapchainImage(appState.RightArrowsSwapchain, &waitInfo));

	CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &setupCommandBuffer));
	CHECK_VKCMD(vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));

	imageMemoryBarrier.image = images[swapchainImageIndex].image;

	CHECK_VKCMD(vkCreateBuffer(appState.Device, &bufferCreateInfo, nullptr, &buffer));
	vkGetBufferMemoryRequirements(appState.Device, buffer, &memoryRequirements);
	createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memoryAllocateInfo);
	CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &memoryBlock));
	CHECK_VKCMD(vkBindBufferMemory(appState.Device, buffer, memoryBlock, 0));

	CHECK_VKCMD(vkMapMemory(appState.Device, memoryBlock, 0, memoryRequirements.size, 0, &mapped));

	ImageAsset rightArrows;
	rightArrows.Open("rightarrows.png", app);
	memcpy(mapped, rightArrows.image, rightArrows.width * rightArrows.height * rightArrows.components);

	vkUnmapMemory(appState.Device, memoryBlock);
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	region.imageSubresource.baseArrayLayer = 0;
	vkCmdCopyBufferToImage(setupCommandBuffer, buffer, images[swapchainImageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	CHECK_VKCMD(vkEndCommandBuffer(setupCommandBuffer));
	CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));

	CHECK_VKCMD(vkQueueWaitIdle(appState.Queue));

	CHECK_XRCMD(xrReleaseSwapchainImage(appState.RightArrowsSwapchain, &releaseInfo));

	vkFreeCommandBuffers(appState.Device, appState.CommandPool, 1, &setupCommandBuffer);
	vkDestroyBuffer(appState.Device, buffer, nullptr);
	vkFreeMemory(appState.Device, memoryBlock, nullptr);

	VkShaderModule sortedSurfaceVertex;
	CreateShader(appState, app, "shaders/sorted_surface.vert.spv", &sortedSurfaceVertex);
	VkShaderModule sortedSurfaceFragment;
	CreateShader(appState, app, "shaders/sorted_surface.frag.spv", &sortedSurfaceFragment);
	VkShaderModule sortedSurfaceRotatedVertex;
	CreateShader(appState, app, "shaders/sorted_surface_rotated.vert.spv", &sortedSurfaceRotatedVertex);
	VkShaderModule surfaceVertex;
	CreateShader(appState, app, "shaders/surface.vert.spv", &surfaceVertex);
	VkShaderModule surfaceRotatedVertex;
	CreateShader(appState, app, "shaders/surface_rotated.vert.spv", &surfaceRotatedVertex);
	VkShaderModule surfaceFragment;
	CreateShader(appState, app, "shaders/surface.frag.spv", &surfaceFragment);
	VkShaderModule sortedFenceFragment;
	CreateShader(appState, app, "shaders/sorted_fence.frag.spv", &sortedFenceFragment);
	VkShaderModule fenceFragment;
	CreateShader(appState, app, "shaders/fence.frag.spv", &fenceFragment);
	VkShaderModule turbulentVertex;
	CreateShader(appState, app, "shaders/turbulent.vert.spv", &turbulentVertex);
	VkShaderModule turbulentFragment;
	CreateShader(appState, app, "shaders/turbulent.frag.spv", &turbulentFragment);
	VkShaderModule turbulentLitFragment;
	CreateShader(appState, app, "shaders/turbulent_lit.frag.spv", &turbulentLitFragment);
	VkShaderModule turbulentRotatedVertex;
	CreateShader(appState, app, "shaders/turbulent_rotated.vert.spv", &turbulentRotatedVertex);
	VkShaderModule turbulentRotatedFragment;
	CreateShader(appState, app, "shaders/turbulent_rotated.frag.spv", &turbulentRotatedFragment);
	VkShaderModule turbulentRotatedLitFragment;
	CreateShader(appState, app, "shaders/turbulent_rotated_lit.frag.spv", &turbulentRotatedLitFragment);
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
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
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

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	graphicsPipelineCreateInfo.pTessellationState = &tessellationStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
	graphicsPipelineCreateInfo.renderPass = appState.RenderPass;

	PipelineAttributes sortedSurfaceAttributes { };
	sortedSurfaceAttributes.vertexAttributes.resize(5);
	sortedSurfaceAttributes.vertexBindings.resize(2);
	sortedSurfaceAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	sortedSurfaceAttributes.vertexBindings[0].stride = 4 * sizeof(float);
	sortedSurfaceAttributes.vertexAttributes[1].location = 1;
	sortedSurfaceAttributes.vertexAttributes[1].binding = 1;
	sortedSurfaceAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	sortedSurfaceAttributes.vertexAttributes[2].location = 2;
	sortedSurfaceAttributes.vertexAttributes[2].binding = 1;
	sortedSurfaceAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	sortedSurfaceAttributes.vertexAttributes[2].offset = 4 * sizeof(float);
	sortedSurfaceAttributes.vertexAttributes[3].location = 3;
	sortedSurfaceAttributes.vertexAttributes[3].binding = 1;
	sortedSurfaceAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	sortedSurfaceAttributes.vertexAttributes[3].offset = 8 * sizeof(float);
	sortedSurfaceAttributes.vertexAttributes[4].location = 4;
	sortedSurfaceAttributes.vertexAttributes[4].binding = 1;
	sortedSurfaceAttributes.vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	sortedSurfaceAttributes.vertexAttributes[4].offset = 12 * sizeof(float);
	sortedSurfaceAttributes.vertexBindings[1].binding = 1;
	sortedSurfaceAttributes.vertexBindings[1].stride = 16 * sizeof(float);
	sortedSurfaceAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	sortedSurfaceAttributes.vertexInputState.vertexBindingDescriptionCount = sortedSurfaceAttributes.vertexBindings.size();
	sortedSurfaceAttributes.vertexInputState.pVertexBindingDescriptions = sortedSurfaceAttributes.vertexBindings.data();
	sortedSurfaceAttributes.vertexInputState.vertexAttributeDescriptionCount = sortedSurfaceAttributes.vertexAttributes.size();
	sortedSurfaceAttributes.vertexInputState.pVertexAttributeDescriptions = sortedSurfaceAttributes.vertexAttributes.data();
	sortedSurfaceAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	sortedSurfaceAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	PipelineAttributes sortedSurfaceRotatedAttributes { };
	sortedSurfaceRotatedAttributes.vertexAttributes.resize(7);
	sortedSurfaceRotatedAttributes.vertexBindings.resize(2);
	sortedSurfaceRotatedAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	sortedSurfaceRotatedAttributes.vertexBindings[0].stride = 4 * sizeof(float);
	sortedSurfaceRotatedAttributes.vertexAttributes[1].location = 1;
	sortedSurfaceRotatedAttributes.vertexAttributes[1].binding = 1;
	sortedSurfaceRotatedAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	sortedSurfaceRotatedAttributes.vertexAttributes[2].location = 2;
	sortedSurfaceRotatedAttributes.vertexAttributes[2].binding = 1;
	sortedSurfaceRotatedAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	sortedSurfaceRotatedAttributes.vertexAttributes[2].offset = 4 * sizeof(float);
	sortedSurfaceRotatedAttributes.vertexAttributes[3].location = 3;
	sortedSurfaceRotatedAttributes.vertexAttributes[3].binding = 1;
	sortedSurfaceRotatedAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	sortedSurfaceRotatedAttributes.vertexAttributes[3].offset = 8 * sizeof(float);
	sortedSurfaceRotatedAttributes.vertexAttributes[4].location = 4;
	sortedSurfaceRotatedAttributes.vertexAttributes[4].binding = 1;
	sortedSurfaceRotatedAttributes.vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	sortedSurfaceRotatedAttributes.vertexAttributes[4].offset = 12 * sizeof(float);
	sortedSurfaceRotatedAttributes.vertexAttributes[5].location = 5;
	sortedSurfaceRotatedAttributes.vertexAttributes[5].binding = 1;
	sortedSurfaceRotatedAttributes.vertexAttributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	sortedSurfaceRotatedAttributes.vertexAttributes[5].offset = 16 * sizeof(float);
	sortedSurfaceRotatedAttributes.vertexAttributes[6].location = 6;
	sortedSurfaceRotatedAttributes.vertexAttributes[6].binding = 1;
	sortedSurfaceRotatedAttributes.vertexAttributes[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	sortedSurfaceRotatedAttributes.vertexAttributes[6].offset = 20 * sizeof(float);
	sortedSurfaceRotatedAttributes.vertexBindings[1].binding = 1;
	sortedSurfaceRotatedAttributes.vertexBindings[1].stride = 24 * sizeof(float);
	sortedSurfaceRotatedAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	sortedSurfaceRotatedAttributes.vertexInputState.vertexBindingDescriptionCount = sortedSurfaceRotatedAttributes.vertexBindings.size();
	sortedSurfaceRotatedAttributes.vertexInputState.pVertexBindingDescriptions = sortedSurfaceRotatedAttributes.vertexBindings.data();
	sortedSurfaceRotatedAttributes.vertexInputState.vertexAttributeDescriptionCount = sortedSurfaceRotatedAttributes.vertexAttributes.size();
	sortedSurfaceRotatedAttributes.vertexInputState.pVertexAttributeDescriptions = sortedSurfaceRotatedAttributes.vertexAttributes.data();
	sortedSurfaceRotatedAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	sortedSurfaceRotatedAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	PipelineAttributes surfaceAttributes { };
	surfaceAttributes.vertexAttributes.resize(5);
	surfaceAttributes.vertexBindings.resize(2);
	surfaceAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceAttributes.vertexBindings[0].stride = 4 * sizeof(float);
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

	PipelineAttributes spriteAttributes { };
	spriteAttributes.vertexAttributes.resize(2);
	spriteAttributes.vertexBindings.resize(2);
	spriteAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	spriteAttributes.vertexBindings[0].stride = 3 * sizeof(float);
	spriteAttributes.vertexAttributes[1].location = 1;
	spriteAttributes.vertexAttributes[1].binding = 1;
	spriteAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	spriteAttributes.vertexBindings[1].binding = 1;
	spriteAttributes.vertexBindings[1].stride = 2 * sizeof(float);
	spriteAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	spriteAttributes.vertexInputState.vertexBindingDescriptionCount = spriteAttributes.vertexBindings.size();
	spriteAttributes.vertexInputState.pVertexBindingDescriptions = spriteAttributes.vertexBindings.data();
	spriteAttributes.vertexInputState.vertexAttributeDescriptionCount = spriteAttributes.vertexAttributes.size();
	spriteAttributes.vertexInputState.pVertexAttributeDescriptions = spriteAttributes.vertexAttributes.data();
	spriteAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	spriteAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	PipelineAttributes colormappedAttributes { };
	colormappedAttributes.vertexAttributes.resize(3);
	colormappedAttributes.vertexBindings.resize(3);
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
	colormappedAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	colormappedAttributes.vertexInputState.vertexBindingDescriptionCount = colormappedAttributes.vertexBindings.size();
	colormappedAttributes.vertexInputState.pVertexBindingDescriptions = colormappedAttributes.vertexBindings.data();
	colormappedAttributes.vertexInputState.vertexAttributeDescriptionCount = colormappedAttributes.vertexAttributes.size();
	colormappedAttributes.vertexInputState.pVertexAttributeDescriptions = colormappedAttributes.vertexAttributes.data();
	colormappedAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	colormappedAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

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
	particleAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	particleAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

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
	skyAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	skyAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

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
	coloredAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	coloredAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	PipelineAttributes floorAttributes { };
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

	VkDescriptorSetLayout descriptorSetLayouts[3] { };

	VkPushConstantRange pushConstantInfo { };

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

	surfaces.stages.resize(2);
	surfaces.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	surfaces.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	surfaces.stages[0].module = sortedSurfaceVertex;
	surfaces.stages[0].pName = "main";
	surfaces.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	surfaces.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	surfaces.stages[1].module = sortedSurfaceFragment;
	surfaces.stages[1].pName = "main";
	descriptorSetLayouts[0] = twoBuffersAndImageLayout;
	descriptorSetLayouts[1] = singleImageLayout;
	descriptorSetLayouts[2] = singleImageLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 3;
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfaces.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = surfaces.stages.size();
	graphicsPipelineCreateInfo.pStages = surfaces.stages.data();
	graphicsPipelineCreateInfo.layout = surfaces.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &sortedSurfaceAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &sortedSurfaceAttributes.inputAssemblyState;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfaces.pipeline));

	surfacesRotated.stages.resize(2);
	surfacesRotated.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	surfacesRotated.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	surfacesRotated.stages[0].module = sortedSurfaceRotatedVertex;
	surfacesRotated.stages[0].pName = "main";
	surfacesRotated.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	surfacesRotated.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	surfacesRotated.stages[1].module = sortedSurfaceFragment;
	surfacesRotated.stages[1].pName = "main";
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotated.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = surfacesRotated.stages.size();
	graphicsPipelineCreateInfo.pStages = surfacesRotated.stages.data();
	graphicsPipelineCreateInfo.layout = surfacesRotated.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &sortedSurfaceRotatedAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &sortedSurfaceRotatedAttributes.inputAssemblyState;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotated.pipeline));

	fences.stages.resize(2);
	fences.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fences.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	fences.stages[0].module = sortedSurfaceVertex;
	fences.stages[0].pName = "main";
	fences.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fences.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fences.stages[1].module = sortedFenceFragment;
	fences.stages[1].pName = "main";
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fences.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = fences.stages.size();
	graphicsPipelineCreateInfo.pStages = fences.stages.data();
	graphicsPipelineCreateInfo.layout = fences.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &sortedSurfaceAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &sortedSurfaceAttributes.inputAssemblyState;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fences.pipeline));

	fencesRotated.stages.resize(2);
	fencesRotated.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fencesRotated.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	fencesRotated.stages[0].module = surfaceRotatedVertex;
	fencesRotated.stages[0].pName = "main";
	fencesRotated.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fencesRotated.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fencesRotated.stages[1].module = fenceFragment;
	fencesRotated.stages[1].pName = "main";
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = sizeof(uint32_t) + 6 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotated.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = fencesRotated.stages.size();
	graphicsPipelineCreateInfo.pStages = fencesRotated.stages.data();
	graphicsPipelineCreateInfo.layout = fencesRotated.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &surfaceAttributes.inputAssemblyState;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotated.pipeline));

	turbulent.stages.resize(2);
	turbulent.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulent.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	turbulent.stages[0].module = turbulentVertex;
	turbulent.stages[0].pName = "main";
	turbulent.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulent.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	turbulent.stages[1].module = turbulentFragment;
	turbulent.stages[1].pName = "main";
	descriptorSetLayouts[0] = doubleBufferLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulent.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = turbulent.stages.size();
	graphicsPipelineCreateInfo.pStages = turbulent.stages.data();
	graphicsPipelineCreateInfo.layout = turbulent.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &sortedSurfaceAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &sortedSurfaceAttributes.inputAssemblyState;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulent.pipeline));

	turbulentLit.stages.resize(2);
	turbulentLit.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulentLit.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	turbulentLit.stages[0].module = surfaceVertex;
	turbulentLit.stages[0].pName = "main";
	turbulentLit.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulentLit.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	turbulentLit.stages[1].module = turbulentLitFragment;
	turbulentLit.stages[1].pName = "main";
	descriptorSetLayouts[0] = twoBuffersAndImageLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 3;
	pushConstantInfo.size = sizeof(float) + sizeof(uint32_t);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentLit.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = turbulentLit.stages.size();
	graphicsPipelineCreateInfo.pStages = turbulentLit.stages.data();
	graphicsPipelineCreateInfo.layout = turbulentLit.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &surfaceAttributes.inputAssemblyState;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentLit.pipeline));

	turbulentRotated.stages.resize(2);
	turbulentRotated.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulentRotated.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	turbulentRotated.stages[0].module = turbulentRotatedVertex;
	turbulentRotated.stages[0].pName = "main";
	turbulentRotated.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulentRotated.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	turbulentRotated.stages[1].module = turbulentRotatedFragment;
	turbulentRotated.stages[1].pName = "main";
	descriptorSetLayouts[0] = doubleBufferLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = 7 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotated.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = turbulentRotated.stages.size();
	graphicsPipelineCreateInfo.pStages = turbulentRotated.stages.data();
	graphicsPipelineCreateInfo.layout = turbulentRotated.pipelineLayout;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotated.pipeline));

	turbulentRotatedLit.stages.resize(2);
	turbulentRotatedLit.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulentRotatedLit.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	turbulentRotatedLit.stages[0].module = surfaceRotatedVertex;
	turbulentRotatedLit.stages[0].pName = "main";
	turbulentRotatedLit.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulentRotatedLit.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	turbulentRotatedLit.stages[1].module = turbulentRotatedLitFragment;
	turbulentRotatedLit.stages[1].pName = "main";
	descriptorSetLayouts[0] = twoBuffersAndImageLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 3;
	pushConstantInfo.size = sizeof(uint32_t) + 7 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotatedLit.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = turbulentRotatedLit.stages.size();
	graphicsPipelineCreateInfo.pStages = turbulentRotatedLit.stages.data();
	graphicsPipelineCreateInfo.layout = turbulentRotatedLit.pipelineLayout;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotatedLit.pipeline));

	sprites.stages.resize(2);
	sprites.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	sprites.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	sprites.stages[0].module = spriteVertex;
	sprites.stages[0].pName = "main";
	sprites.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	sprites.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	sprites.stages[1].module = spriteFragment;
	sprites.stages[1].pName = "main";
	descriptorSetLayouts[0] = doubleBufferLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &sprites.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = sprites.stages.size();
	graphicsPipelineCreateInfo.pStages = sprites.stages.data();
	graphicsPipelineCreateInfo.layout = sprites.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &spriteAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &spriteAttributes.inputAssemblyState;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &sprites.pipeline));

	alias.stages.resize(2);
	alias.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	alias.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	alias.stages[0].module = aliasVertex;
	alias.stages[0].pName = "main";
	alias.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	alias.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	alias.stages[1].module = aliasFragment;
	alias.stages[1].pName = "main";
	pipelineLayoutCreateInfo.setLayoutCount = 3;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantInfo.size = 16 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &alias.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = alias.stages.size();
	graphicsPipelineCreateInfo.pStages = alias.stages.data();
	graphicsPipelineCreateInfo.layout = alias.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &colormappedAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &colormappedAttributes.inputAssemblyState;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &alias.pipeline));

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
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &viewmodel.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = viewmodel.stages.size();
	graphicsPipelineCreateInfo.pStages = viewmodel.stages.data();
	graphicsPipelineCreateInfo.layout = viewmodel.pipelineLayout;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &viewmodel.pipeline));

	particle.stages.resize(2);
	particle.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	particle.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	particle.stages[0].module = particleVertex;
	particle.stages[0].pName = "main";
	particle.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	particle.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	particle.stages[1].module = coloredFragment;
	particle.stages[1].pName = "main";
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantInfo.size = 8 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &particle.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = particle.stages.size();
	graphicsPipelineCreateInfo.pStages = particle.stages.data();
	graphicsPipelineCreateInfo.layout = particle.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &particleAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &particleAttributes.inputAssemblyState;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &particle.pipeline));

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
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &colored.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = colored.stages.size();
	graphicsPipelineCreateInfo.pStages = colored.stages.data();
	graphicsPipelineCreateInfo.layout = colored.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &coloredAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &coloredAttributes.inputAssemblyState;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &colored.pipeline));

	sky.stages.resize(2);
	sky.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	sky.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	sky.stages[0].module = skyVertex;
	sky.stages[0].pName = "main";
	sky.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	sky.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	sky.stages[1].module = skyFragment;
	sky.stages[1].pName = "main";
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = 13 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &sky.pipelineLayout));
	depthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
	graphicsPipelineCreateInfo.stageCount = sky.stages.size();
	graphicsPipelineCreateInfo.pStages = sky.stages.data();
	graphicsPipelineCreateInfo.layout = sky.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &skyAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &skyAttributes.inputAssemblyState;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &sky.pipeline));

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
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &floor.pipelineLayout));
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	graphicsPipelineCreateInfo.stageCount = floor.stages.size();
	graphicsPipelineCreateInfo.pStages = floor.stages.data();
	graphicsPipelineCreateInfo.layout = floor.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &floorAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &floorAttributes.inputAssemblyState;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &floor.pipeline));

	vkDestroyShaderModule(appState.Device, floorFragment, nullptr);
	vkDestroyShaderModule(appState.Device, floorVertex, nullptr);
	vkDestroyShaderModule(appState.Device, coloredFragment, nullptr);
	vkDestroyShaderModule(appState.Device, coloredVertex, nullptr);
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
	vkDestroyShaderModule(appState.Device, turbulentFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentVertex, nullptr);
	vkDestroyShaderModule(appState.Device, fenceFragment, nullptr);
	vkDestroyShaderModule(appState.Device, sortedFenceFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRotatedVertex, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceVertex, nullptr);
	vkDestroyShaderModule(appState.Device, sortedSurfaceRotatedVertex, nullptr);
	vkDestroyShaderModule(appState.Device, sortedSurfaceFragment, nullptr);
	vkDestroyShaderModule(appState.Device, sortedSurfaceVertex, nullptr);

	for (auto& perFrame : appState.PerFrame)
	{
		perFrame.matrices.CreateUpdatableUniformBuffer(appState, (2 * 2 + 1) * sizeof(XrMatrix4x4f));
	}

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
	host_colormapSize = 0;
	skySize = 0;
	controllerVerticesSize = 0;
	buffers.Initialize();
	indexBuffers.Initialize();
	lightmaps.first = nullptr;
	lightmaps.current = nullptr;
	textures.first = nullptr;
	textures.current = nullptr;
}

void Scene::AddToBufferBarrier(VkBuffer buffer)
{
	stagingBuffer.lastBarrier++;
	if (stagingBuffer.bufferBarriers.size() <= stagingBuffer.lastBarrier)
	{
		stagingBuffer.bufferBarriers.emplace_back();
	}

	auto& barrier = stagingBuffer.bufferBarriers[stagingBuffer.lastBarrier];
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.buffer = buffer;
	barrier.size = VK_WHOLE_SIZE;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
}

void Scene::GetVerticesStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size)
{
	auto vertexes = ((model_t*)turbulent.model)->vertexes;
	if (previousVertexes != vertexes)
	{
		auto entry = vertexCache.find(vertexes);
		if (entry == vertexCache.end())
		{
			loaded.vertices.size = ((model_t*)turbulent.model)->numvertexes * 4 * sizeof(float);
			loaded.vertices.buffer = new SharedMemoryBuffer { };
			loaded.vertices.buffer->CreateVertexBuffer(appState, loaded.vertices.size);
			buffers.MoveToFront(loaded.vertices.buffer);
			size += loaded.vertices.size;
			buffers.SetupVertices(loaded.vertices);
			vertexCache.insert({ vertexes, loaded.vertices.buffer });
		}
		else
		{
			loaded.vertices.size = 0;
			loaded.vertices.buffer = entry->second;
		}
		previousVertexes = vertexes;
		previousVertexBuffer = loaded.vertices.buffer;
	}
	else
	{
		loaded.vertices.size = 0;
		loaded.vertices.buffer = previousVertexBuffer;
	}
	loaded.vertices.source = vertexes;
}

void Scene::GetTextureStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size)
{
	if (previousTexture != turbulent.data)
	{
		auto entry = textureCache.find(turbulent.data);
		if (entry == textureCache.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, turbulent.width, turbulent.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			textures.MoveToFront(texture);
			loaded.texture.size = turbulent.size;
			size += loaded.texture.size;
			loaded.texture.texture = texture;
			loaded.texture.source = turbulent.data;
			textures.Setup(loaded.texture);
			textureCache.insert({ turbulent.data, texture });
		}
		else
		{
			loaded.texture.size = 0;
			loaded.texture.texture = entry->second;
		}
		previousTexture = turbulent.data;
		previousSharedMemoryTexture = loaded.texture.texture;
	}
	else
	{
		loaded.texture.size = 0;
		loaded.texture.texture = previousSharedMemoryTexture;
	}
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size)
{
	GetVerticesStagingBufferSize(appState, surface, loaded, size);
	GetTextureStagingBufferSize(appState, surface, loaded, size);
	auto entry = perSurfaceCache.find(surface.surface);
	if (entry == perSurfaceCache.end())
	{
		loaded.texturePositions.size = 16 * sizeof(float);
		if (latestSharedMemoryTexturePositionBuffer == nullptr || usedInLatestSharedMemoryTexturePositionBuffer + loaded.texturePositions.size > latestSharedMemoryTexturePositionBuffer->size)
		{
			loaded.texturePositions.texturePositions.buffer = new SharedMemoryBuffer { };
			loaded.texturePositions.texturePositions.buffer->CreateVertexBuffer(appState, Constants::memoryBlockSize);
			buffers.MoveToFront(loaded.texturePositions.texturePositions.buffer);
			latestSharedMemoryTexturePositionBuffer = loaded.texturePositions.texturePositions.buffer;
			usedInLatestSharedMemoryTexturePositionBuffer = 0;
		}
		else
		{
			loaded.texturePositions.texturePositions.buffer = latestSharedMemoryTexturePositionBuffer;
		}
		loaded.texturePositions.texturePositions.offset = usedInLatestSharedMemoryTexturePositionBuffer;
		usedInLatestSharedMemoryTexturePositionBuffer += loaded.texturePositions.size;
		size += loaded.texturePositions.size;
		loaded.texturePositions.texturePositions.firstInstance = loaded.texturePositions.texturePositions.offset / (16 * sizeof(float));
		buffers.SetupSurfaceTexturePositions(loaded.texturePositions);
		auto face = (msurface_t*)surface.surface;
		auto model = (model_t*)surface.model;
		unsigned int maxIndex = 0;
		for (auto i = 0; i < face->numedges; i++)
		{
			auto edge = model->surfedges[face->firstedge + i];
			if (edge >= 0)
			{
				maxIndex = std::max(maxIndex, model->edges[edge].v[0]);
			}
			else
			{
				maxIndex = std::max(maxIndex, model->edges[-edge].v[1]);
			}
		}
		if (maxIndex < UPPER_8BIT_LIMIT && appState.IndexTypeUInt8Enabled)
		{
			loaded.indices.size = surface.count;
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
			loaded.indices.indices.indexType = VK_INDEX_TYPE_UINT8_EXT;
			loaded.indices.indices.firstIndex = loaded.indices.indices.offset;
			indexBuffers.SetupIndices8(loaded.indices);
		}
		else if (maxIndex < UPPER_16BIT_LIMIT)
		{
			loaded.indices.size = surface.count * sizeof(uint16_t);
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
			loaded.indices.indices.indexType = VK_INDEX_TYPE_UINT16;
			loaded.indices.indices.firstIndex = loaded.indices.indices.offset / 2;
			indexBuffers.SetupIndices16(loaded.indices);
		}
		else
		{
			loaded.indices.size = surface.count * sizeof(uint32_t);
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
			loaded.indices.indices.indexType = VK_INDEX_TYPE_UINT32;
			loaded.indices.indices.firstIndex = loaded.indices.indices.offset / 4;
			indexBuffers.SetupIndices32(loaded.indices);
		}
		PerSurface perSurface { loaded.texturePositions.texturePositions, loaded.indices.indices };
		perSurfaceCache.insert({ surface.surface, perSurface });
	}
	else
	{
		loaded.texturePositions.size = 0;
		loaded.texturePositions.texturePositions = entry->second.texturePosition;
		loaded.indices.size = 0;
		loaded.indices.indices = entry->second.indices;
	}
	loaded.texturePositions.source = surface.surface;
	loaded.indices.firstSource = surface.surface;
	loaded.indices.secondSource = surface.model;
	auto lightmapEntry = lightmaps.lightmaps.find(surface.surface);
	if (lightmapEntry == lightmaps.lightmaps.end())
	{
		auto lightmap = new Lightmap { };
		lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		lightmap->key = surface.surface;
		lightmap->createdFrameCount = surface.created;
		loaded.lightmap.lightmap = lightmap;
		loaded.lightmap.size = surface.lightmap_size * sizeof(float);
		size += loaded.lightmap.size;
		loaded.lightmap.source = d_lists.lightmap_texels.data() + surface.lightmap_texels;
		lightmaps.Setup(loaded.lightmap);
		lightmaps.lightmaps.insert({ lightmap->key, lightmap });
	}
	else
	{
		auto lightmap = lightmapEntry->second;
		if (lightmap->createdFrameCount != surface.created)
		{
			lightmap->next = lightmaps.oldLightmaps;
			lightmaps.oldLightmaps = lightmap;
			lightmap = new Lightmap { };
			lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			lightmap->key = surface.surface;
			lightmap->createdFrameCount = surface.created;
			lightmapEntry->second = lightmap;
			loaded.lightmap.lightmap = lightmap;
			loaded.lightmap.size = surface.lightmap_size * sizeof(float);
			size += loaded.lightmap.size;
			loaded.lightmap.source = d_lists.lightmap_texels.data() + surface.lightmap_texels;
			lightmaps.Setup(loaded.lightmap);
		}
		else
		{
			loaded.lightmap.lightmap = lightmap;
			loaded.lightmap.size = 0;
		}
	}
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

void Scene::GetStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size)
{
	GetVerticesStagingBufferSize(appState, turbulent, loaded, size);
	GetTextureStagingBufferSize(appState, turbulent, loaded, size);
	auto entry = perSurfaceCache.find(turbulent.surface);
	if (entry == perSurfaceCache.end())
	{
		loaded.texturePositions.size = 16 * sizeof(float);
		if (latestSharedMemoryTexturePositionBuffer == nullptr || usedInLatestSharedMemoryTexturePositionBuffer + loaded.texturePositions.size > latestSharedMemoryTexturePositionBuffer->size)
		{
			loaded.texturePositions.texturePositions.buffer = new SharedMemoryBuffer { };
			loaded.texturePositions.texturePositions.buffer->CreateVertexBuffer(appState, Constants::memoryBlockSize);
			buffers.MoveToFront(loaded.texturePositions.texturePositions.buffer);
			latestSharedMemoryTexturePositionBuffer = loaded.texturePositions.texturePositions.buffer;
			usedInLatestSharedMemoryTexturePositionBuffer = 0;
		}
		else
		{
			loaded.texturePositions.texturePositions.buffer = latestSharedMemoryTexturePositionBuffer;
		}
		loaded.texturePositions.texturePositions.offset = usedInLatestSharedMemoryTexturePositionBuffer;
		usedInLatestSharedMemoryTexturePositionBuffer += loaded.texturePositions.size;
		size += loaded.texturePositions.size;
		loaded.texturePositions.texturePositions.firstInstance = loaded.texturePositions.texturePositions.offset / (16 * sizeof(float));
		buffers.SetupTurbulentTexturePositions(loaded.texturePositions);
		auto face = (msurface_t*)turbulent.surface;
		auto model = (model_t*)turbulent.model;
		unsigned int maxIndex = 0;
		for (auto i = 0; i < face->numedges; i++)
		{
			auto edge = model->surfedges[face->firstedge + i];
			if (edge >= 0)
			{
				maxIndex = std::max(maxIndex, model->edges[edge].v[0]);
			}
			else
			{
				maxIndex = std::max(maxIndex, model->edges[-edge].v[1]);
			}
		}
		if (maxIndex < UPPER_8BIT_LIMIT && appState.IndexTypeUInt8Enabled)
		{
			loaded.indices.size = turbulent.count;
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
			loaded.indices.indices.indexType = VK_INDEX_TYPE_UINT8_EXT;
			loaded.indices.indices.firstIndex = loaded.indices.indices.offset;
			indexBuffers.SetupIndices8(loaded.indices);
		}
		else if (maxIndex < UPPER_16BIT_LIMIT)
		{
			loaded.indices.size = turbulent.count * sizeof(uint16_t);
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
			loaded.indices.indices.indexType = VK_INDEX_TYPE_UINT16;
			loaded.indices.indices.firstIndex = loaded.indices.indices.offset / 2;
			indexBuffers.SetupIndices16(loaded.indices);
		}
		else
		{
			loaded.indices.size = turbulent.count * sizeof(uint32_t);
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
			loaded.indices.indices.indexType = VK_INDEX_TYPE_UINT32;
			loaded.indices.indices.firstIndex = loaded.indices.indices.offset / 4;
			indexBuffers.SetupIndices32(loaded.indices);
		}
		PerSurface perSurface { loaded.texturePositions.texturePositions, loaded.indices.indices };
		perSurfaceCache.insert({ turbulent.surface, perSurface });
	}
	else
	{
		loaded.texturePositions.size = 0;
		loaded.texturePositions.texturePositions = entry->second.texturePosition;
		loaded.indices.size = 0;
		loaded.indices.indices = entry->second.indices;
	}
	loaded.texturePositions.source = turbulent.surface;
	loaded.indices.firstSource = turbulent.surface;
	loaded.indices.secondSource = turbulent.model;
	loaded.count = turbulent.count;
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
			texture->Create(appState, sprite.width, sprite.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			textures.MoveToFront(texture);
			loaded.texture.size = sprite.size;
			size += loaded.texture.size;
			loaded.texture.texture = texture;
			loaded.texture.source = sprite.data;
			textures.Setup(loaded.texture);
			spriteCache.insert({ sprite.data, texture });
		}
		else
		{
			loaded.texture.size = 0;
			loaded.texture.texture = entry->second;
		}
		previousTexture = sprite.data;
		previousSharedMemoryTexture = loaded.texture.texture;
	}
	else
	{
		loaded.texture.size = 0;
		loaded.texture.texture = previousSharedMemoryTexture;
	}
	loaded.count = sprite.count;
	loaded.firstVertex = sprite.first_vertex;
}

void Scene::GetStagingBufferSize(AppState& appState, const dalias_t& alias, LoadedAlias& loaded, Texture* host_colormap, VkDeviceSize& size)
{
	if (previousApverts != alias.apverts)
	{
		auto entry = aliasVerticesCache.find(alias.apverts);
		if (entry == aliasVerticesCache.end())
		{
			loaded.vertices.size = alias.vertex_count * 2 * 4 * sizeof(float);
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
			aliasVerticesCache.insert({ alias.apverts, { loaded.vertices.buffer, loaded.texCoords.buffer } });
		}
		else
		{
			loaded.vertices.size = 0;
			loaded.vertices.buffer = entry->second.vertices;
			loaded.texCoords.size = 0;
			loaded.texCoords.buffer = entry->second.texCoords;
		}
		previousApverts = alias.apverts;
		previousVertexBuffer = loaded.vertices.buffer;
		previousTexCoordsBuffer = loaded.texCoords.buffer;
	}
	else
	{
		loaded.vertices.size = 0;
		loaded.vertices.buffer = previousVertexBuffer;
		loaded.texCoords.size = 0;
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
		auto entry = aliasTexturesCache.find(alias.data);
		if (entry == aliasTexturesCache.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(alias.width, alias.height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, alias.width, alias.height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			textures.MoveToFront(texture);
			loaded.colormapped.texture.size = alias.size;
			size += loaded.colormapped.texture.size;
			loaded.colormapped.texture.texture = texture;
			loaded.colormapped.texture.source = alias.data;
			textures.Setup(loaded.colormapped.texture);
			aliasTexturesCache.insert({ alias.data, texture });
		}
		else
		{
			loaded.colormapped.texture.size = 0;
			loaded.colormapped.texture.texture = entry->second;
		}
		previousTexture = alias.data;
		previousSharedMemoryTexture = loaded.colormapped.texture.texture;
	}
	else
	{
		loaded.colormapped.texture.size = 0;
		loaded.colormapped.texture.texture = previousSharedMemoryTexture;
	}
	auto entry = aliasIndicesCache.find(alias.aliashdr);
	if (entry == aliasIndicesCache.end())
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
			loaded.indices.firstSource = alias.aliashdr;
			loaded.indices.secondSource = nullptr;
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
			loaded.indices.firstSource = alias.aliashdr;
			loaded.indices.secondSource = nullptr;
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
			loaded.indices.firstSource = alias.aliashdr;
			loaded.indices.secondSource = nullptr;
			loaded.indices.indices.indexType = VK_INDEX_TYPE_UINT32;
			loaded.indices.indices.firstIndex = loaded.indices.indices.offset / 4;
			indexBuffers.SetupAliasIndices32(loaded.indices);
		}
		aliasIndicesCache.insert({ alias.aliashdr, loaded.indices.indices });
	}
	else
	{
		loaded.indices.size = 0;
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
	lastSurface = d_lists.last_surface;
	lastSurfaceRotated = d_lists.last_surface_rotated;
	lastFence = d_lists.last_fence;
	lastFenceRotated = d_lists.last_fence_rotated;
	lastTurbulent = d_lists.last_turbulent;
	lastTurbulentLit = d_lists.last_turbulent_lit;
	lastTurbulentRotated = d_lists.last_turbulent_rotated;
	lastTurbulentRotatedLit = d_lists.last_turbulent_rotated_lit;
	lastSprite = d_lists.last_sprite;
	lastAlias = d_lists.last_alias;
	lastViewmodel = d_lists.last_viewmodel;
	lastParticle = d_lists.last_particle_color;
	lastColoredIndex8 = d_lists.last_colored_index8;
	lastColoredIndex16 = d_lists.last_colored_index16;
	lastColoredIndex32 = d_lists.last_colored_index32;
	lastSky = d_lists.last_sky;
	appState.FromEngine.vieworg0 = d_lists.vieworg0;
	appState.FromEngine.vieworg1 = d_lists.vieworg1;
	appState.FromEngine.vieworg2 = d_lists.vieworg2;
	appState.FromEngine.vpn0 = d_lists.vpn0;
	appState.FromEngine.vpn1 = d_lists.vpn1;
	appState.FromEngine.vpn2 = d_lists.vpn2;
	appState.FromEngine.vright0 = d_lists.vright0;
	appState.FromEngine.vright1 = d_lists.vright1;
	appState.FromEngine.vright2 = d_lists.vright2;
	appState.FromEngine.vup0 = d_lists.vup0;
	appState.FromEngine.vup1 = d_lists.vup1;
	appState.FromEngine.vup2 = d_lists.vup2;
	if (lastSurface >= loadedSurfaces.size())
	{
		loadedSurfaces.resize(lastSurface + 1);
	}
	if (lastSurfaceRotated >= loadedSurfacesRotated.size())
	{
		loadedSurfacesRotated.resize(lastSurfaceRotated + 1);
	}
	if (lastFence >= loadedFences.size())
	{
		loadedFences.resize(lastFence + 1);
	}
	if (lastFenceRotated >= loadedFencesRotated.size())
	{
		loadedFencesRotated.resize(lastFenceRotated + 1);
	}
	if (lastTurbulent >= loadedTurbulent.size())
	{
		loadedTurbulent.resize(lastTurbulent + 1);
	}
	if (lastTurbulentLit >= loadedTurbulentLit.size())
	{
		loadedTurbulentLit.resize(lastTurbulentLit + 1);
	}
	if (lastTurbulentRotated >= loadedTurbulentRotated.size())
	{
		loadedTurbulentRotated.resize(lastTurbulentRotated + 1);
	}
	if (lastTurbulentRotatedLit >= loadedTurbulentRotatedLit.size())
	{
		loadedTurbulentRotatedLit.resize(lastTurbulentRotatedLit + 1);
	}
	if (lastSprite >= loadedSprites.size())
	{
		loadedSprites.resize(lastSprite + 1);
	}
	if (lastAlias >= loadedAlias.size())
	{
		loadedAlias.resize(lastAlias + 1);
	}
	if (lastViewmodel >= loadedViewmodels.size())
	{
		loadedViewmodels.resize(lastViewmodel + 1);
	}
	VkDeviceSize size = 0;
	if (perFrame.palette == nullptr || perFrame.paletteChanged != pal_changed)
	{
		if (perFrame.palette == nullptr)
		{
			perFrame.palette = new Buffer();
			perFrame.palette->CreateUniformBuffer(appState, 256 * 4 * sizeof(float));
		}
		paletteSize = perFrame.palette->size;
		size += paletteSize;
		perFrame.paletteChanged = pal_changed;
	}
	if (!::host_colormap.empty() && perFrame.host_colormap == nullptr)
	{
		perFrame.host_colormap = new Texture();
		perFrame.host_colormap->Create(appState, 256, 64, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		host_colormapSize = 16384;
		size += host_colormapSize;
	}
	previousVertexes = nullptr;
	previousTexture = nullptr;
	sorted.Initialize(sorted.surfaces);
	for (auto i = 0; i <= d_lists.last_surface; i++)
	{
		auto& loaded = loadedSurfaces[i];
		GetStagingBufferSize(appState, d_lists.surfaces[i], loaded, size);
		sorted.Sort(loaded, i, sorted.surfaces);
		sortedVerticesSize += (loaded.count * 4 * sizeof(float));
		sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(sorted.surfaces);
	sortedSurfaceRotatedVerticesBase = sortedVerticesSize;
	sortedSurfaceRotatedAttributesBase = sortedAttributesSize;
	sortedSurfaceRotatedIndicesBase = sortedIndicesCount;
	previousVertexes = nullptr;
	previousTexture = nullptr;
	sorted.Initialize(sorted.surfacesRotated);
	for (auto i = 0; i <= lastSurfaceRotated; i++)
	{
		auto& loaded = loadedSurfacesRotated[i];
		GetStagingBufferSize(appState, d_lists.surfaces_rotated[i], loaded, size);
		sorted.Sort(loaded, i, sorted.surfacesRotated);
		sortedVerticesSize += (loaded.count * 4 * sizeof(float));
		sortedAttributesSize += (loaded.count * 24 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(sorted.surfacesRotated);
	sortedFenceVerticesBase = sortedVerticesSize;
	sortedFenceAttributesBase = sortedAttributesSize;
	sortedFenceIndicesBase = sortedIndicesCount;
	previousVertexes = nullptr;
	previousTexture = nullptr;
	sorted.Initialize(sorted.fences);
	for (auto i = 0; i <= lastFence; i++)
	{
		auto& loaded = loadedFences[i];
		GetStagingBufferSize(appState, d_lists.fences[i], loaded, size);
		sorted.Sort(loaded, i, sorted.fences);
		sortedVerticesSize += (loaded.count * 4 * sizeof(float));
		sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(sorted.fences);
	previousVertexes = nullptr;
	previousTexture = nullptr;
	sorted.Initialize(sorted.fencesRotated);
	for (auto i = 0; i <= lastFenceRotated; i++)
	{
		auto& loaded = loadedFencesRotated[i];
		GetStagingBufferSize(appState, d_lists.fences_rotated[i], loaded, size);
		sorted.Sort(loaded, i, sorted.fencesRotated);
	}
	SortedSurfaces::Cleanup(sorted.fencesRotated);
	sortedTurbulentVerticesBase = sortedVerticesSize;
	sortedTurbulentAttributesBase = sortedAttributesSize;
	sortedTurbulentIndicesBase = sortedIndicesCount;
	previousVertexes = nullptr;
	previousTexture = nullptr;
	sorted.Initialize(sorted.turbulent);
	for (auto i = 0; i <= lastTurbulent; i++)
	{
		auto& loaded = loadedTurbulent[i];
		GetStagingBufferSize(appState, d_lists.turbulent[i], loaded, size);
		sorted.Sort(loaded, i, sorted.turbulent);
		sortedVerticesSize += (loaded.count * 4 * sizeof(float));
		sortedAttributesSize += (loaded.count * 16 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
	}
	SortedSurfaces::Cleanup(sorted.turbulent);
	previousVertexes = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= lastTurbulentLit; i++)
	{
		GetStagingBufferSize(appState, d_lists.turbulent_lit[i], loadedTurbulentLit[i], size);
	}
	previousVertexes = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= lastTurbulentRotated; i++)
	{
		GetStagingBufferSize(appState, d_lists.turbulent_rotated[i], loadedTurbulentRotated[i], size);
	}
	previousVertexes = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= lastTurbulentRotatedLit; i++)
	{
		GetStagingBufferSize(appState, d_lists.turbulent_rotated_lit[i], loadedTurbulentRotatedLit[i], size);
	}
	previousTexture = nullptr;
	for (auto i = 0; i <= lastSprite; i++)
	{
		GetStagingBufferSize(appState, d_lists.sprites[i], loadedSprites[i], size);
	}
	previousApverts = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= lastAlias; i++)
	{
		GetStagingBufferSize(appState, d_lists.alias[i], loadedAlias[i], perFrame.host_colormap, size);
	}
	previousApverts = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= lastViewmodel; i++)
	{
		GetStagingBufferSize(appState, d_lists.viewmodels[i], loadedViewmodels[i], perFrame.host_colormap, size);
	}
	if (lastSky >= 0)
	{
		if (perFrame.sky == nullptr)
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(128, 128)))) + 1;
			perFrame.sky = new Texture();
			perFrame.sky->Create(appState, 128, 128, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		skySize = 16384;
		size += skySize;
		skyCount = d_lists.sky[0].count;
		firstSkyVertex = d_lists.sky[0].first_vertex;
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
		floorIndicesSize = 6;
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
		sortedSurfaceRotatedIndicesBase *= sizeof(uint16_t);
		sortedFenceIndicesBase *= sizeof(uint16_t);
		sortedTurbulentIndicesBase *= sizeof(uint16_t);
	}
	else
	{
		sortedIndices16Size = 0;
		sortedIndices32Size = sortedIndicesCount * sizeof(uint32_t);
		sortedSurfaceRotatedIndicesBase *= sizeof(uint32_t);
		sortedFenceIndicesBase *= sizeof(uint32_t);
		sortedTurbulentIndicesBase *= sizeof(uint32_t);
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
	aliasTexturesCache.clear();
	spriteCache.clear();
	textureCache.clear();
	textures.DisposeFront();
	lightmaps.DisposeFront();
	indexBuffers.DisposeFront();
	buffers.DisposeFront();
	latestTextureDescriptorSets = nullptr;
	latestIndexBuffer32 = nullptr;
	usedInLatestIndexBuffer32 = 0;
	latestIndexBuffer16 = nullptr;
	usedInLatestIndexBuffer16 = 0;
	latestIndexBuffer8 = nullptr;
	usedInLatestIndexBuffer8 = 0;
	latestSharedMemoryTexturePositionBuffer = nullptr;
	usedInLatestSharedMemoryTexturePositionBuffer = 0;
	latestMemory.clear();
	Skybox::MoveToPrevious(*this);
	aliasIndicesCache.clear();
	aliasVerticesCache.clear();
	perSurfaceCache.clear();
	vertexCache.clear();
}
