#include "Scene.h"
#include "AppState.h"
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
	appState.Screen.VulkanImages.resize(imageCount);
	appState.Screen.Images.resize(imageCount);
	appState.Screen.CommandBuffers.resize(imageCount);
	appState.Screen.SubmitInfo.resize(imageCount);
	for (auto i = 0; i < imageCount; i++)
	{
		appState.Screen.VulkanImages[i] = { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR };
		appState.Screen.Images[i] = reinterpret_cast<XrSwapchainImageBaseHeader*>(&appState.Screen.VulkanImages[i]);
		CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &appState.Screen.CommandBuffers[i]));
		appState.Screen.SubmitInfo[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		appState.Screen.SubmitInfo[i].commandBufferCount = 1;
		appState.Screen.SubmitInfo[i].pCommandBuffers = &appState.Screen.CommandBuffers[i];
	}
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Screen.Swapchain, imageCount, &imageCount, appState.Screen.Images[0]));
	appState.Screen.Data.resize(swapchainCreateInfo.width * swapchainCreateInfo.height, 255 << 24);
	ImageAsset play;
	play.Open("play.png", app);
	CopyImage(appState, play.image, appState.Screen.Data.data() + ((appState.ScreenHeight - play.height) * appState.ScreenWidth + appState.ScreenWidth - play.width) / 2, play.width, play.height);
	play.Close();
	AddBorder(appState, appState.Screen.Data);
	appState.Screen.StagingBuffer.CreateStagingBuffer(appState, appState.Screen.Data.size() * sizeof(uint32_t));
	CHECK_VKCMD(vkMapMemory(appState.Device, appState.Screen.StagingBuffer.memory, 0, VK_WHOLE_SIZE, 0, &appState.Screen.StagingBuffer.mapped));

	swapchainCreateInfo.width = appState.ScreenWidth;
	swapchainCreateInfo.height = appState.ScreenHeight / 2;
	CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &appState.Keyboard.Screen.Swapchain));
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Keyboard.Screen.Swapchain, 0, &imageCount, nullptr));
	appState.Keyboard.Screen.VulkanImages.resize(imageCount);
	appState.Keyboard.Screen.Images.resize(imageCount);
	appState.Keyboard.Screen.CommandBuffers.resize(imageCount);
	appState.Keyboard.Screen.SubmitInfo.resize(imageCount);
	for (auto i = 0; i < imageCount; i++)
	{
		appState.Keyboard.Screen.VulkanImages[i] = { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR };
		appState.Keyboard.Screen.Images[i] = reinterpret_cast<XrSwapchainImageBaseHeader*>(&appState.Keyboard.Screen.VulkanImages[i]);
		CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &appState.Keyboard.Screen.CommandBuffers[i]));
		appState.Keyboard.Screen.SubmitInfo[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		appState.Keyboard.Screen.SubmitInfo[i].commandBufferCount = 1;
		appState.Keyboard.Screen.SubmitInfo[i].pCommandBuffers = &appState.Keyboard.Screen.CommandBuffers[i];
	}
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Keyboard.Screen.Swapchain, imageCount, &imageCount, appState.Keyboard.Screen.Images[0]));
	appState.Keyboard.Screen.Data.resize(swapchainCreateInfo.width * swapchainCreateInfo.height, 255 << 24);
	appState.Keyboard.Screen.StagingBuffer.CreateStagingBuffer(appState, appState.Keyboard.Screen.Data.size() * sizeof(uint32_t));
	CHECK_VKCMD(vkMapMemory(appState.Device, appState.Keyboard.Screen.StagingBuffer.memory, 0, VK_WHOLE_SIZE, 0, &appState.Keyboard.Screen.StagingBuffer.mapped));

	CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &setupCommandBuffer));
	CHECK_VKCMD(vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));

	ImageAsset floorImage;
	floorImage.Open("floor.png", app);
	appState.Scene.floorTexture.Create(appState, floorImage.width, floorImage.height, Constants::colorFormat, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	ImageAsset controller;
	controller.Open("controller.png", app);
	appState.Scene.controllerTexture.Create(appState, controller.width, controller.height, Constants::colorFormat, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	VkDeviceSize stagingBufferSize = (floorImage.width * floorImage.height + controller.width * controller.height + 2 * appState.ScreenWidth * appState.ScreenHeight) * sizeof(uint32_t);
	Buffer stagingBuffer;
	stagingBuffer.CreateStagingBuffer(appState, stagingBufferSize);
	CHECK_VKCMD(vkMapMemory(appState.Device, stagingBuffer.memory, 0, VK_WHOLE_SIZE, 0, &stagingBuffer.mapped));
	memcpy(stagingBuffer.mapped, floorImage.image, floorImage.width * floorImage.height * sizeof(uint32_t));
	floorImage.Close();
	size_t offset = floorImage.width * floorImage.height;
	memcpy((uint32_t*)stagingBuffer.mapped + offset, controller.image, controller.width * controller.height * sizeof(uint32_t));
	offset = 0;
	appState.Scene.floorTexture.Fill(appState, &stagingBuffer, offset, setupCommandBuffer);
	offset += floorImage.width * floorImage.height * sizeof(uint32_t);
	appState.Scene.controllerTexture.Fill(appState, &stagingBuffer, offset, setupCommandBuffer);
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
	appState.LeftArrowsVulkanImages.resize(imageCount);
	appState.LeftArrowsImages.resize(imageCount);
	for (auto i = 0; i < imageCount; i++)
	{
		appState.LeftArrowsVulkanImages[i] = { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR };
		appState.LeftArrowsImages[i] = reinterpret_cast<XrSwapchainImageBaseHeader*>(&appState.LeftArrowsVulkanImages[i]);
	}
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.LeftArrowsSwapchain, imageCount, &imageCount, appState.LeftArrowsImages[0]));

	VkBufferCreateInfo bufferCreateInfo { };
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = swapchainCreateInfo.width * swapchainCreateInfo.height * 4;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkBuffer buffer;
	VkMemoryRequirements memoryRequirements;
	VkMemoryAllocateInfo memoryAllocateInfo { };
	VkDeviceMemory memoryBlock;
	VkImageMemoryBarrier imageMemoryBarrier { };
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
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

	auto image = appState.LeftArrowsVulkanImages[swapchainImageIndex].image;
	imageMemoryBarrier.image = image;

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
	vkCmdCopyBufferToImage(setupCommandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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
	appState.RightArrowsVulkanImages.resize(imageCount);
	appState.RightArrowsImages.resize(imageCount);
	for (auto i = 0; i < imageCount; i++)
	{
		appState.RightArrowsVulkanImages[i] = { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR };
		appState.RightArrowsImages[i] = reinterpret_cast<XrSwapchainImageBaseHeader*>(&appState.RightArrowsVulkanImages[i]);
	}
	CHECK_XRCMD(xrEnumerateSwapchainImages(appState.RightArrowsSwapchain, imageCount, &imageCount, appState.RightArrowsImages[0]));
 
 	bufferCreateInfo.size = swapchainCreateInfo.width * swapchainCreateInfo.height * 4;
	region.imageExtent.width = swapchainCreateInfo.width;
	region.imageExtent.height = swapchainCreateInfo.height;

	CHECK_XRCMD(xrAcquireSwapchainImage(appState.RightArrowsSwapchain, &acquireInfo, &swapchainImageIndex));

	CHECK_XRCMD(xrWaitSwapchainImage(appState.RightArrowsSwapchain, &waitInfo));

	CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &setupCommandBuffer));
	CHECK_VKCMD(vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));

	image = appState.RightArrowsVulkanImages[swapchainImageIndex].image;
	imageMemoryBarrier.image = image;

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
	vkCmdCopyBufferToImage(setupCommandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	CHECK_VKCMD(vkEndCommandBuffer(setupCommandBuffer));
	CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));

	CHECK_VKCMD(vkQueueWaitIdle(appState.Queue));

	CHECK_XRCMD(xrReleaseSwapchainImage(appState.RightArrowsSwapchain, &releaseInfo));

	vkFreeCommandBuffers(appState.Device, appState.CommandPool, 1, &setupCommandBuffer);
	vkDestroyBuffer(appState.Device, buffer, nullptr);
	vkFreeMemory(appState.Device, memoryBlock, nullptr);

	CreateShader(appState, app, "shaders/surface.vert.spv", &surfaceVertex);
	CreateShader(appState, app, "shaders/surface_rotated.vert.spv", &surfaceRotatedVertex);
	CreateShader(appState, app, "shaders/surface.frag.spv", &surfaceFragment);
	CreateShader(appState, app, "shaders/fence.frag.spv", &fenceFragment);
	CreateShader(appState, app, "shaders/sprite.vert.spv", &spriteVertex);
	CreateShader(appState, app, "shaders/sprite.frag.spv", &spriteFragment);
	CreateShader(appState, app, "shaders/turbulent.vert.spv", &turbulentVertex);
	CreateShader(appState, app, "shaders/turbulent.frag.spv", &turbulentFragment);
	CreateShader(appState, app, "shaders/alias.vert.spv", &aliasVertex);
	CreateShader(appState, app, "shaders/alias.frag.spv", &aliasFragment);
	CreateShader(appState, app, "shaders/viewmodel.vert.spv", &viewmodelVertex);
	CreateShader(appState, app, "shaders/viewmodel.frag.spv", &viewmodelFragment);
	CreateShader(appState, app, "shaders/colored.vert.spv", &coloredVertex);
	CreateShader(appState, app, "shaders/colored.frag.spv", &coloredFragment);
	CreateShader(appState, app, "shaders/sky.vert.spv", &skyVertex);
	CreateShader(appState, app, "shaders/sky.frag.spv", &skyFragment);
	CreateShader(appState, app, "shaders/floor.vert.spv", &floorVertex);
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
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
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
	PipelineAttributes surfaceAttributes { };
	surfaceAttributes.vertexAttributes.resize(9);
	surfaceAttributes.vertexBindings.resize(3);
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
	surfaceAttributes.vertexAttributes[5].location = 5;
	surfaceAttributes.vertexAttributes[5].binding = 2;
	surfaceAttributes.vertexAttributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceAttributes.vertexAttributes[6].location = 6;
	surfaceAttributes.vertexAttributes[6].binding = 2;
	surfaceAttributes.vertexAttributes[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceAttributes.vertexAttributes[6].offset = 4 * sizeof(float);
	surfaceAttributes.vertexAttributes[7].location = 7;
	surfaceAttributes.vertexAttributes[7].binding = 2;
	surfaceAttributes.vertexAttributes[7].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceAttributes.vertexAttributes[7].offset = 8 * sizeof(float);
	surfaceAttributes.vertexAttributes[8].location = 8;
	surfaceAttributes.vertexAttributes[8].binding = 2;
	surfaceAttributes.vertexAttributes[8].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	surfaceAttributes.vertexAttributes[8].offset = 12 * sizeof(float);
	surfaceAttributes.vertexBindings[2].binding = 2;
	surfaceAttributes.vertexBindings[2].stride = 16 * sizeof(float);
	surfaceAttributes.vertexBindings[2].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
	surfaceAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	surfaceAttributes.vertexInputState.vertexBindingDescriptionCount = surfaceAttributes.vertexBindings.size();
	surfaceAttributes.vertexInputState.pVertexBindingDescriptions = surfaceAttributes.vertexBindings.data();
	surfaceAttributes.vertexInputState.vertexAttributeDescriptionCount = surfaceAttributes.vertexAttributes.size();
	surfaceAttributes.vertexInputState.pVertexAttributeDescriptions = surfaceAttributes.vertexAttributes.data();
	surfaceAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	surfaceAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	PipelineAttributes spriteAttributes { };
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
	PipelineAttributes colormappedAttributes { };
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
	PipelineAttributes coloredAttributes { };
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
	PipelineAttributes consoleAttributes { };
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
	CHECK_VKCMD(vkCreateDescriptorSetLayout(appState.Device, &descriptorSetLayoutCreateInfo, nullptr, &singleBufferLayout));
	descriptorSetBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetBindings[1].descriptorCount = 1;
	descriptorSetBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutCreateInfo.bindingCount = 2;
	CHECK_VKCMD(vkCreateDescriptorSetLayout(appState.Device, &descriptorSetLayoutCreateInfo, nullptr, &bufferAndImageLayout));
	descriptorSetBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetBindings[2].descriptorCount = 1;
	descriptorSetBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutCreateInfo.bindingCount = 3;
	CHECK_VKCMD(vkCreateDescriptorSetLayout(appState.Device, &descriptorSetLayoutCreateInfo, nullptr, &bufferAndTwoImagesLayout));
	descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	CHECK_VKCMD(vkCreateDescriptorSetLayout(appState.Device, &descriptorSetLayoutCreateInfo, nullptr, &singleImageLayout));
	descriptorSetLayoutCreateInfo.bindingCount = 2;
	CHECK_VKCMD(vkCreateDescriptorSetLayout(appState.Device, &descriptorSetLayoutCreateInfo, nullptr, &doubleImageLayout));
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
	pushConstantInfo.size = 6 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfaces.pipelineLayout));
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
	pushConstantInfo.size = 9 * sizeof(float);
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
	pushConstantInfo.size = 6 * sizeof(float);
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
	pushConstantInfo.size = 9 * sizeof(float);
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
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	descriptorSetLayouts[0] = bufferAndImageLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pushConstantInfo.size = 6 * sizeof(float);
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
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = 7 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulent.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = turbulent.stages.size();
	graphicsPipelineCreateInfo.pStages = turbulent.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &surfaceAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = turbulent.pipelineLayout;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulent.pipeline));
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
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
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
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = 13 * sizeof(float);
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
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
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &console.pipelineLayout));
	graphicsPipelineCreateInfo.stageCount = console.stages.size();
	graphicsPipelineCreateInfo.pStages = console.stages.data();
	graphicsPipelineCreateInfo.pVertexInputState = &consoleAttributes.vertexInputState;
	graphicsPipelineCreateInfo.pInputAssemblyState = &consoleAttributes.inputAssemblyState;
	graphicsPipelineCreateInfo.layout = console.pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = appState.RenderPass;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &console.pipeline));
	matrices.CreateUniformBuffer(appState, 2 * 2 * sizeof(XrMatrix4x4f));
	VkSamplerCreateInfo samplerCreateInfo { };
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.maxLod = 0;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
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
	VkShaderModuleCreateInfo moduleCreateInfo { };
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
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
	latestBufferSharedMemory = nullptr;
	usedInLatestBufferSharedMemory = 0;
	aliasIndicesPerKey.clear();
	indicesPerKey.clear();
	aliasVerticesPerKey.clear();
	texturePositionsPerKey.clear();
	verticesPerKey.clear();
}
