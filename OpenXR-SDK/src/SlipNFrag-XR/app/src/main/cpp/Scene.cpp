#include "AppState.h"
#include "Scene.h" // This header is specified second in the list to allow headers in AppState.h to include the core engine structs
#include "Utils.h"
#include "ImageAsset.h"
#include "PipelineAttributes.h"
#include "Constants.h"
#include "MemoryAllocateInfo.h"

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

	std::vector<XrSwapchainImageVulkan2KHR> images(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR });
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
	
	images.resize(imageCount,  { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR });
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

	images.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR });
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

	XrSwapchainImageAcquireInfo acquireInfo { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
	uint32_t swapchainImageIndex;
	CHECK_XRCMD(xrAcquireSwapchainImage(appState.LeftArrowsSwapchain, &acquireInfo, &swapchainImageIndex));

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

	images.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR });
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.RightArrowsSwapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)images.data()));
 
 	bufferCreateInfo.size = swapchainCreateInfo.width * swapchainCreateInfo.height * 4;
	region.imageExtent.width = swapchainCreateInfo.width;
	region.imageExtent.height = swapchainCreateInfo.height;

	CHECK_XRCMD(xrAcquireSwapchainImage(appState.RightArrowsSwapchain, &acquireInfo, &swapchainImageIndex));

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

	VkShaderModule surfaceVertex;
	CreateShader(appState, app, "shaders/surface.vert.spv", &surfaceVertex);
	VkShaderModule surfaceRotatedVertex;
	CreateShader(appState, app, "shaders/surface_rotated.vert.spv", &surfaceRotatedVertex);
	VkShaderModule surfaceFragment;
	CreateShader(appState, app, "shaders/surface.frag.spv", &surfaceFragment);
	VkShaderModule fenceFragment;
	CreateShader(appState, app, "shaders/fence.frag.spv", &fenceFragment);
	VkShaderModule spriteVertex;
	CreateShader(appState, app, "shaders/sprite.vert.spv", &spriteVertex);
	VkShaderModule spriteFragment;
	CreateShader(appState, app, "shaders/sprite.frag.spv", &spriteFragment);
	VkShaderModule turbulentVertex;
	CreateShader(appState, app, "shaders/turbulent.vert.spv", &turbulentVertex);
	VkShaderModule turbulentFragment;
	CreateShader(appState, app, "shaders/turbulent.frag.spv", &turbulentFragment);
	VkShaderModule turbulentRotatedVertex;
	CreateShader(appState, app, "shaders/turbulent_rotated.vert.spv", &turbulentRotatedVertex);
	VkShaderModule aliasVertex;
	CreateShader(appState, app, "shaders/alias.vert.spv", &aliasVertex);
	VkShaderModule aliasFragment;
	CreateShader(appState, app, "shaders/alias.frag.spv", &aliasFragment);
	VkShaderModule viewmodelVertex;
	CreateShader(appState, app, "shaders/viewmodel.vert.spv", &viewmodelVertex);
	VkShaderModule viewmodelFragment;
	CreateShader(appState, app, "shaders/viewmodel.frag.spv", &viewmodelFragment);
	VkShaderModule coloredVertex;
	CreateShader(appState, app, "shaders/colored.vert.spv", &coloredVertex);
	VkShaderModule coloredFragment;
	CreateShader(appState, app, "shaders/colored.frag.spv", &coloredFragment);
	VkShaderModule skyVertex;
	CreateShader(appState, app, "shaders/sky.vert.spv", &skyVertex);
	VkShaderModule skyFragment;
	CreateShader(appState, app, "shaders/sky.frag.spv", &skyFragment);
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
	surfaces.stages[0].module = surfaceVertex;
	surfaces.stages[0].pName = "main";
	surfaces.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	surfaces.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	surfaces.stages[1].module = surfaceFragment;
	surfaces.stages[1].pName = "main";
	descriptorSetLayouts[0] = twoBuffersAndImageLayout;
	descriptorSetLayouts[1] = singleImageLayout;
	descriptorSetLayouts[2] = singleImageLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 3;
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = 4 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfaces.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = surfaces.stages.size();
	graphicsPipelineCreateInfo.pStages = surfaces.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &surfaceAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = surfaces.pipelineLayout;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfaces.pipeline));

	surfacesRotated.stages.resize(2);
	surfacesRotated.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	surfacesRotated.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	surfacesRotated.stages[0].module = surfaceRotatedVertex;
	surfacesRotated.stages[0].pName = "main";
	surfacesRotated.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	surfacesRotated.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	surfacesRotated.stages[1].module = surfaceFragment;
	surfacesRotated.stages[1].pName = "main";
	pushConstantInfo.size = 7 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotated.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = surfacesRotated.stages.size();
	graphicsPipelineCreateInfo.pStages = surfacesRotated.stages.data();
	graphicsPipelineCreateInfo.layout = surfacesRotated.pipelineLayout;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotated.pipeline));

	fences.stages.resize(2);
	fences.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fences.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	fences.stages[0].module = surfaceVertex;
	fences.stages[0].pName = "main";
	fences.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fences.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fences.stages[1].module = fenceFragment;
	fences.stages[1].pName = "main";
	pushConstantInfo.size = 4 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fences.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = fences.stages.size();
	graphicsPipelineCreateInfo.pStages = fences.stages.data();
	graphicsPipelineCreateInfo.layout = fences.pipelineLayout;
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
	pushConstantInfo.size = 7 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotated.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = fencesRotated.stages.size();
	graphicsPipelineCreateInfo.pStages = fencesRotated.stages.data();
	graphicsPipelineCreateInfo.layout = fencesRotated.pipelineLayout;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotated.pipeline));

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
	graphicsPipelineCreateInfo.pVertexInputState = &spriteAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &spriteAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = sprites.pipelineLayout;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &sprites.pipeline));

	turbulent.stages.resize(2);
	turbulent.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulent.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	turbulent.stages[0].module = turbulentVertex;
	turbulent.stages[0].pName = "main";
	turbulent.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulent.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	turbulent.stages[1].module = turbulentFragment;
	turbulent.stages[1].pName = "main";
	pushConstantInfo.size = 4 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulent.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = turbulent.stages.size();
	graphicsPipelineCreateInfo.pStages = turbulent.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &surfaceAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = turbulent.pipelineLayout;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulent.pipeline));

	turbulentRotated.stages.resize(2);
	turbulentRotated.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulentRotated.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	turbulentRotated.stages[0].module = turbulentRotatedVertex;
	turbulentRotated.stages[0].pName = "main";
	turbulentRotated.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	turbulentRotated.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	turbulentRotated.stages[1].module = turbulentFragment;
	turbulentRotated.stages[1].pName = "main";
	pushConstantInfo.size = 7 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotated.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = turbulentRotated.stages.size();
	graphicsPipelineCreateInfo.pStages = turbulentRotated.stages.data();
	graphicsPipelineCreateInfo.layout = turbulentRotated.pipelineLayout;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotated.pipeline));

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
	graphicsPipelineCreateInfo.pVertexInputState = &colormappedAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &colormappedAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = alias.pipelineLayout;
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

	colored.stages.resize(2);
	colored.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	colored.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	colored.stages[0].module = coloredVertex;
	colored.stages[0].pName = "main";
	colored.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	colored.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	colored.stages[1].module = coloredFragment;
	colored.stages[1].pName = "main";
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &colored.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = colored.stages.size();
	graphicsPipelineCreateInfo.pStages = colored.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &coloredAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &coloredAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = colored.pipelineLayout;
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
	graphicsPipelineCreateInfo.stageCount = sky.stages.size();
	graphicsPipelineCreateInfo.pStages = sky.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &skyAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &skyAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = sky.pipelineLayout;
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
	graphicsPipelineCreateInfo.stageCount = floor.stages.size();
	graphicsPipelineCreateInfo.pStages = floor.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &floorAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &floorAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = floor.pipelineLayout;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &floor.pipeline));

	vkDestroyShaderModule(appState.Device, floorFragment, nullptr);
	vkDestroyShaderModule(appState.Device, floorVertex, nullptr);
	vkDestroyShaderModule(appState.Device, skyFragment, nullptr);
	vkDestroyShaderModule(appState.Device, skyVertex, nullptr);
	vkDestroyShaderModule(appState.Device, coloredFragment, nullptr);
	vkDestroyShaderModule(appState.Device, coloredVertex, nullptr);
	vkDestroyShaderModule(appState.Device, viewmodelFragment, nullptr);
	vkDestroyShaderModule(appState.Device, viewmodelVertex, nullptr);
	vkDestroyShaderModule(appState.Device, aliasFragment, nullptr);
	vkDestroyShaderModule(appState.Device, aliasVertex, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentRotatedVertex, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentVertex, nullptr);
	vkDestroyShaderModule(appState.Device, spriteFragment, nullptr);
	vkDestroyShaderModule(appState.Device, spriteVertex, nullptr);
	vkDestroyShaderModule(appState.Device, fenceFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRotatedVertex, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceVertex, nullptr);

	for (auto& perImage : appState.PerImage)
	{
		perImage.matrices.CreateUniformBuffer(appState, (2 * 2 + 1) * sizeof(XrMatrix4x4f));
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
	verticesSize = 0;
	paletteSize = 0;
	host_colormapSize = 0;
	skySize = 0;
	controllerVerticesSize = 0;
	buffers.Initialize();
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

void Scene::GetIndices16StagingBufferSize(AppState& appState, dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size)
{
	auto entry = indicesPerKey.find(surface.surface);
	if (entry == indicesPerKey.end())
	{
		loaded.indices.size = surface.count * sizeof(uint16_t);
		if (latestSharedMemoryIndexBuffer16 == nullptr || usedInLatestSharedMemoryIndexBuffer16 + loaded.indices.size > latestSharedMemoryIndexBuffer16->size)
		{
			loaded.indices.buffer = new SharedMemoryBuffer { };
			loaded.indices.buffer->CreateIndexBuffer(appState, Constants::memoryBlockSize);
			buffers.MoveToFront(loaded.indices.buffer);
			latestSharedMemoryIndexBuffer16 = loaded.indices.buffer;
			usedInLatestSharedMemoryIndexBuffer16 = 0;
		}
		else
		{
			loaded.indices.buffer = latestSharedMemoryIndexBuffer16;
			loaded.indices.offset = usedInLatestSharedMemoryIndexBuffer16;
		}
		usedInLatestSharedMemoryIndexBuffer16 += loaded.indices.size;
		size += loaded.indices.size;
		loaded.indices.firstSource = surface.surface;
		loaded.indices.secondSource = surface.model;
		buffers.SetupIndices16(loaded.indices);
		SharedMemoryBufferWithOffset newEntry { loaded.indices.buffer, loaded.indices.offset };
		indicesPerKey.insert({ surface.surface, newEntry });
	}
	else
	{
		loaded.indices.size = 0;
		loaded.indices.buffer = entry->second.buffer;
		loaded.indices.offset = entry->second.offset;
	}
}

void Scene::GetIndices32StagingBufferSize(AppState& appState, dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size)
{
	auto entry = indicesPerKey.find(surface.surface);
	if (entry == indicesPerKey.end())
	{
		loaded.indices.size = surface.count * sizeof(uint32_t);
		if (latestSharedMemoryIndexBuffer32 == nullptr || usedInLatestSharedMemoryIndexBuffer32 + loaded.indices.size > latestSharedMemoryIndexBuffer32->size)
		{
			loaded.indices.buffer = new SharedMemoryBuffer { };
			loaded.indices.buffer->CreateIndexBuffer(appState, Constants::memoryBlockSize);
			buffers.MoveToFront(loaded.indices.buffer);
			latestSharedMemoryIndexBuffer32 = loaded.indices.buffer;
			usedInLatestSharedMemoryIndexBuffer32 = 0;
		}
		else
		{
			loaded.indices.buffer = latestSharedMemoryIndexBuffer32;
			loaded.indices.offset = usedInLatestSharedMemoryIndexBuffer32;
		}
		usedInLatestSharedMemoryIndexBuffer32 += loaded.indices.size;
		size += loaded.indices.size;
		loaded.indices.firstSource = surface.surface;
		loaded.indices.secondSource = surface.model;
		buffers.SetupIndices32(loaded.indices);
		SharedMemoryBufferWithOffset newEntry { loaded.indices.buffer, loaded.indices.offset };
		indicesPerKey.insert({ surface.surface, newEntry });
	}
	else
	{
		loaded.indices.size = 0;
		loaded.indices.buffer = entry->second.buffer;
		loaded.indices.offset = entry->second.offset;
	}
}

void Scene::GetIndices16StagingBufferSize(AppState& appState, dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size)
{
	auto entry = indicesPerKey.find(surface.surface);
	if (entry == indicesPerKey.end())
	{
		loaded.indices.size = surface.count * sizeof(uint16_t);
		if (latestSharedMemoryIndexBuffer16 == nullptr || usedInLatestSharedMemoryIndexBuffer16 + loaded.indices.size > latestSharedMemoryIndexBuffer16->size)
		{
			loaded.indices.buffer = new SharedMemoryBuffer { };
			loaded.indices.buffer->CreateIndexBuffer(appState, Constants::memoryBlockSize);
			buffers.MoveToFront(loaded.indices.buffer);
			latestSharedMemoryIndexBuffer16 = loaded.indices.buffer;
			usedInLatestSharedMemoryIndexBuffer16 = 0;
		}
		else
		{
			loaded.indices.buffer = latestSharedMemoryIndexBuffer16;
			loaded.indices.offset = usedInLatestSharedMemoryIndexBuffer16;
		}
		usedInLatestSharedMemoryIndexBuffer16 += loaded.indices.size;
		size += loaded.indices.size;
		loaded.indices.firstSource = surface.surface;
		loaded.indices.secondSource = surface.model;
		buffers.SetupIndices16(loaded.indices);
		SharedMemoryBufferWithOffset newEntry { loaded.indices.buffer, loaded.indices.offset };
		indicesPerKey.insert({ surface.surface, newEntry });
	}
	else
	{
		loaded.indices.size = 0;
		loaded.indices.buffer = entry->second.buffer;
		loaded.indices.offset = entry->second.offset;
	}
}

void Scene::GetIndices32StagingBufferSize(AppState& appState, dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size)
{
	auto entry = indicesPerKey.find(surface.surface);
	if (entry == indicesPerKey.end())
	{
		loaded.indices.size = surface.count * sizeof(uint32_t);
		if (latestSharedMemoryIndexBuffer32 == nullptr || usedInLatestSharedMemoryIndexBuffer32 + loaded.indices.size > latestSharedMemoryIndexBuffer32->size)
		{
			loaded.indices.buffer = new SharedMemoryBuffer { };
			loaded.indices.buffer->CreateIndexBuffer(appState, Constants::memoryBlockSize);
			buffers.MoveToFront(loaded.indices.buffer);
			latestSharedMemoryIndexBuffer32 = loaded.indices.buffer;
			usedInLatestSharedMemoryIndexBuffer32 = 0;
		}
		else
		{
			loaded.indices.buffer = latestSharedMemoryIndexBuffer32;
			loaded.indices.offset = usedInLatestSharedMemoryIndexBuffer32;
		}
		usedInLatestSharedMemoryIndexBuffer32 += loaded.indices.size;
		size += loaded.indices.size;
		loaded.indices.firstSource = surface.surface;
		loaded.indices.secondSource = surface.model;
		buffers.SetupIndices32(loaded.indices);
		SharedMemoryBufferWithOffset newEntry { loaded.indices.buffer, loaded.indices.offset };
		indicesPerKey.insert({ surface.surface, newEntry });
	}
	else
	{
		loaded.indices.size = 0;
		loaded.indices.buffer = entry->second.buffer;
		loaded.indices.offset = entry->second.offset;
	}
}

void Scene::GetAliasIndices16StagingBufferSize(AppState& appState, dalias_t& alias, LoadedAlias& loaded, VkDeviceSize& size)
{
	auto entry = aliasIndicesPerKey.find(alias.aliashdr);
	if (entry == aliasIndicesPerKey.end())
	{
		loaded.indices.size = alias.count * sizeof(uint16_t);
		if (latestSharedMemoryIndexBuffer16 == nullptr || usedInLatestSharedMemoryIndexBuffer16 + loaded.indices.size > latestSharedMemoryIndexBuffer16->size)
		{
			loaded.indices.buffer = new SharedMemoryBuffer { };
			loaded.indices.buffer->CreateIndexBuffer(appState, Constants::memoryBlockSize);
			buffers.MoveToFront(loaded.indices.buffer);
			latestSharedMemoryIndexBuffer16 = loaded.indices.buffer;
			usedInLatestSharedMemoryIndexBuffer16 = 0;
		}
		else
		{
			loaded.indices.buffer = latestSharedMemoryIndexBuffer16;
			loaded.indices.offset = usedInLatestSharedMemoryIndexBuffer16;
		}
		usedInLatestSharedMemoryIndexBuffer16 += loaded.indices.size;
		size += loaded.indices.size;
		loaded.indices.source = alias.aliashdr;
		buffers.SetupAliasIndices16(loaded.indices);
		SharedMemoryBufferWithOffset newEntry { loaded.indices.buffer, loaded.indices.offset };
		aliasIndicesPerKey.insert({ alias.aliashdr, newEntry });
	}
	else
	{
		loaded.indices.size = 0;
		loaded.indices.buffer = entry->second.buffer;
		loaded.indices.offset = entry->second.offset;
	}
}

void Scene::GetAliasIndices32StagingBufferSize(AppState& appState, dalias_t& alias, LoadedAlias& loaded, VkDeviceSize& size)
{
	auto entry = aliasIndicesPerKey.find(alias.aliashdr);
	if (entry == aliasIndicesPerKey.end())
	{
		loaded.indices.size = alias.count * sizeof(uint32_t);
		if (latestSharedMemoryIndexBuffer32 == nullptr || usedInLatestSharedMemoryIndexBuffer32 + loaded.indices.size > latestSharedMemoryIndexBuffer32->size)
		{
			loaded.indices.buffer = new SharedMemoryBuffer { };
			loaded.indices.buffer->CreateIndexBuffer(appState, Constants::memoryBlockSize);
			buffers.MoveToFront(loaded.indices.buffer);
			latestSharedMemoryIndexBuffer32 = loaded.indices.buffer;
			usedInLatestSharedMemoryIndexBuffer32 = 0;
		}
		else
		{
			loaded.indices.buffer = latestSharedMemoryIndexBuffer32;
			loaded.indices.offset = usedInLatestSharedMemoryIndexBuffer32;
		}
		usedInLatestSharedMemoryIndexBuffer32 += loaded.indices.size;
		size += loaded.indices.size;
		loaded.indices.source = alias.aliashdr;
		buffers.SetupAliasIndices32(loaded.indices);
		SharedMemoryBufferWithOffset newEntry { loaded.indices.buffer, loaded.indices.offset };
		aliasIndicesPerKey.insert({ alias.aliashdr, newEntry });
	}
	else
	{
		loaded.indices.size = 0;
		loaded.indices.buffer = entry->second.buffer;
		loaded.indices.offset = entry->second.offset;
	}
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, LoadedSurface& loaded, VkDeviceSize& size)
{
	auto vertexes = ((model_t*)surface.model)->vertexes;
	if (previousVertexes != vertexes)
	{
		auto entry = verticesPerKey.find(vertexes);
		if (entry == verticesPerKey.end())
		{
			loaded.vertices.size = ((model_t*)surface.model)->numvertexes * 3 * sizeof(float);
			loaded.vertices.buffer = new SharedMemoryBuffer { };
			loaded.vertices.buffer->CreateVertexBuffer(appState, loaded.vertices.size);
			buffers.MoveToFront(loaded.vertices.buffer);
			size += loaded.vertices.size;
			loaded.vertices.source = vertexes;
			buffers.SetupVertices(loaded.vertices);
			verticesPerKey.insert({ vertexes, loaded.vertices.buffer });
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
	auto texturePositionEntry = texturePositionsPerKey.find(surface.surface);
	if (texturePositionEntry == texturePositionsPerKey.end())
	{
		loaded.texturePositions.size = 16 * sizeof(float);
		if (latestSharedMemoryTexturePositionBuffer == nullptr || usedInLatestSharedMemoryTexturePositionBuffer + loaded.texturePositions.size > latestSharedMemoryTexturePositionBuffer->size)
		{
			loaded.texturePositions.buffer = new SharedMemoryBuffer { };
			loaded.texturePositions.buffer->CreateVertexBuffer(appState, Constants::memoryBlockSize);
			buffers.MoveToFront(loaded.texturePositions.buffer);
			latestSharedMemoryTexturePositionBuffer = loaded.texturePositions.buffer;
			usedInLatestSharedMemoryTexturePositionBuffer = 0;
		}
		else
		{
			loaded.texturePositions.buffer = latestSharedMemoryTexturePositionBuffer;
			loaded.texturePositions.offset = usedInLatestSharedMemoryTexturePositionBuffer;
		}
		usedInLatestSharedMemoryTexturePositionBuffer += loaded.texturePositions.size;
		size += loaded.texturePositions.size;
		loaded.texturePositions.source = surface.surface;
		buffers.SetupSurfaceTexturePositions(loaded.texturePositions);
		SharedMemoryBufferWithOffset newEntry { loaded.texturePositions.buffer, loaded.texturePositions.offset };
		texturePositionsPerKey.insert({ surface.surface, newEntry });
	}
	else
	{
		loaded.texturePositions.size = 0;
		loaded.texturePositions.buffer = texturePositionEntry->second.buffer;
		loaded.texturePositions.offset = texturePositionEntry->second.offset;
	}
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
	if (previousTexture != surface.texture)
	{
		auto entry = surfaceTexturesPerKey.find(surface.texture);
		if (entry == surfaceTexturesPerKey.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(surface.texture_width, surface.texture_height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, surface.texture_width, surface.texture_height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			textures.MoveToFront(texture);
			loaded.texture.size = surface.texture_size;
			size += loaded.texture.size;
			loaded.texture.texture = texture;
			loaded.texture.source = surface.texture;
			textures.Setup(loaded.texture);
			surfaceTexturesPerKey.insert({ surface.texture, texture });
		}
		else
		{
			loaded.texture.size = 0;
			loaded.texture.texture = entry->second;
		}
		previousTexture = surface.texture;
		previousSharedMemoryTexture = loaded.texture.texture;
	}
	else
	{
		loaded.texture.size = 0;
		loaded.texture.texture = previousSharedMemoryTexture;
	}
	loaded.count = surface.count;
	loaded.originX = surface.origin_x;
	loaded.originY = surface.origin_y;
	loaded.originZ = surface.origin_z;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, LoadedSurfaceRotated& loaded, VkDeviceSize& size)
{
	auto vertexes = ((model_t*)surface.model)->vertexes;
	if (previousVertexes != vertexes)
	{
		auto entry = verticesPerKey.find(vertexes);
		if (entry == verticesPerKey.end())
		{
			loaded.vertices.size = ((model_t*)surface.model)->numvertexes * 3 * sizeof(float);
			loaded.vertices.buffer = new SharedMemoryBuffer { };
			loaded.vertices.buffer->CreateVertexBuffer(appState, loaded.vertices.size);
			buffers.MoveToFront(loaded.vertices.buffer);
			size += loaded.vertices.size;
			loaded.vertices.source = vertexes;
			buffers.SetupVertices(loaded.vertices);
			verticesPerKey.insert({ vertexes, loaded.vertices.buffer });
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
	auto texturePositionEntry = texturePositionsPerKey.find(surface.surface);
	if (texturePositionEntry == texturePositionsPerKey.end())
	{
		loaded.texturePositions.size = 16 * sizeof(float);
		if (latestSharedMemoryTexturePositionBuffer == nullptr || usedInLatestSharedMemoryTexturePositionBuffer + loaded.texturePositions.size > latestSharedMemoryTexturePositionBuffer->size)
		{
			loaded.texturePositions.buffer = new SharedMemoryBuffer { };
			loaded.texturePositions.buffer->CreateVertexBuffer(appState, Constants::memoryBlockSize);
			buffers.MoveToFront(loaded.texturePositions.buffer);
			latestSharedMemoryTexturePositionBuffer = loaded.texturePositions.buffer;
			usedInLatestSharedMemoryTexturePositionBuffer = 0;
		}
		else
		{
			loaded.texturePositions.buffer = latestSharedMemoryTexturePositionBuffer;
			loaded.texturePositions.offset = usedInLatestSharedMemoryTexturePositionBuffer;
		}
		usedInLatestSharedMemoryTexturePositionBuffer += loaded.texturePositions.size;
		size += loaded.texturePositions.size;
		loaded.texturePositions.source = surface.surface;
		buffers.SetupSurfaceTexturePositions(loaded.texturePositions);
		SharedMemoryBufferWithOffset newEntry { loaded.texturePositions.buffer, loaded.texturePositions.offset };
		texturePositionsPerKey.insert({ surface.surface, newEntry });
	}
	else
	{
		loaded.texturePositions.size = 0;
		loaded.texturePositions.buffer = texturePositionEntry->second.buffer;
		loaded.texturePositions.offset = texturePositionEntry->second.offset;
	}
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
			auto lightmap = new Lightmap { };
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
	if (previousTexture != surface.texture)
	{
		auto entry = surfaceTexturesPerKey.find(surface.texture);
		if (entry == surfaceTexturesPerKey.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(surface.texture_width, surface.texture_height)))) + 1;
			auto texture = new SharedMemoryTexture { };
			texture->Create(appState, surface.texture_width, surface.texture_height, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			textures.MoveToFront(texture);
			loaded.texture.size = surface.texture_size;
			size += loaded.texture.size;
			loaded.texture.texture = texture;
			loaded.texture.source = surface.texture;
			textures.Setup(loaded.texture);
			surfaceTexturesPerKey.insert({ surface.texture, texture });
		}
		else
		{
			loaded.texture.size = 0;
			loaded.texture.texture = entry->second;
		}
		previousTexture = surface.texture;
		previousSharedMemoryTexture = loaded.texture.texture;
	}
	else
	{
		loaded.texture.size = 0;
		loaded.texture.texture = previousSharedMemoryTexture;
	}
	loaded.count = surface.count;
	loaded.originX = surface.origin_x;
	loaded.originY = surface.origin_y;
	loaded.originZ = surface.origin_z;
	loaded.yaw = surface.yaw;
	loaded.pitch = surface.pitch;
	loaded.roll = surface.roll;
}

void Scene::GetStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, LoadedTurbulent& loaded, VkDeviceSize& size)
{
	auto vertexes = ((model_t*)turbulent.model)->vertexes;
	if (previousVertexes != vertexes)
	{
		auto entry = verticesPerKey.find(vertexes);
		if (entry == verticesPerKey.end())
		{
			loaded.vertices.size = ((model_t*)turbulent.model)->numvertexes * 3 * sizeof(float);
			loaded.vertices.buffer = new SharedMemoryBuffer { };
			loaded.vertices.buffer->CreateVertexBuffer(appState, loaded.vertices.size);
			buffers.MoveToFront(loaded.vertices.buffer);
			size += loaded.vertices.size;
			loaded.vertices.source = vertexes;
			buffers.SetupVertices(loaded.vertices);
			verticesPerKey.insert({ vertexes, loaded.vertices.buffer });
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
	auto entry = texturePositionsPerKey.find(turbulent.surface);
	if (entry == texturePositionsPerKey.end())
	{
		loaded.texturePositions.size = 16 * sizeof(float);
		if (latestSharedMemoryTexturePositionBuffer == nullptr || usedInLatestSharedMemoryTexturePositionBuffer + loaded.texturePositions.size > latestSharedMemoryTexturePositionBuffer->size)
		{
			loaded.texturePositions.buffer = new SharedMemoryBuffer { };
			loaded.texturePositions.buffer->CreateVertexBuffer(appState, Constants::memoryBlockSize);
			buffers.MoveToFront(loaded.texturePositions.buffer);
			latestSharedMemoryTexturePositionBuffer = loaded.texturePositions.buffer;
			usedInLatestSharedMemoryTexturePositionBuffer = 0;
		}
		else
		{
			loaded.texturePositions.buffer = latestSharedMemoryTexturePositionBuffer;
			loaded.texturePositions.offset = usedInLatestSharedMemoryTexturePositionBuffer;
		}
		usedInLatestSharedMemoryTexturePositionBuffer += loaded.texturePositions.size;
		size += loaded.texturePositions.size;
		loaded.texturePositions.source = turbulent.surface;
		buffers.SetupTurbulentTexturePositions(loaded.texturePositions);
		SharedMemoryBufferWithOffset newEntry { loaded.texturePositions.buffer, loaded.texturePositions.offset };
		texturePositionsPerKey.insert({ turbulent.surface, newEntry });
	}
	else
	{
		loaded.texturePositions.size = 0;
		loaded.texturePositions.buffer = entry->second.buffer;
		loaded.texturePositions.offset = entry->second.offset;
	}
	if (previousTexture != turbulent.texture)
	{
		auto entry = turbulentPerKey.find(turbulent.texture);
		if (entry == turbulentPerKey.end())
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
			turbulentPerKey.insert({ turbulent.texture, texture });
		}
		else
		{
			loaded.texture.size = 0;
			loaded.texture.texture = entry->second;
		}
		previousTexture = turbulent.texture;
		previousSharedMemoryTexture = loaded.texture.texture;
	}
	else
	{
		loaded.texture.size = 0;
		loaded.texture.texture = previousSharedMemoryTexture;
	}
	loaded.count = turbulent.count;
	loaded.originX = turbulent.origin_x;
	loaded.originY = turbulent.origin_y;
	loaded.originZ = turbulent.origin_z;
}

void Scene::GetStagingBufferSize(AppState& appState, const dturbulentrotated_t& turbulent, LoadedTurbulentRotated& loaded, VkDeviceSize& size)
{
	auto vertexes = ((model_t*)turbulent.model)->vertexes;
	if (previousVertexes != vertexes)
	{
		auto entry = verticesPerKey.find(vertexes);
		if (entry == verticesPerKey.end())
		{
			loaded.vertices.size = ((model_t*)turbulent.model)->numvertexes * 3 * sizeof(float);
			loaded.vertices.buffer = new SharedMemoryBuffer { };
			loaded.vertices.buffer->CreateVertexBuffer(appState, loaded.vertices.size);
			buffers.MoveToFront(loaded.vertices.buffer);
			size += loaded.vertices.size;
			loaded.vertices.source = vertexes;
			buffers.SetupVertices(loaded.vertices);
			verticesPerKey.insert({ vertexes, loaded.vertices.buffer });
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
	auto entry = texturePositionsPerKey.find(turbulent.surface);
	if (entry == texturePositionsPerKey.end())
	{
		loaded.texturePositions.size = 16 * sizeof(float);
		if (latestSharedMemoryTexturePositionBuffer == nullptr || usedInLatestSharedMemoryTexturePositionBuffer + loaded.texturePositions.size > latestSharedMemoryTexturePositionBuffer->size)
		{
			loaded.texturePositions.buffer = new SharedMemoryBuffer { };
			loaded.texturePositions.buffer->CreateVertexBuffer(appState, Constants::memoryBlockSize);
			buffers.MoveToFront(loaded.texturePositions.buffer);
			latestSharedMemoryTexturePositionBuffer = loaded.texturePositions.buffer;
			usedInLatestSharedMemoryTexturePositionBuffer = 0;
		}
		else
		{
			loaded.texturePositions.buffer = latestSharedMemoryTexturePositionBuffer;
			loaded.texturePositions.offset = usedInLatestSharedMemoryTexturePositionBuffer;
		}
		usedInLatestSharedMemoryTexturePositionBuffer += loaded.texturePositions.size;
		size += loaded.texturePositions.size;
		loaded.texturePositions.source = turbulent.surface;
		buffers.SetupTurbulentTexturePositions(loaded.texturePositions);
		SharedMemoryBufferWithOffset newEntry { loaded.texturePositions.buffer, loaded.texturePositions.offset };
		texturePositionsPerKey.insert({ turbulent.surface, newEntry });
	}
	else
	{
		loaded.texturePositions.size = 0;
		loaded.texturePositions.buffer = entry->second.buffer;
		loaded.texturePositions.offset = entry->second.offset;
	}
	if (previousTexture != turbulent.texture)
	{
		auto entry = turbulentPerKey.find(turbulent.texture);
		if (entry == turbulentPerKey.end())
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
			turbulentPerKey.insert({ turbulent.texture, texture });
		}
		else
		{
			loaded.texture.size = 0;
			loaded.texture.texture = entry->second;
		}
		previousTexture = turbulent.texture;
		previousSharedMemoryTexture = loaded.texture.texture;
	}
	else
	{
		loaded.texture.size = 0;
		loaded.texture.texture = previousSharedMemoryTexture;
	}
	loaded.count = turbulent.count;
	loaded.originX = turbulent.origin_x;
	loaded.originY = turbulent.origin_y;
	loaded.originZ = turbulent.origin_z;
	loaded.yaw = turbulent.yaw;
	loaded.pitch = turbulent.pitch;
	loaded.roll = turbulent.roll;
}

void Scene::Reset()
{
	D_ResetLists();
	aliasTexturesPerKey.clear();
	turbulentPerKey.clear();
	spritesPerKey.clear();
	surfaceTexturesPerKey.clear();
	textures.DisposeFront();
	lightmaps.DisposeFront();
	buffers.DisposeFront();
	latestTextureDescriptorSets = nullptr;
	latestTextureSharedMemory = nullptr;
	usedInLatestTextureSharedMemory = 0;
	latestSharedMemoryIndexBuffer32 = nullptr;
	usedInLatestSharedMemoryIndexBuffer32 = 0;
	latestSharedMemoryIndexBuffer16 = nullptr;
	usedInLatestSharedMemoryIndexBuffer16 = 0;
	latestSharedMemoryTexturePositionBuffer = nullptr;
	usedInLatestSharedMemoryTexturePositionBuffer = 0;
	latestBufferSharedMemory = nullptr;
	usedInLatestBufferSharedMemory = 0;
	Skybox::MoveToPrevious(*this);
	aliasIndicesPerKey.clear();
	indicesPerKey.clear();
	aliasVerticesPerKey.clear();
	texturePositionsPerKey.clear();
	verticesPerKey.clear();
}
