#include "AppState.h"
#include "Scene.h" // This header is specified second in the list to allow headers in AppState.h to include the core engine structs
#include "Utils.h"
#include "Constants.h"
#include "ImageAsset.h"
#include "MemoryAllocateInfo.h"
#include "PipelineAttributes.h"
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
#include "RendererNames.h"
#endif

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

void Scene::Create(AppState& appState)
{
	latestMemory.resize(appState.MemoryProperties.memoryTypeCount);

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

    appState.ScreenData.assign(swapchainCreateInfo.width * swapchainCreateInfo.height, 255 << 24);
    ImageAsset play;
    play.Open("play.png", appState.FileLoader);
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
		VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        updateMemoryAllocateInfo(appState, memoryRequirements, properties, memoryAllocateInfo, true);
        CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &consoleTexture.memory));
        CHECK_VKCMD(vkBindImageMemory(appState.Device, consoleTexture.image, consoleTexture.memory, 0));

        auto& statusBarTexture = appState.StatusBarTextures[i];
        statusBarTexture.width = appState.ScreenWidth;
        statusBarTexture.height = (SBAR_HEIGHT + 24) * Constants::screenToConsoleMultiplier;
        statusBarTexture.mipCount = 1;
        statusBarTexture.layerCount = 1;
        imageCreateInfo.extent.width = statusBarTexture.width;
        imageCreateInfo.extent.height = statusBarTexture.height;
        imageCreateInfo.mipLevels = statusBarTexture.mipCount;
        imageCreateInfo.arrayLayers = statusBarTexture.layerCount;
        CHECK_VKCMD(vkCreateImage(appState.Device, &imageCreateInfo, nullptr, &statusBarTexture.image));
        vkGetImageMemoryRequirements(appState.Device, statusBarTexture.image, &memoryRequirements);
		properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        updateMemoryAllocateInfo(appState, memoryRequirements, properties, memoryAllocateInfo, true);
        CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &statusBarTexture.memory));
        CHECK_VKCMD(vkBindImageMemory(appState.Device, statusBarTexture.image, statusBarTexture.memory, 0));
    }

    swapchainCreateInfo.width = appState.ScreenWidth;
    swapchainCreateInfo.height = appState.ScreenHeight / 2;
    CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &appState.Keyboard.Screen.swapchain));

    CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Keyboard.Screen.swapchain, 0, &imageCount, nullptr));

    appState.Keyboard.Screen.swapchainImages.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
    CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Keyboard.Screen.swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)appState.Keyboard.Screen.swapchainImages.data()));

    VkCommandBufferAllocateInfo commandBufferAllocateInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    commandBufferAllocateInfo.commandPool = appState.SetupCommandPool;
    commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer setupCommandBuffer;

	CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &setupCommandBuffer));

	VkSubmitInfo setupSubmitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	setupSubmitInfo.commandBufferCount = 1;
	setupSubmitInfo.pCommandBuffers = &setupCommandBuffer;

	VkCommandBufferBeginInfo commandBufferBeginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    CHECK_VKCMD(vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));

    ImageAsset floorImage;
    floorImage.Open("floor.png", appState.FileLoader);
    floorTexture.Create(appState, floorImage.width, floorImage.height, Constants::colorFormat, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    ImageAsset controller;
    controller.Open("controller.png", appState.FileLoader);
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
    vkFreeCommandBuffers(appState.Device, appState.SetupCommandPool, 1, &setupCommandBuffer);
    stagingBuffer.Delete(appState);
    appState.NoGameDataData.resize(appState.ScreenWidth * appState.ScreenHeight, 255 << 24);
    ImageAsset noGameData;
    noGameData.Open("nogamedata.png", appState.FileLoader);
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

	Buffer buffer;
	buffer.CreateSourceBuffer(appState, swapchainCreateInfo.width * swapchainCreateInfo.height * 4);

    CHECK_VKCMD(vkMapMemory(appState.Device, buffer.memory, 0, VK_WHOLE_SIZE, 0, &buffer.mapped));

    ImageAsset leftArrows;
    leftArrows.Open("leftarrows.png", appState.FileLoader);
    memcpy(buffer.mapped, leftArrows.image, leftArrows.width * leftArrows.height * leftArrows.components);

    appState.copyBarrier.image = swapchainImages[swapchainImageIndex].image;
    vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.copyBarrier);

    region.imageSubresource.baseArrayLayer = 0;
    vkCmdCopyBufferToImage(setupCommandBuffer, buffer.buffer, swapchainImages[swapchainImageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    appState.submitBarrier.image = swapchainImages[swapchainImageIndex].image;
    vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.submitBarrier);

    CHECK_VKCMD(vkEndCommandBuffer(setupCommandBuffer));
    CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));

    CHECK_VKCMD(vkQueueWaitIdle(appState.Queue));

    XrSwapchainImageReleaseInfo releaseInfo { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
    CHECK_XRCMD(xrReleaseSwapchainImage(appState.LeftArrowsSwapchain, &releaseInfo));

    vkFreeCommandBuffers(appState.Device, appState.SetupCommandPool, 1, &setupCommandBuffer);

    CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &appState.RightArrowsSwapchain));

    CHECK_XRCMD(xrEnumerateSwapchainImages(appState.RightArrowsSwapchain, 0, &imageCount, nullptr));

    swapchainImages.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
    CHECK_XRCMD(xrEnumerateSwapchainImages(appState.RightArrowsSwapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)swapchainImages.data()));

    CHECK_XRCMD(xrAcquireSwapchainImage(appState.RightArrowsSwapchain, nullptr, &swapchainImageIndex));

    CHECK_XRCMD(xrWaitSwapchainImage(appState.RightArrowsSwapchain, &waitInfo));

    CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &setupCommandBuffer));
    CHECK_VKCMD(vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));

    ImageAsset rightArrows;
    rightArrows.Open("rightarrows.png", appState.FileLoader);
    memcpy(buffer.mapped, rightArrows.image, rightArrows.width * rightArrows.height * rightArrows.components);

    appState.copyBarrier.image = swapchainImages[swapchainImageIndex].image;
    vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.copyBarrier);

    region.imageSubresource.baseArrayLayer = 0;
    vkCmdCopyBufferToImage(setupCommandBuffer, buffer.buffer, swapchainImages[swapchainImageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    appState.submitBarrier.image = swapchainImages[swapchainImageIndex].image;
    vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.submitBarrier);

    CHECK_VKCMD(vkEndCommandBuffer(setupCommandBuffer));
    CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));

    CHECK_VKCMD(vkQueueWaitIdle(appState.Queue));

    CHECK_XRCMD(xrReleaseSwapchainImage(appState.RightArrowsSwapchain, &releaseInfo));

    vkFreeCommandBuffers(appState.Device, appState.SetupCommandPool, 1, &setupCommandBuffer);

	buffer.Delete(appState);

    VkShaderModule surfaceVertex;
    CreateShader(appState, "shaders/surface.vert.spv", &surfaceVertex);
    VkShaderModule surfaceFragment;
    CreateShader(appState, "shaders/surface.frag.spv", &surfaceFragment);
    VkShaderModule surfaceColoredLightsFragment;
    CreateShader(appState, "shaders/surface_colored_lights.frag.spv", &surfaceColoredLightsFragment);
    VkShaderModule surfaceRGBAVertex;
    CreateShader(appState, "shaders/surface_rgba.vert.spv", &surfaceRGBAVertex);
    VkShaderModule surfaceRGBAFragment;
    CreateShader(appState, "shaders/surface_rgba.frag.spv", &surfaceRGBAFragment);
    VkShaderModule surfaceRGBAColoredLightsFragment;
    CreateShader(appState, "shaders/surface_rgba_colored_lights.frag.spv", &surfaceRGBAColoredLightsFragment);
    VkShaderModule surfaceRGBANoGlowFragment;
    CreateShader(appState, "shaders/surface_rgba_no_glow.frag.spv", &surfaceRGBANoGlowFragment);
    VkShaderModule surfaceRGBANoGlowColoredLightsFragment;
    CreateShader(appState, "shaders/surface_rgba_no_glow_colored_lights.frag.spv", &surfaceRGBANoGlowColoredLightsFragment);
    VkShaderModule surfaceRotatedVertex;
    CreateShader(appState, "shaders/surface_rotated.vert.spv", &surfaceRotatedVertex);
	VkShaderModule surfaceRotatedFragment;
	CreateShader(appState, "shaders/surface_rotated.frag.spv", &surfaceRotatedFragment);
	VkShaderModule surfaceRotatedColoredLightsFragment;
	CreateShader(appState, "shaders/surface_rotated_colored_lights.frag.spv", &surfaceRotatedColoredLightsFragment);
    VkShaderModule surfaceRotatedRGBAVertex;
    CreateShader(appState, "shaders/surface_rotated_rgba.vert.spv", &surfaceRotatedRGBAVertex);
	VkShaderModule surfaceRotatedRGBAFragment;
	CreateShader(appState, "shaders/surface_rotated_rgba.frag.spv", &surfaceRotatedRGBAFragment);
	VkShaderModule surfaceRotatedRGBAColoredLightsFragment;
	CreateShader(appState, "shaders/surface_rotated_rgba_colored_lights.frag.spv", &surfaceRotatedRGBAColoredLightsFragment);
	VkShaderModule surfaceRotatedRGBANoGlowFragment;
	CreateShader(appState, "shaders/surface_rotated_rgba_no_glow.frag.spv", &surfaceRotatedRGBANoGlowFragment);
	VkShaderModule surfaceRotatedRGBANoGlowColoredLightsFragment;
	CreateShader(appState, "shaders/surface_rotated_rgba_no_glow_colored_lights.frag.spv", &surfaceRotatedRGBANoGlowColoredLightsFragment);
    VkShaderModule fenceFragment;
    CreateShader(appState, "shaders/fence.frag.spv", &fenceFragment);
    VkShaderModule fenceColoredLightsFragment;
    CreateShader(appState, "shaders/fence_colored_lights.frag.spv", &fenceColoredLightsFragment);
    VkShaderModule fenceRGBAFragment;
    CreateShader(appState, "shaders/fence_rgba.frag.spv", &fenceRGBAFragment);
    VkShaderModule fenceRGBAColoredLightsFragment;
    CreateShader(appState, "shaders/fence_rgba_colored_lights.frag.spv", &fenceRGBAColoredLightsFragment);
    VkShaderModule fenceRGBANoGlowFragment;
    CreateShader(appState, "shaders/fence_rgba_no_glow.frag.spv", &fenceRGBANoGlowFragment);
    VkShaderModule fenceRGBANoGlowColoredLightsFragment;
    CreateShader(appState, "shaders/fence_rgba_no_glow_colored_lights.frag.spv", &fenceRGBANoGlowColoredLightsFragment);
    VkShaderModule fenceRotatedFragment;
    CreateShader(appState, "shaders/fence_rotated.frag.spv", &fenceRotatedFragment);
    VkShaderModule fenceRotatedColoredLightsFragment;
    CreateShader(appState, "shaders/fence_rotated_colored_lights.frag.spv", &fenceRotatedColoredLightsFragment);
    VkShaderModule fenceRotatedRGBAFragment;
    CreateShader(appState, "shaders/fence_rotated_rgba.frag.spv", &fenceRotatedRGBAFragment);
    VkShaderModule fenceRotatedRGBAColoredLightsFragment;
    CreateShader(appState, "shaders/fence_rotated_rgba_colored_lights.frag.spv", &fenceRotatedRGBAColoredLightsFragment);
    VkShaderModule fenceRotatedRGBANoGlowFragment;
    CreateShader(appState, "shaders/fence_rotated_rgba_no_glow.frag.spv", &fenceRotatedRGBANoGlowFragment);
    VkShaderModule fenceRotatedRGBANoGlowColoredLightsFragment;
    CreateShader(appState, "shaders/fence_rotated_rgba_no_glow_colored_lights.frag.spv", &fenceRotatedRGBANoGlowColoredLightsFragment);
    VkShaderModule turbulentVertex;
    CreateShader(appState, "shaders/turbulent.vert.spv", &turbulentVertex);
    VkShaderModule turbulentFragment;
    CreateShader(appState, "shaders/turbulent.frag.spv", &turbulentFragment);
    VkShaderModule turbulentRGBAFragment;
    CreateShader(appState, "shaders/turbulent_rgba.frag.spv", &turbulentRGBAFragment);
    VkShaderModule turbulentLitFragment;
    CreateShader(appState, "shaders/turbulent_lit.frag.spv", &turbulentLitFragment);
    VkShaderModule turbulentColoredLightsFragment;
    CreateShader(appState, "shaders/turbulent_colored_lights.frag.spv", &turbulentColoredLightsFragment);
    VkShaderModule turbulentRGBALitFragment;
    CreateShader(appState, "shaders/turbulent_rgba_lit.frag.spv", &turbulentRGBALitFragment);
    VkShaderModule turbulentRGBAColoredLightsFragment;
    CreateShader(appState, "shaders/turbulent_rgba_colored_lights.frag.spv", &turbulentRGBAColoredLightsFragment);
	VkShaderModule turbulentRotatedVertex;
	CreateShader(appState, "shaders/turbulent_rotated.vert.spv", &turbulentRotatedVertex);
	VkShaderModule turbulentRotatedFragment;
	CreateShader(appState, "shaders/turbulent_rotated.frag.spv", &turbulentRotatedFragment);
	VkShaderModule turbulentRotatedRGBAFragment;
	CreateShader(appState, "shaders/turbulent_rotated_rgba.frag.spv", &turbulentRotatedRGBAFragment);
    VkShaderModule turbulentRotatedLitFragment;
    CreateShader(appState, "shaders/turbulent_rotated_lit.frag.spv", &turbulentRotatedLitFragment);
    VkShaderModule turbulentRotatedColoredLightsFragment;
    CreateShader(appState, "shaders/turbulent_rotated_colored_lights.frag.spv", &turbulentRotatedColoredLightsFragment);
    VkShaderModule turbulentRotatedRGBALitFragment;
    CreateShader(appState, "shaders/turbulent_rotated_rgba_lit.frag.spv", &turbulentRotatedRGBALitFragment);
    VkShaderModule turbulentRotatedRGBAColoredLightsFragment;
    CreateShader(appState, "shaders/turbulent_rotated_rgba_colored_lights.frag.spv", &turbulentRotatedRGBAColoredLightsFragment);
    VkShaderModule spriteVertex;
    CreateShader(appState, "shaders/sprite.vert.spv", &spriteVertex);
    VkShaderModule spriteFragment;
    CreateShader(appState, "shaders/sprite.frag.spv", &spriteFragment);
    VkShaderModule aliasVertex;
    CreateShader(appState, "shaders/alias.vert.spv", &aliasVertex);
    VkShaderModule aliasFragment;
    CreateShader(appState, "shaders/alias.frag.spv", &aliasFragment);
	VkShaderModule aliasHoleyFragment;
	CreateShader(appState, "shaders/alias_holey.frag.spv", &aliasHoleyFragment);
	VkShaderModule aliasColoredLightsVertex;
	CreateShader(appState, "shaders/alias_colored_lights.vert.spv", &aliasColoredLightsVertex);
	VkShaderModule aliasColoredLightsFragment;
	CreateShader(appState, "shaders/alias_colored_lights.frag.spv", &aliasColoredLightsFragment);
	VkShaderModule aliasHoleyColoredLightsFragment;
	CreateShader(appState, "shaders/alias_holey_colored_lights.frag.spv", &aliasHoleyColoredLightsFragment);
	VkShaderModule aliasAlphaVertex;
	CreateShader(appState, "shaders/alias_alpha.vert.spv", &aliasAlphaVertex);
	VkShaderModule aliasAlphaFragment;
	CreateShader(appState, "shaders/alias_alpha.frag.spv", &aliasAlphaFragment);
	VkShaderModule aliasAlphaColoredLightsVertex;
	CreateShader(appState, "shaders/alias_alpha_colored_lights.vert.spv", &aliasAlphaColoredLightsVertex);
	VkShaderModule aliasAlphaColoredLightsFragment;
	CreateShader(appState, "shaders/alias_alpha_colored_lights.frag.spv", &aliasAlphaColoredLightsFragment);
	VkShaderModule aliasHoleyAlphaFragment;
	CreateShader(appState, "shaders/alias_holey_alpha.frag.spv", &aliasHoleyAlphaFragment);
	VkShaderModule aliasHoleyAlphaColoredLightsFragment;
	CreateShader(appState, "shaders/alias_holey_alpha_colored_lights.frag.spv", &aliasHoleyAlphaColoredLightsFragment);
    VkShaderModule viewmodelVertex;
    CreateShader(appState, "shaders/viewmodel.vert.spv", &viewmodelVertex);
    VkShaderModule viewmodelFragment;
    CreateShader(appState, "shaders/viewmodel.frag.spv", &viewmodelFragment);
    VkShaderModule viewmodelHoleyFragment;
    CreateShader(appState, "shaders/viewmodel_holey.frag.spv", &viewmodelHoleyFragment);
	VkShaderModule viewmodelColoredLightsVertex;
	CreateShader(appState, "shaders/viewmodel_colored_lights.vert.spv", &viewmodelColoredLightsVertex);
	VkShaderModule viewmodelColoredLightsFragment;
	CreateShader(appState, "shaders/viewmodel_colored_lights.frag.spv", &viewmodelColoredLightsFragment);
	VkShaderModule viewmodelHoleyColoredLightsFragment;
	CreateShader(appState, "shaders/viewmodel_holey_colored_lights.frag.spv", &viewmodelHoleyColoredLightsFragment);
    VkShaderModule particleVertex;
    CreateShader(appState, "shaders/particle.vert.spv", &particleVertex);
    VkShaderModule skyVertex;
    CreateShader(appState, "shaders/sky.vert.spv", &skyVertex);
    VkShaderModule skyFragment;
    CreateShader(appState, "shaders/sky.frag.spv", &skyFragment);
    VkShaderModule skyRGBAFragment;
    CreateShader(appState, "shaders/sky_rgba.frag.spv", &skyRGBAFragment);
    VkShaderModule coloredVertex;
    CreateShader(appState, "shaders/colored.vert.spv", &coloredVertex);
    VkShaderModule coloredFragment;
    CreateShader(appState, "shaders/colored.frag.spv", &coloredFragment);
    VkShaderModule cutoutVertex;
    CreateShader(appState, "shaders/cutout.vert.spv", &cutoutVertex);
    VkShaderModule cutoutFragment;
    CreateShader(appState, "shaders/cutout.frag.spv", &cutoutFragment);
    VkShaderModule texturedVertex;
    CreateShader(appState, "shaders/textured.vert.spv", &texturedVertex);
    VkShaderModule texturedFragment;
    CreateShader(appState, "shaders/textured.frag.spv", &texturedFragment);

    VkDescriptorSetLayoutBinding descriptorSetBindings[3] { };
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    descriptorSetLayoutCreateInfo.pBindings = descriptorSetBindings;
    descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorSetBindings[0].descriptorCount = 1;
    descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descriptorSetLayoutCreateInfo.bindingCount = 1;
    CHECK_VKCMD(vkCreateDescriptorSetLayout(appState.Device, &descriptorSetLayoutCreateInfo, nullptr, &singleStorageBufferLayout));
	descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	CHECK_VKCMD(vkCreateDescriptorSetLayout(appState.Device, &descriptorSetLayoutCreateInfo, nullptr, &singleFragmentStorageBufferLayout));
    descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
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

	VkViewport viewport { };
	viewport.x = (float)appState.SwapchainRect.offset.x;
	viewport.y = (float)appState.SwapchainRect.offset.y;
	viewport.width = (float)appState.SwapchainRect.extent.width;
	viewport.height = (float)appState.SwapchainRect.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &appState.SwapchainRect;

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
    graphicsPipelineCreateInfo.renderPass = appState.RenderPass;
    graphicsPipelineCreateInfo.stageCount = stages.size();
    graphicsPipelineCreateInfo.pStages = stages.data();

    VkPipelineInputAssemblyStateCreateInfo triangles { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    triangles.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineInputAssemblyStateCreateInfo triangleStrip { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    triangleStrip.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    PipelineAttributes surfaceAttributes { };
    surfaceAttributes.vertexAttributes.resize(1);
    surfaceAttributes.vertexBindings.resize(1);
    surfaceAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    surfaceAttributes.vertexBindings[0].stride = 4 * sizeof(float);
    surfaceAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    surfaceAttributes.vertexInputState.vertexBindingDescriptionCount = surfaceAttributes.vertexBindings.size();
    surfaceAttributes.vertexInputState.pVertexBindingDescriptions = surfaceAttributes.vertexBindings.data();
    surfaceAttributes.vertexInputState.vertexAttributeDescriptionCount = surfaceAttributes.vertexAttributes.size();
    surfaceAttributes.vertexInputState.pVertexAttributeDescriptions = surfaceAttributes.vertexAttributes.data();

	PipelineAttributes spriteAttributes { };
	spriteAttributes.vertexAttributes.resize(2);
	spriteAttributes.vertexBindings.resize(1);
	spriteAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	spriteAttributes.vertexAttributes[1].location = 1;
	spriteAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	spriteAttributes.vertexAttributes[1].offset = 3 * sizeof(float);
	spriteAttributes.vertexBindings[0].stride = 5 * sizeof(float);
	spriteAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	spriteAttributes.vertexInputState.vertexBindingDescriptionCount = spriteAttributes.vertexBindings.size();
	spriteAttributes.vertexInputState.pVertexBindingDescriptions = spriteAttributes.vertexBindings.data();
	spriteAttributes.vertexInputState.vertexAttributeDescriptionCount = spriteAttributes.vertexAttributes.size();
	spriteAttributes.vertexInputState.pVertexAttributeDescriptions = spriteAttributes.vertexAttributes.data();

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
    aliasAttributes.vertexAttributes[0].format = VK_FORMAT_R8G8B8_UINT;
    aliasAttributes.vertexBindings[0].stride = 3 * sizeof(byte);
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

    PipelineAttributes aliasAlphaAttributes { };
	aliasAlphaAttributes.vertexAttributes.resize(3);
	aliasAlphaAttributes.vertexBindings.resize(3);
	aliasAlphaAttributes.vertexAttributes[0].format = VK_FORMAT_R8G8B8_UINT;
	aliasAlphaAttributes.vertexBindings[0].stride = 3 * sizeof(byte);
	aliasAlphaAttributes.vertexAttributes[1].location = 1;
	aliasAlphaAttributes.vertexAttributes[1].binding = 1;
	aliasAlphaAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	aliasAlphaAttributes.vertexBindings[1].binding = 1;
	aliasAlphaAttributes.vertexBindings[1].stride = 2 * sizeof(float);
	aliasAlphaAttributes.vertexAttributes[2].location = 2;
	aliasAlphaAttributes.vertexAttributes[2].binding = 2;
	aliasAlphaAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32_SFLOAT;
	aliasAlphaAttributes.vertexBindings[2].binding = 2;
	aliasAlphaAttributes.vertexBindings[2].stride = 2 * sizeof(float);
	aliasAlphaAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	aliasAlphaAttributes.vertexInputState.vertexBindingDescriptionCount = aliasAlphaAttributes.vertexBindings.size();
	aliasAlphaAttributes.vertexInputState.pVertexBindingDescriptions = aliasAlphaAttributes.vertexBindings.data();
	aliasAlphaAttributes.vertexInputState.vertexAttributeDescriptionCount = aliasAlphaAttributes.vertexAttributes.size();
	aliasAlphaAttributes.vertexInputState.pVertexAttributeDescriptions = aliasAlphaAttributes.vertexAttributes.data();

    PipelineAttributes aliasColoredLightsAttributes { };
	aliasColoredLightsAttributes.vertexAttributes.resize(3);
	aliasColoredLightsAttributes.vertexBindings.resize(3);
	aliasColoredLightsAttributes.vertexAttributes[0].format = VK_FORMAT_R8G8B8_UINT;
	aliasColoredLightsAttributes.vertexBindings[0].stride = 3 * sizeof(byte);
	aliasColoredLightsAttributes.vertexAttributes[1].location = 1;
	aliasColoredLightsAttributes.vertexAttributes[1].binding = 1;
	aliasColoredLightsAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	aliasColoredLightsAttributes.vertexBindings[1].binding = 1;
	aliasColoredLightsAttributes.vertexBindings[1].stride = 2 * sizeof(float);
	aliasColoredLightsAttributes.vertexAttributes[2].location = 2;
	aliasColoredLightsAttributes.vertexAttributes[2].binding = 2;
	aliasColoredLightsAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	aliasColoredLightsAttributes.vertexBindings[2].binding = 2;
	aliasColoredLightsAttributes.vertexBindings[2].stride = 3 * sizeof(float);
	aliasColoredLightsAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	aliasColoredLightsAttributes.vertexInputState.vertexBindingDescriptionCount = aliasColoredLightsAttributes.vertexBindings.size();
	aliasColoredLightsAttributes.vertexInputState.pVertexBindingDescriptions = aliasColoredLightsAttributes.vertexBindings.data();
	aliasColoredLightsAttributes.vertexInputState.vertexAttributeDescriptionCount = aliasColoredLightsAttributes.vertexAttributes.size();
	aliasColoredLightsAttributes.vertexInputState.pVertexAttributeDescriptions = aliasColoredLightsAttributes.vertexAttributes.data();

    PipelineAttributes aliasAlphaColoredLightsAttributes { };
	aliasAlphaColoredLightsAttributes.vertexAttributes.resize(3);
	aliasAlphaColoredLightsAttributes.vertexBindings.resize(3);
	aliasAlphaColoredLightsAttributes.vertexAttributes[0].format = VK_FORMAT_R8G8B8_UINT;
	aliasAlphaColoredLightsAttributes.vertexBindings[0].stride = 3 * sizeof(byte);
	aliasAlphaColoredLightsAttributes.vertexAttributes[1].location = 1;
	aliasAlphaColoredLightsAttributes.vertexAttributes[1].binding = 1;
	aliasAlphaColoredLightsAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
	aliasAlphaColoredLightsAttributes.vertexBindings[1].binding = 1;
	aliasAlphaColoredLightsAttributes.vertexBindings[1].stride = 2 * sizeof(float);
	aliasAlphaColoredLightsAttributes.vertexAttributes[2].location = 2;
	aliasAlphaColoredLightsAttributes.vertexAttributes[2].binding = 2;
	aliasAlphaColoredLightsAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	aliasAlphaColoredLightsAttributes.vertexBindings[2].binding = 2;
	aliasAlphaColoredLightsAttributes.vertexBindings[2].stride = 4 * sizeof(float);
	aliasAlphaColoredLightsAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	aliasAlphaColoredLightsAttributes.vertexInputState.vertexBindingDescriptionCount = aliasAlphaColoredLightsAttributes.vertexBindings.size();
	aliasAlphaColoredLightsAttributes.vertexInputState.pVertexBindingDescriptions = aliasAlphaColoredLightsAttributes.vertexBindings.data();
	aliasAlphaColoredLightsAttributes.vertexInputState.vertexAttributeDescriptionCount = aliasAlphaColoredLightsAttributes.vertexAttributes.size();
	aliasAlphaColoredLightsAttributes.vertexInputState.pVertexAttributeDescriptions = aliasAlphaColoredLightsAttributes.vertexAttributes.data();

    PipelineAttributes particleAttributes { };
    particleAttributes.vertexAttributes.resize(1);
    particleAttributes.vertexBindings.resize(1);
    particleAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    particleAttributes.vertexBindings[0].stride = 4 * sizeof(float);
    particleAttributes.vertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
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

    PipelineAttributes cutoutAttributes { };
    cutoutAttributes.vertexAttributes.resize(1);
    cutoutAttributes.vertexBindings.resize(1);
    cutoutAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    cutoutAttributes.vertexBindings[0].stride = 3 * sizeof(float);
    cutoutAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    cutoutAttributes.vertexInputState.vertexBindingDescriptionCount = cutoutAttributes.vertexBindings.size();
    cutoutAttributes.vertexInputState.pVertexBindingDescriptions = cutoutAttributes.vertexBindings.data();
    cutoutAttributes.vertexInputState.vertexAttributeDescriptionCount = cutoutAttributes.vertexAttributes.size();
    cutoutAttributes.vertexInputState.pVertexAttributeDescriptions = cutoutAttributes.vertexAttributes.data();

    descriptorSetLayouts.resize(5);

    VkPushConstantRange pushConstantInfo { };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

    descriptorSetLayouts[0] = twoBuffersAndImageLayout;
    descriptorSetLayouts[1] = singleStorageBufferLayout;
    descriptorSetLayouts[2] = singleImageLayout;
    descriptorSetLayouts[3] = singleFragmentStorageBufferLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 4;
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfaces.pipelineLayout));
    graphicsPipelineCreateInfo.layout = surfaces.pipelineLayout;
    graphicsPipelineCreateInfo.pVertexInputState = &surfaceAttributes.vertexInputState;
    graphicsPipelineCreateInfo.pInputAssemblyState = &triangles;
    stages[0].module = surfaceVertex;
    stages[1].module = surfaceFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfaces.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	VkDebugUtilsObjectNameInfoEXT pipelineName { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
	pipelineName.objectType = VK_OBJECT_TYPE_PIPELINE;
	pipelineName.objectHandle = (uint64_t)surfaces.pipeline;
	pipelineName.pObjectName = SURFACES_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = doubleBufferLayout;
    pushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantInfo.size = 5 * sizeof(float);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = surfacesColoredLights.pipelineLayout;
    stages[1].module = surfaceColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)surfacesColoredLights.pipeline;
	pipelineName.pObjectName = SURFACES_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = singleBufferLayout;
	descriptorSetLayouts[2] = singleImageLayout;
	descriptorSetLayouts[3] = singleImageLayout;
    descriptorSetLayouts[4] = singleFragmentStorageBufferLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 5;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRGBA.pipelineLayout));
    graphicsPipelineCreateInfo.layout = surfacesRGBA.pipelineLayout;
    stages[0].module = surfaceRGBAVertex;
    stages[1].module = surfaceRGBAFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRGBA.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)surfacesRGBA.pipeline;
	pipelineName.pObjectName = SURFACES_RGBA_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRGBAColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = surfacesRGBAColoredLights.pipelineLayout;
    stages[1].module = surfaceRGBAColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRGBAColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)surfacesRGBAColoredLights.pipeline;
	pipelineName.pObjectName = SURFACES_RGBA_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

	descriptorSetLayouts[3] = singleFragmentStorageBufferLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 4;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRGBANoGlow.pipelineLayout));
    graphicsPipelineCreateInfo.layout = surfacesRGBANoGlow.pipelineLayout;
    stages[0].module = surfaceVertex;
    stages[1].module = surfaceRGBANoGlowFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRGBANoGlow.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)surfacesRGBANoGlow.pipeline;
	pipelineName.pObjectName = SURFACES_RGBA_NO_GLOW_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRGBANoGlowColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = surfacesRGBANoGlowColoredLights.pipelineLayout;
    stages[1].module = surfaceRGBANoGlowColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRGBANoGlowColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)surfacesRGBANoGlowColoredLights.pipeline;
	pipelineName.pObjectName = SURFACES_RGBA_NO_GLOW_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = twoBuffersAndImageLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotated.pipelineLayout));
    graphicsPipelineCreateInfo.layout = surfacesRotated.pipelineLayout;
    stages[0].module = surfaceRotatedVertex;
    stages[1].module = surfaceRotatedFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotated.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)surfacesRotated.pipeline;
	pipelineName.pObjectName = SURFACES_ROTATED_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = doubleBufferLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotatedColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = surfacesRotatedColoredLights.pipelineLayout;
    stages[1].module = surfaceRotatedColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotatedColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)surfacesRotatedColoredLights.pipeline;
	pipelineName.pObjectName = SURFACES_ROTATED_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = singleBufferLayout;
	descriptorSetLayouts[3] = singleImageLayout;
	descriptorSetLayouts[4] = singleFragmentStorageBufferLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 5;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotatedRGBA.pipelineLayout));
    graphicsPipelineCreateInfo.layout = surfacesRotatedRGBA.pipelineLayout;
    stages[0].module = surfaceRotatedRGBAVertex;
    stages[1].module = surfaceRotatedRGBAFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotatedRGBA.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)surfacesRotatedRGBA.pipeline;
	pipelineName.pObjectName = SURFACES_ROTATED_RGBA_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotatedRGBAColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = surfacesRotatedRGBAColoredLights.pipelineLayout;
    stages[1].module = surfaceRotatedRGBAColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotatedRGBAColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)surfacesRotatedRGBAColoredLights.pipeline;
	pipelineName.pObjectName = SURFACES_ROTATED_RGBA_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

	descriptorSetLayouts[3] = singleFragmentStorageBufferLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 4;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotatedRGBANoGlow.pipelineLayout));
    graphicsPipelineCreateInfo.layout = surfacesRotatedRGBANoGlow.pipelineLayout;
    stages[0].module = surfaceRotatedVertex;
    stages[1].module = surfaceRotatedRGBANoGlowFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotatedRGBANoGlow.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)surfacesRotatedRGBANoGlow.pipeline;
	pipelineName.pObjectName = SURFACES_ROTATED_RGBA_NO_GLOW_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &surfacesRotatedRGBANoGlowColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = surfacesRotatedRGBANoGlowColoredLights.pipelineLayout;
    stages[1].module = surfaceRotatedRGBANoGlowColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &surfacesRotatedRGBANoGlowColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)surfacesRotatedRGBANoGlowColoredLights.pipeline;
	pipelineName.pObjectName = SURFACES_ROTATED_RGBA_NO_GLOW_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = twoBuffersAndImageLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fences.pipelineLayout));
    graphicsPipelineCreateInfo.layout = fences.pipelineLayout;
    stages[0].module = surfaceVertex;
    stages[1].module = fenceFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fences.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)fences.pipeline;
	pipelineName.pObjectName = FENCES_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = doubleBufferLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = fencesColoredLights.pipelineLayout;
    stages[1].module = fenceColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)fencesColoredLights.pipeline;
	pipelineName.pObjectName = FENCES_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = singleBufferLayout;
	descriptorSetLayouts[3] = singleImageLayout;
	descriptorSetLayouts[4] = singleFragmentStorageBufferLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 5;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRGBA.pipelineLayout));
    graphicsPipelineCreateInfo.layout = fencesRGBA.pipelineLayout;
    stages[0].module = surfaceRGBAVertex;
    stages[1].module = fenceRGBAFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRGBA.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)fencesRGBA.pipeline;
	pipelineName.pObjectName = FENCES_RGBA_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRGBAColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = fencesRGBAColoredLights.pipelineLayout;
    stages[1].module = fenceRGBAColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRGBAColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)fencesRGBAColoredLights.pipeline;
	pipelineName.pObjectName = FENCES_RGBA_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

	descriptorSetLayouts[3] = singleFragmentStorageBufferLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 4;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRGBANoGlow.pipelineLayout));
    graphicsPipelineCreateInfo.layout = fencesRGBANoGlow.pipelineLayout;
    stages[0].module = surfaceVertex;
    stages[1].module = fenceRGBANoGlowFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRGBANoGlow.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)fencesRGBANoGlow.pipeline;
	pipelineName.pObjectName = FENCES_RGBA_NO_GLOW_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRGBANoGlowColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = fencesRGBANoGlowColoredLights.pipelineLayout;
    stages[1].module = fenceRGBANoGlowColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRGBANoGlowColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)fencesRGBANoGlowColoredLights.pipeline;
	pipelineName.pObjectName = FENCES_RGBA_NO_GLOW_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = twoBuffersAndImageLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotated.pipelineLayout));
    graphicsPipelineCreateInfo.layout = fencesRotated.pipelineLayout;
    stages[0].module = surfaceRotatedVertex;
    stages[1].module = fenceRotatedFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotated.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)fencesRotated.pipeline;
	pipelineName.pObjectName = FENCES_ROTATED_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = doubleBufferLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotatedColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = fencesRotatedColoredLights.pipelineLayout;
    stages[1].module = fenceRotatedColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotatedColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)fencesRotatedColoredLights.pipeline;
	pipelineName.pObjectName = FENCES_ROTATED_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = singleBufferLayout;
	descriptorSetLayouts[3] = singleImageLayout;
	descriptorSetLayouts[4] = singleFragmentStorageBufferLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 5;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotatedRGBA.pipelineLayout));
    graphicsPipelineCreateInfo.layout = fencesRotatedRGBA.pipelineLayout;
    stages[0].module = surfaceRotatedRGBAVertex;
    stages[1].module = fenceRotatedRGBAFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotatedRGBA.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)fencesRotatedRGBA.pipeline;
	pipelineName.pObjectName = FENCES_ROTATED_RGBA_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotatedRGBAColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = fencesRotatedRGBAColoredLights.pipelineLayout;
    stages[1].module = fenceRotatedRGBAColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotatedRGBAColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)fencesRotatedRGBAColoredLights.pipeline;
	pipelineName.pObjectName = FENCES_ROTATED_RGBA_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

	descriptorSetLayouts[3] = singleFragmentStorageBufferLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 4;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotatedRGBANoGlow.pipelineLayout));
    graphicsPipelineCreateInfo.layout = fencesRotatedRGBANoGlow.pipelineLayout;
    stages[0].module = surfaceRotatedVertex;
    stages[1].module = fenceRotatedRGBANoGlowFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotatedRGBANoGlow.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)fencesRotatedRGBANoGlow.pipeline;
	pipelineName.pObjectName = FENCES_ROTATED_RGBA_NO_GLOW_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &fencesRotatedRGBANoGlowColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = fencesRotatedRGBANoGlowColoredLights.pipelineLayout;
    stages[1].module = fenceRotatedRGBANoGlowColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &fencesRotatedRGBANoGlowColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)fencesRotatedRGBANoGlowColoredLights.pipeline;
	pipelineName.pObjectName = FENCES_ROTATED_RGBA_NO_GLOW_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = doubleBufferLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 3;
    pushConstantInfo.size = sizeof(float);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulent.pipelineLayout));
    graphicsPipelineCreateInfo.layout = turbulent.pipelineLayout;
    stages[0].module = turbulentVertex;
    stages[1].module = turbulentFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulent.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)turbulent.pipeline;
	pipelineName.pObjectName = TURBULENT_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = singleBufferLayout;
    pushConstantInfo.size = 6 * sizeof(float);
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRGBA.pipelineLayout));
    graphicsPipelineCreateInfo.layout = turbulentRGBA.pipelineLayout;
    stages[0].module = turbulentVertex;
    stages[1].module = turbulentRGBAFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRGBA.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)turbulentRGBA.pipeline;
	pipelineName.pObjectName = TURBULENT_RGBA_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = twoBuffersAndImageLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 4;
    pushConstantInfo.size = sizeof(float);
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentLit.pipelineLayout));
    graphicsPipelineCreateInfo.layout = turbulentLit.pipelineLayout;
    stages[0].module = surfaceVertex;
    stages[1].module = turbulentLitFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentLit.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)turbulentLit.pipeline;
	pipelineName.pObjectName = TURBULENT_LIT_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = doubleBufferLayout;
    pushConstantInfo.size = 6 * sizeof(float);
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = turbulentColoredLights.pipelineLayout;
    stages[1].module = turbulentColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)turbulentColoredLights.pipeline;
	pipelineName.pObjectName = TURBULENT_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = singleBufferLayout;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRGBALit.pipelineLayout));
    graphicsPipelineCreateInfo.layout = turbulentRGBALit.pipelineLayout;
    stages[1].module = turbulentRGBALitFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRGBALit.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)turbulentRGBALit.pipeline;
	pipelineName.pObjectName = TURBULENT_RGBA_LIT_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

	descriptorSetLayouts[3] = singleFragmentStorageBufferLayout;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRGBAColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = turbulentRGBAColoredLights.pipelineLayout;
    stages[1].module = turbulentRGBAColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRGBAColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)turbulentRGBAColoredLights.pipeline;
	pipelineName.pObjectName = TURBULENT_RGBA_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = doubleBufferLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 3;
    pushConstantInfo.size = sizeof(float);
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotated.pipelineLayout));
    graphicsPipelineCreateInfo.layout = turbulentRotated.pipelineLayout;
    stages[0].module = turbulentRotatedVertex;
    stages[1].module = turbulentRotatedFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotated.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)turbulentRotated.pipeline;
	pipelineName.pObjectName = TURBULENT_ROTATED_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = singleBufferLayout;
    pushConstantInfo.size = 6 * sizeof(float);
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotatedRGBA.pipelineLayout));
    graphicsPipelineCreateInfo.layout = turbulentRotatedRGBA.pipelineLayout;
    stages[1].module = turbulentRotatedRGBAFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotatedRGBA.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)turbulentRotatedRGBA.pipeline;
	pipelineName.pObjectName = TURBULENT_ROTATED_RGBA_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = twoBuffersAndImageLayout;
    pipelineLayoutCreateInfo.setLayoutCount = 4;
    pushConstantInfo.size = sizeof(float);
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotatedLit.pipelineLayout));
    graphicsPipelineCreateInfo.layout = turbulentRotatedLit.pipelineLayout;
    stages[0].module = surfaceRotatedVertex;
    stages[1].module = turbulentRotatedLitFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotatedLit.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)turbulentRotatedLit.pipeline;
	pipelineName.pObjectName = TURBULENT_ROTATED_LIT_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = doubleBufferLayout;
    pushConstantInfo.size = 6 * sizeof(float);
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotatedColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = turbulentRotatedColoredLights.pipelineLayout;
    stages[1].module = turbulentRotatedColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotatedColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)turbulentRotatedColoredLights.pipeline;
	pipelineName.pObjectName = TURBULENT_ROTATED_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = singleBufferLayout;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotatedRGBALit.pipelineLayout));
    graphicsPipelineCreateInfo.layout = turbulentRotatedRGBALit.pipelineLayout;
    stages[0].module = surfaceRotatedVertex;
    stages[1].module = turbulentRotatedRGBALitFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotatedRGBALit.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)turbulentRotatedRGBALit.pipeline;
	pipelineName.pObjectName = TURBULENT_ROTATED_RGBA_LIT_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &turbulentRotatedRGBAColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = turbulentRotatedRGBAColoredLights.pipelineLayout;
    stages[1].module = turbulentRotatedRGBAColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &turbulentRotatedRGBAColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)turbulentRotatedRGBAColoredLights.pipeline;
	pipelineName.pObjectName = TURBULENT_ROTATED_RGBA_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = doubleBufferLayout;
    descriptorSetLayouts[1] = singleImageLayout;
	pipelineLayoutCreateInfo.setLayoutCount = 2;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &sprites.pipelineLayout));
    graphicsPipelineCreateInfo.layout = sprites.pipelineLayout;
    graphicsPipelineCreateInfo.pVertexInputState = &spriteAttributes.vertexInputState;
    stages[0].module = spriteVertex;
    stages[1].module = spriteFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &sprites.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)sprites.pipeline;
	pipelineName.pObjectName = SPRITES_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    pipelineLayoutCreateInfo.setLayoutCount = 3;
    pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantInfo.size = 16 * sizeof(float);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &alias.pipelineLayout));
    graphicsPipelineCreateInfo.layout = alias.pipelineLayout;
    graphicsPipelineCreateInfo.pVertexInputState = &aliasAttributes.vertexInputState;
    stages[0].module = aliasVertex;
    stages[1].module = aliasFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &alias.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)alias.pipeline;
	pipelineName.pObjectName = ALIAS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &aliasAlpha.pipelineLayout));
    graphicsPipelineCreateInfo.layout = aliasAlpha.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &aliasAlphaAttributes.vertexInputState;
	stages[0].module = aliasAlphaVertex;
    stages[1].module = aliasAlphaFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &aliasAlpha.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)aliasAlpha.pipeline;
	pipelineName.pObjectName = ALIAS_ALPHA_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &aliasHoley.pipelineLayout));
    graphicsPipelineCreateInfo.layout = aliasHoley.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &aliasAttributes.vertexInputState;
	stages[0].module = aliasVertex;
    stages[1].module = aliasHoleyFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &aliasHoley.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)aliasHoley.pipeline;
	pipelineName.pObjectName = ALIAS_HOLEY_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &aliasHoleyAlpha.pipelineLayout));
    graphicsPipelineCreateInfo.layout = aliasHoleyAlpha.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &aliasAlphaAttributes.vertexInputState;
	stages[0].module = aliasAlphaVertex;
    stages[1].module = aliasHoleyAlphaFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &aliasHoleyAlpha.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)aliasHoleyAlpha.pipeline;
	pipelineName.pObjectName = ALIAS_HOLEY_ALPHA_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = 21 * sizeof(float);
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &aliasColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = aliasColoredLights.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &aliasColoredLightsAttributes.vertexInputState;
	stages[0].module = aliasColoredLightsVertex;
    stages[1].module = aliasColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &aliasColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)aliasColoredLights.pipeline;
	pipelineName.pObjectName = ALIAS_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &aliasAlphaColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = aliasAlphaColoredLights.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &aliasAlphaColoredLightsAttributes.vertexInputState;
	stages[0].module = aliasAlphaColoredLightsVertex;
    stages[1].module = aliasAlphaColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &aliasAlphaColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)aliasAlphaColoredLights.pipeline;
	pipelineName.pObjectName = ALIAS_ALPHA_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &aliasHoleyColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = aliasHoleyColoredLights.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &aliasColoredLightsAttributes.vertexInputState;
	stages[0].module = aliasColoredLightsVertex;
    stages[1].module = aliasHoleyColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &aliasHoleyColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)aliasHoleyColoredLights.pipeline;
	pipelineName.pObjectName = ALIAS_HOLEY_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &aliasHoleyAlphaColoredLights.pipelineLayout));
    graphicsPipelineCreateInfo.layout = aliasHoleyAlphaColoredLights.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &aliasAlphaColoredLightsAttributes.vertexInputState;
	stages[0].module = aliasAlphaColoredLightsVertex;
    stages[1].module = aliasHoleyAlphaColoredLightsFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &aliasHoleyAlphaColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)aliasHoleyAlphaColoredLights.pipeline;
	pipelineName.pObjectName = ALIAS_HOLEY_ALPHA_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

	pipelineLayoutCreateInfo.setLayoutCount = 3;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantInfo.size = 16 * sizeof(float);
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &viewmodels.pipelineLayout));
    graphicsPipelineCreateInfo.layout = viewmodels.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &aliasAttributes.vertexInputState;
    stages[0].module = viewmodelVertex;
    stages[1].module = viewmodelFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &viewmodels.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)viewmodels.pipeline;
	pipelineName.pObjectName = VIEWMODELS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &viewmodelsHoley.pipelineLayout));
    graphicsPipelineCreateInfo.layout = viewmodelsHoley.pipelineLayout;
    stages[1].module = viewmodelHoleyFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &viewmodelsHoley.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)viewmodelsHoley.pipeline;
	pipelineName.pObjectName = VIEWMODELS_HOLEY_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

	pipelineLayoutCreateInfo.setLayoutCount = 2;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantInfo.size = 21 * sizeof(float);
	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &viewmodelsColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = viewmodelsColoredLights.pipelineLayout;
	graphicsPipelineCreateInfo.pVertexInputState = &aliasColoredLightsAttributes.vertexInputState;
	stages[0].module = viewmodelColoredLightsVertex;
	stages[1].module = viewmodelColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &viewmodelsColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)viewmodelsColoredLights.pipeline;
	pipelineName.pObjectName = VIEWMODELS_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

	CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &viewmodelsHoleyColoredLights.pipelineLayout));
	graphicsPipelineCreateInfo.layout = viewmodelsHoleyColoredLights.pipelineLayout;
	stages[1].module = viewmodelHoleyColoredLightsFragment;
	CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &viewmodelsHoleyColoredLights.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)viewmodelsHoleyColoredLights.pipeline;
	pipelineName.pObjectName = VIEWMODELS_HOLEY_COLORED_LIGHTS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    pipelineLayoutCreateInfo.setLayoutCount = 1;
	pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantInfo.size = 8 * sizeof(float);
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &particles.pipelineLayout));
    graphicsPipelineCreateInfo.layout = particles.pipelineLayout;
    graphicsPipelineCreateInfo.pVertexInputState = &particleAttributes.vertexInputState;
    stages[0].module = particleVertex;
    stages[1].module = coloredFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &particles.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)particles.pipeline;
	pipelineName.pObjectName = PARTICLES_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &colored.pipelineLayout));
    graphicsPipelineCreateInfo.layout = colored.pipelineLayout;
    graphicsPipelineCreateInfo.pVertexInputState = &coloredAttributes.vertexInputState;
    stages[0].module = coloredVertex;
    stages[1].module = coloredFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &colored.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)colored.pipeline;
	pipelineName.pObjectName = COLORED_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &cutout.pipelineLayout));
	colorBlendAttachmentState.blendEnable = false;
    graphicsPipelineCreateInfo.layout = cutout.pipelineLayout;
    graphicsPipelineCreateInfo.pVertexInputState = &cutoutAttributes.vertexInputState;
    stages[0].module = cutoutVertex;
    stages[1].module = cutoutFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &cutout.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)cutout.pipeline;
	pipelineName.pObjectName = CUTOUT_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    pipelineLayoutCreateInfo.setLayoutCount = 2;
    pushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantInfo.size = 13 * sizeof(float);
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &sky.pipelineLayout));
    depthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
	colorBlendAttachmentState.blendEnable = true;
    graphicsPipelineCreateInfo.layout = sky.pipelineLayout;
    graphicsPipelineCreateInfo.pVertexInputState = &skyAttributes.vertexInputState;
    graphicsPipelineCreateInfo.pInputAssemblyState = &triangleStrip;
    stages[0].module = skyVertex;
    stages[1].module = skyFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &sky.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)sky.pipeline;
	pipelineName.pObjectName = SKY_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    descriptorSetLayouts[0] = singleBufferLayout;
    pushConstantInfo.size = 15 * sizeof(float);
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &skyRGBA.pipelineLayout));
    graphicsPipelineCreateInfo.layout = skyRGBA.pipelineLayout;
    stages[0].module = skyVertex;
    stages[1].module = skyRGBAFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &skyRGBA.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)skyRGBA.pipeline;
	pipelineName.pObjectName = SKY_RGBA_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &controllers.pipelineLayout));
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    graphicsPipelineCreateInfo.layout = controllers.pipelineLayout;
    graphicsPipelineCreateInfo.pVertexInputState = &texturedAttributes.vertexInputState;
    graphicsPipelineCreateInfo.pInputAssemblyState = &triangles;
    stages[0].module = texturedVertex;
    stages[1].module = texturedFragment;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &controllers.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)controllers.pipeline;
	pipelineName.pObjectName = CONTROLLERS_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    CHECK_VKCMD(vkCreatePipelineLayout(appState.Device, &pipelineLayoutCreateInfo, nullptr, &floor.pipelineLayout));
    graphicsPipelineCreateInfo.layout = floor.pipelineLayout;
    graphicsPipelineCreateInfo.pInputAssemblyState = &triangleStrip;
    CHECK_VKCMD(vkCreateGraphicsPipelines(appState.Device, appState.PipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &floor.pipeline));
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	pipelineName.objectHandle = (uint64_t)floor.pipeline;
	pipelineName.pObjectName = FLOOR_NAME;
	CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &pipelineName));
#endif

    vkDestroyShaderModule(appState.Device, texturedFragment, nullptr);
    vkDestroyShaderModule(appState.Device, texturedVertex, nullptr);
    vkDestroyShaderModule(appState.Device, cutoutFragment, nullptr);
    vkDestroyShaderModule(appState.Device, cutoutVertex, nullptr);
    vkDestroyShaderModule(appState.Device, coloredFragment, nullptr);
    vkDestroyShaderModule(appState.Device, coloredVertex, nullptr);
    vkDestroyShaderModule(appState.Device, skyRGBAFragment, nullptr);
    vkDestroyShaderModule(appState.Device, skyFragment, nullptr);
    vkDestroyShaderModule(appState.Device, skyVertex, nullptr);
    vkDestroyShaderModule(appState.Device, particleVertex, nullptr);
    vkDestroyShaderModule(appState.Device, viewmodelHoleyColoredLightsFragment, nullptr);
    vkDestroyShaderModule(appState.Device, viewmodelColoredLightsFragment, nullptr);
    vkDestroyShaderModule(appState.Device, viewmodelColoredLightsVertex, nullptr);
    vkDestroyShaderModule(appState.Device, viewmodelHoleyFragment, nullptr);
    vkDestroyShaderModule(appState.Device, viewmodelFragment, nullptr);
    vkDestroyShaderModule(appState.Device, viewmodelVertex, nullptr);
	vkDestroyShaderModule(appState.Device, aliasHoleyAlphaColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, aliasHoleyAlphaFragment, nullptr);
	vkDestroyShaderModule(appState.Device, aliasAlphaColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, aliasAlphaColoredLightsVertex, nullptr);
	vkDestroyShaderModule(appState.Device, aliasAlphaFragment, nullptr);
	vkDestroyShaderModule(appState.Device, aliasAlphaVertex, nullptr);
    vkDestroyShaderModule(appState.Device, aliasHoleyColoredLightsFragment, nullptr);
    vkDestroyShaderModule(appState.Device, aliasColoredLightsFragment, nullptr);
    vkDestroyShaderModule(appState.Device, aliasColoredLightsVertex, nullptr);
    vkDestroyShaderModule(appState.Device, aliasHoleyFragment, nullptr);
    vkDestroyShaderModule(appState.Device, aliasFragment, nullptr);
    vkDestroyShaderModule(appState.Device, aliasVertex, nullptr);
    vkDestroyShaderModule(appState.Device, spriteFragment, nullptr);
    vkDestroyShaderModule(appState.Device, spriteVertex, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentRotatedRGBAColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentRotatedRGBALitFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentRotatedColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentRotatedLitFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentRotatedRGBAFragment, nullptr);
	vkDestroyShaderModule(appState.Device, turbulentRotatedFragment, nullptr);
    vkDestroyShaderModule(appState.Device, turbulentRotatedVertex, nullptr);
    vkDestroyShaderModule(appState.Device, turbulentRGBAColoredLightsFragment, nullptr);
    vkDestroyShaderModule(appState.Device, turbulentRGBALitFragment, nullptr);
    vkDestroyShaderModule(appState.Device, turbulentColoredLightsFragment, nullptr);
    vkDestroyShaderModule(appState.Device, turbulentLitFragment, nullptr);
    vkDestroyShaderModule(appState.Device, turbulentRGBAFragment, nullptr);
    vkDestroyShaderModule(appState.Device, turbulentFragment, nullptr);
    vkDestroyShaderModule(appState.Device, turbulentVertex, nullptr);
	vkDestroyShaderModule(appState.Device, fenceRotatedRGBANoGlowColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, fenceRotatedRGBANoGlowFragment, nullptr);
	vkDestroyShaderModule(appState.Device, fenceRotatedRGBAColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, fenceRotatedRGBAFragment, nullptr);
	vkDestroyShaderModule(appState.Device, fenceRotatedColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, fenceRotatedFragment, nullptr);
    vkDestroyShaderModule(appState.Device, fenceRGBANoGlowColoredLightsFragment, nullptr);
    vkDestroyShaderModule(appState.Device, fenceRGBANoGlowFragment, nullptr);
    vkDestroyShaderModule(appState.Device, fenceRGBAColoredLightsFragment, nullptr);
    vkDestroyShaderModule(appState.Device, fenceRGBAFragment, nullptr);
    vkDestroyShaderModule(appState.Device, fenceColoredLightsFragment, nullptr);
    vkDestroyShaderModule(appState.Device, fenceFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRotatedRGBANoGlowColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRotatedRGBANoGlowFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRotatedRGBAColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRotatedRGBAFragment, nullptr);
    vkDestroyShaderModule(appState.Device, surfaceRotatedRGBAVertex, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRotatedColoredLightsFragment, nullptr);
	vkDestroyShaderModule(appState.Device, surfaceRotatedFragment, nullptr);
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
	samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
	CHECK_VKCMD(vkCreateSampler(appState.Device, &samplerCreateInfo, nullptr, &sampler));

    auto screenImageCount = appState.Screen.swapchainImages.size();

	VkBufferCreateInfo bufferCreateInfo { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferCreateInfo.size = 256 * 4 * sizeof(float);
	bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    paletteBuffers.resize(screenImageCount);
    for (uint32_t i = 0; i < screenImageCount; i++)
    {
        CHECK_VKCMD(vkCreateBuffer(appState.Device, &bufferCreateInfo, nullptr, &paletteBuffers[i]));
    }
    neutralPaletteBuffers.resize(screenImageCount);
    for (uint32_t i = 0; i < screenImageCount; i++)
    {
        CHECK_VKCMD(vkCreateBuffer(appState.Device, &bufferCreateInfo, nullptr, &neutralPaletteBuffers[i]));
    }
    vkGetBufferMemoryRequirements(appState.Device, paletteBuffers[0], &memoryRequirements);
    paletteBufferSize = memoryRequirements.size;
    memoryAllocateInfo.allocationSize = paletteBufferSize * 2 * screenImageCount;
	auto properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    auto found = false;
    for (auto i = 0; i < appState.MemoryProperties.memoryTypeCount; i++)
    {
        if ((memoryRequirements.memoryTypeBits & (1 << i)) != 0)
        {
            const VkFlags propertyFlags = appState.MemoryProperties.memoryTypes[i].propertyFlags;
            if ((propertyFlags & properties) == properties)
            {
                found = true;
                memoryAllocateInfo.memoryTypeIndex = i;
                break;
            }
        }
    }
    if (!found)
    {
        THROW(Fmt("Scene::Create(): Memory type %d with properties %d not found.", memoryRequirements.memoryTypeBits, properties));
    }
    CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &paletteMemory));
	VkDeviceSize memoryOffset = 0;
    for (uint32_t i = 0; i < screenImageCount; i++)
    {
        CHECK_VKCMD(vkBindBufferMemory(appState.Device, paletteBuffers[i], paletteMemory, memoryOffset));
        memoryOffset += paletteBufferSize;
        CHECK_VKCMD(vkBindBufferMemory(appState.Device, neutralPaletteBuffers[i], paletteMemory, memoryOffset));
        memoryOffset += paletteBufferSize;
    }

    appState.Keyboard.Create(appState);

    created = true;
}

void Scene::CreateShader(AppState& appState, const char* filename, VkShaderModule* shaderModule)
{
    std::vector<unsigned char> buffer;
    appState.FileLoader->Load(filename, buffer);

    if ((buffer.size() % 4) != 0)
    {
        THROW(Fmt("Scene::CreateShader(): %s is not 4-byte aligned.", filename));
    }

    VkShaderModuleCreateInfo moduleCreateInfo { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    moduleCreateInfo.pCode = (uint32_t *)buffer.data();
    moduleCreateInfo.codeSize = buffer.size();
    CHECK_VKCMD(vkCreateShaderModule(appState.Device, &moduleCreateInfo, nullptr, shaderModule));
}

void Scene::Initialize()
{
    sortedVerticesCount = 0;
    sortedVerticesSize = 0;
    sortedAttributesSize = 0;
    sortedIndicesCount = 0;
    paletteSize = 0;
    colormapSize = 0;
	aliasBuffers.Initialize();
    indexBuffers.Initialize();
	lightmapChains.clear();
	lightmapChainTexturesInUse.clear();
	lightmapRGBChains.clear();
	lightmapRGBChainTexturesInUse.clear();
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

void Scene::AddToVertexInputBarriers(VkBuffer buffer, VkAccessFlags flags)
{
    stagingBuffer.lastEndBarrier++;
    if (stagingBuffer.vertexInputEndBarriers.size() <= stagingBuffer.lastEndBarrier)
    {
        stagingBuffer.vertexInputEndBarriers.emplace_back();
    }

    auto& barrier = stagingBuffer.vertexInputEndBarriers[stagingBuffer.lastEndBarrier];
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.buffer = buffer;
    barrier.size = VK_WHOLE_SIZE;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = flags;
}

void Scene::AddToVertexShaderBarriers(VkBuffer buffer, VkAccessFlags flags)
{
    stagingBuffer.lastVertexShaderEndBarrier++;
    if (stagingBuffer.vertexShaderEndBarriers.size() <= stagingBuffer.lastVertexShaderEndBarrier)
    {
        stagingBuffer.vertexShaderEndBarriers.emplace_back();
    }

    auto& barrier = stagingBuffer.vertexShaderEndBarriers[stagingBuffer.lastVertexShaderEndBarrier];
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.buffer = buffer;
    barrier.size = VK_WHOLE_SIZE;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = flags;
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

uint32_t Scene::GetLayerCountFor(int width, int height)
{
	auto slot0 = std::ceil(std::log2(width));
	auto slot1 = std::ceil(std::log2(height));
	auto slot = (uint32_t)std::log2(std::pow(2, slot0) * std::pow(2, slot1));
	uint32_t layerCount;
	if (slot <= 15)
	{
		layerCount = 128;
	}
	else if (slot >= 22)
	{
		layerCount = 1;
	}
	else
	{
		layerCount = (uint32_t)std::pow(2, 22 - slot);
	}
	return layerCount;
}

void Scene::CacheVertices(PerSurfaceData& perSurface, LoadedTurbulent& loaded)
{
	auto face = (msurface_t*)loaded.face;
	loaded.numedges = face->numedges;
	if (perSurface.vertices.empty())
	{
		perSurface.vertices.resize(loaded.numedges * 3);
		auto model = (model_t*)loaded.model;
		auto vertexes = model->vertexes;
        auto e = face->firstedge;
        auto v = 0;
        for (auto i = 0; i < loaded.numedges; i++)
        {
            auto edge = model->surfedges[e];
            unsigned int index;
            if (edge >= 0)
            {
                index = model->edges[edge].v[0];
            }
            else
            {
                index = model->edges[-edge].v[1];
            }
            auto vertex = vertexes[index].position;
			perSurface.vertices[v++] = vertex[0];
			perSurface.vertices[v++] = vertex[1];
			perSurface.vertices[v++] = vertex[2];
            e++;
        }
    }
	loaded.vertices = perSurface.vertices.data();
}

void Scene::AddLightmapToDescriptorWrites(AppState& appState, Lightmap* lightmap)
{
	lightmapDescriptorInfos.push_back({ });
	auto& bufferInfo = lightmapDescriptorInfos.back();
	bufferInfo.range = VK_WHOLE_SIZE;
	bufferInfo.buffer = lightmap->buffer->buffer.buffer;

	lightmapDescriptorWrites.push_back({ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET });
	auto& write = lightmapDescriptorWrites.back();
	write.descriptorCount = 1;
	write.dstSet = lightmap->buffer->descriptorSet;
	// write.pBufferInfo is not written here - it depends on the dynamic contents of lightmapDescriptorInfos...
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
}

void Scene::AddLightmapRGBToDescriptorWrites(AppState& appState, LightmapRGB* lightmap)
{
	lightmapDescriptorInfos.push_back({ });
	auto& bufferInfo = lightmapDescriptorInfos.back();
	bufferInfo.range = VK_WHOLE_SIZE;
	bufferInfo.buffer = lightmap->buffer->buffer.buffer;

	lightmapDescriptorWrites.push_back({ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET });
	auto& write = lightmapDescriptorWrites.back();
	write.descriptorCount = 1;
	write.dstSet = lightmap->buffer->descriptorSet;
	// write.pBufferInfo is not written here - it depends on the dynamic contents of lightmapDescriptorInfos...
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, PerSurfaceData& perSurface, LoadedLightmap& loaded, VkDeviceSize& size)
{
	if (perSurface.lightmap == nullptr)
	{
		perSurface.lightmap = new Lightmap { };
		auto addToWrites = perSurface.lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, perSurface.texture);
		if (addToWrites)
		{
			AddLightmapToDescriptorWrites(appState, perSurface.lightmap);
		}
		perSurface.lightmap->createdFrameCount = surface.created;
		loaded.lightmap = perSurface.lightmap;
		loaded.size = surface.lightmap_size * sizeof(uint32_t);
		size += loaded.size;
		loaded.source = d_lists.lightmap_texels.data() + surface.first_lightmap_texel;
		loaded.next = nullptr;
		auto entry = lightmapChainTexturesInUse.find(perSurface.texture);
		if (entry == lightmapChainTexturesInUse.end())
		{
			lightmapChainTexturesInUse.insert({ perSurface.texture, lightmapChains.size() });
			lightmapChains.emplace_back();
			lightmapChains.back().first = &loaded;
			lightmapChains.back().current = &loaded;
		}
		else
		{
			auto& chain = lightmapChains[entry->second];
			chain.current->next = &loaded;
			chain.current = &loaded;
		}
	}
	else if (perSurface.lightmap->createdFrameCount != surface.created)
	{
		lightmapsToDelete.Dispose(perSurface.lightmap);
		perSurface.lightmap = new Lightmap { };
		auto addToWrites = perSurface.lightmap->Create(appState, surface.lightmap_width, surface.lightmap_height, perSurface.texture);
		if (addToWrites)
		{
			AddLightmapToDescriptorWrites(appState, perSurface.lightmap);
		}
		perSurface.lightmap->createdFrameCount = surface.created;
		loaded.lightmap = perSurface.lightmap;
		loaded.size = surface.lightmap_size * sizeof(uint32_t);
		size += loaded.size;
		loaded.source = d_lists.lightmap_texels.data() + surface.first_lightmap_texel;
		loaded.next = nullptr;
		auto entry = lightmapChainTexturesInUse.find(perSurface.texture);
		if (entry == lightmapChainTexturesInUse.end())
		{
			lightmapChainTexturesInUse.insert({ perSurface.texture, lightmapChains.size() });
			lightmapChains.emplace_back();
			lightmapChains.back().first = &loaded;
			lightmapChains.back().current = &loaded;
		}
		else
		{
			auto& chain = lightmapChains[entry->second];
			chain.current->next = &loaded;
			chain.current = &loaded;
		}
	}
	else
	{
		loaded.lightmap = perSurface.lightmap;
	}
	perSurface.frameCount = frameCount;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, PerSurfaceData& perSurface, LoadedLightmapRGB& loaded, VkDeviceSize& size)
{
	if (perSurface.lightmapRGB == nullptr)
	{
		perSurface.lightmapRGB = new LightmapRGB { };
		auto addToWrites = perSurface.lightmapRGB->Create(appState, surface.lightmap_width, surface.lightmap_height, perSurface.texture);
		if (addToWrites)
		{
			AddLightmapRGBToDescriptorWrites(appState, perSurface.lightmapRGB);
		}
		perSurface.lightmapRGB->createdFrameCount = surface.created;
		loaded.lightmap = perSurface.lightmapRGB;
		loaded.size = surface.lightmap_size * sizeof(uint32_t);
		size += loaded.size;
		loaded.source = d_lists.lightmap_texels.data() + surface.first_lightmap_texel;
		loaded.next = nullptr;
		auto entry = lightmapRGBChainTexturesInUse.find(perSurface.texture);
		if (entry == lightmapRGBChainTexturesInUse.end())
		{
			lightmapRGBChainTexturesInUse.insert({ perSurface.texture, lightmapRGBChains.size() });
			lightmapRGBChains.emplace_back();
			lightmapRGBChains.back().first = &loaded;
			lightmapRGBChains.back().current = &loaded;
		}
		else
		{
			auto& chain = lightmapRGBChains[entry->second];
			chain.current->next = &loaded;
			chain.current = &loaded;
		}
    }
	else if (perSurface.lightmapRGB->createdFrameCount != surface.created)
	{
		lightmapsRGBToDelete.Dispose(perSurface.lightmapRGB);
		perSurface.lightmapRGB = new LightmapRGB { };
		auto addToWrites = perSurface.lightmapRGB->Create(appState, surface.lightmap_width, surface.lightmap_height, perSurface.texture);
		if (addToWrites)
		{
			AddLightmapRGBToDescriptorWrites(appState, perSurface.lightmapRGB);
		}
		perSurface.lightmapRGB->createdFrameCount = surface.created;
		loaded.lightmap = perSurface.lightmapRGB;
		loaded.size = surface.lightmap_size * sizeof(uint32_t);
		size += loaded.size;
		loaded.source = d_lists.lightmap_texels.data() + surface.first_lightmap_texel;
		loaded.next = nullptr;
		auto entry = lightmapRGBChainTexturesInUse.find(perSurface.texture);
		if (entry == lightmapRGBChainTexturesInUse.end())
		{
			lightmapRGBChainTexturesInUse.insert({ perSurface.texture, lightmapRGBChains.size() });
			lightmapRGBChains.emplace_back();
			lightmapRGBChains.back().first = &loaded;
			lightmapRGBChains.back().current = &loaded;
		}
		else
		{
			auto& chain = lightmapRGBChains[entry->second];
			chain.current->next = &loaded;
			chain.current = &loaded;
		}
	}
	else
	{
		loaded.lightmap = perSurface.lightmapRGB;
	}
	perSurface.frameCount = frameCount;
}

void Scene::GetStagingBufferSize(AppState& appState, const dturbulent_t& turbulent, PerSurfaceData& perSurface, LoadedTurbulent& loaded, VkDeviceSize& size)
{
    loaded.face = turbulent.face;
    loaded.model = turbulent.model;
    loaded.count = turbulent.count;
    CacheVertices(perSurface, loaded);
	if (perSurface.textureSource != turbulent.data)
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
            if (cached->textures.empty() || cached->currentIndex >= cached->textures.back()->layerCount)
            {
				auto layerCount = GetLayerCountFor(turbulent.width, turbulent.height);
                auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
                auto texture = new SharedMemoryTexture { };
                texture->Create(appState, turbulent.width, turbulent.height, VK_FORMAT_R8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                cached->MoveToFront(texture);
				perSurface.texture = texture;
                cached->currentIndex = 0;
            }
            else
            {
				perSurface.texture = cached->textures.back();
            }
            cached->Setup(loaded.texture);
			perSurface.textureIndex = cached->currentIndex;
            loaded.texture.size = turbulent.size;
            loaded.texture.allocated = GetAllocatedFor(turbulent.width, turbulent.height);
            size += loaded.texture.allocated;
            loaded.texture.source = turbulent.data;
            loaded.texture.mips = turbulent.mips;
            surfaceTextureCache.insert({ turbulent.data, { perSurface.texture, perSurface.textureIndex } });
            cached->currentIndex++;
        }
        else
        {
			perSurface.texture = entry->second.texture;
			perSurface.textureIndex = entry->second.index;
        }
		perSurface.textureSource = turbulent.data;
    }
	loaded.texture.texture = perSurface.texture;
	loaded.texture.index = perSurface.textureIndex;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dturbulent_t& turbulent, PerSurfaceData& perSurface, LoadedTurbulent& loaded, VkDeviceSize& size)
{
    loaded.face = turbulent.face;
    loaded.model = turbulent.model;
    loaded.count = turbulent.count;
    CacheVertices(perSurface, loaded);
	if (perSurface.textureSource != turbulent.data)
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
            if (cached->textures.empty() || cached->currentIndex >= cached->textures.back()->layerCount)
            {
				auto layerCount = GetLayerCountFor(turbulent.width, turbulent.height);
                auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
                auto texture = new SharedMemoryTexture { };
                texture->Create(appState, turbulent.width, turbulent.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                cached->MoveToFront(texture);
				perSurface.texture = texture;
                cached->currentIndex = 0;
            }
            else
            {
				perSurface.texture = cached->textures.back();
            }
            cached->Setup(loaded.texture);
			perSurface.textureIndex = cached->currentIndex;
            loaded.texture.size = turbulent.size;
            loaded.texture.allocated = GetAllocatedFor(turbulent.width, turbulent.height) * sizeof(unsigned);
            size += loaded.texture.allocated;
            loaded.texture.source = turbulent.data;
            loaded.texture.mips = turbulent.mips;
            surfaceTextureCache.insert({ turbulent.data, { perSurface.texture, perSurface.textureIndex } });
            cached->currentIndex++;
		}
		else
		{
			perSurface.texture = entry->second.texture;
			perSurface.textureIndex = entry->second.index;
		}
		perSurface.textureSource = turbulent.data;
	}
	loaded.texture.texture = perSurface.texture;
	loaded.texture.index = perSurface.textureIndex;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, PerSurfaceData& perSurface, LoadedSurface& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dturbulent_t&)surface, perSurface, loaded, size);
    GetStagingBufferSize(appState, surface, perSurface, loaded.lightmap, size);
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurface_t& surface, PerSurfaceData& perSurface, LoadedSurfaceColoredLights& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dturbulent_t&)surface, perSurface, loaded, size);
    GetStagingBufferSize(appState, surface, perSurface, loaded.lightmap, size);
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacewithglow_t& surface, PerSurfaceData& perSurface, LoadedSurface2Textures& loaded, VkDeviceSize& size)
{
    loaded.face = surface.face;
    loaded.model = surface.model;
    loaded.count = surface.count;
    CacheVertices(perSurface, loaded);
	if (perSurface.textureSource != surface.data)
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
			if (cached->textures.empty() || cached->currentIndex >= cached->textures.back()->layerCount - extra)
			{
				auto layerCount = GetLayerCountFor(surface.width, surface.height);
				auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				perSurface.texture = texture;
				cached->currentIndex = 0;
			}
			else
			{
				perSurface.texture = cached->textures.back();
			}
			cached->Setup(loaded.texture);
			perSurface.textureIndex = cached->currentIndex;
			loaded.texture.size = surface.size;
			loaded.texture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
			size += loaded.texture.allocated;
			loaded.texture.source = surface.data;
			loaded.texture.mips = surface.mips;
			surfaceTextureCache.insert({ surface.data, { perSurface.texture, perSurface.textureIndex } });
			cached->currentIndex++;
		}
		else
		{
			perSurface.texture = entry->second.texture;
			perSurface.textureIndex = entry->second.index;
		}
		perSurface.textureSource = surface.data;
	}
	loaded.texture.texture = perSurface.texture;
	loaded.texture.index = perSurface.textureIndex;
	if (perSurface.glowTextureSource != surface.glow_data)
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
			if (cached->textures.empty() || cached->currentIndex >= cached->textures.back()->layerCount)
			{
				auto layerCount = GetLayerCountFor(surface.width, surface.height);
				auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				perSurface.glowTexture = texture;
				cached->currentIndex = 0;
			}
			else
			{
				perSurface.glowTexture = cached->textures.back();
			}
			cached->Setup(loaded.glowTexture);
			perSurface.glowTextureIndex = cached->currentIndex;
			loaded.glowTexture.size = surface.size;
			loaded.glowTexture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
			size += loaded.glowTexture.allocated;
			loaded.glowTexture.source = surface.glow_data;
			loaded.glowTexture.mips = surface.mips;
			surfaceTextureCache.insert({ surface.glow_data, { perSurface.glowTexture, perSurface.glowTextureIndex } });
			cached->currentIndex++;
		}
		else
		{
			perSurface.glowTexture = entry->second.texture;
			perSurface.glowTextureIndex = entry->second.index;
		}
		perSurface.glowTextureSource = surface.glow_data;
	}
	loaded.glowTexture.texture = perSurface.glowTexture;
	loaded.glowTexture.index = perSurface.glowTextureIndex;
    GetStagingBufferSize(appState, surface, perSurface, loaded.lightmap, size);
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacewithglow_t& surface, PerSurfaceData& perSurface, LoadedSurface2TexturesColoredLights& loaded, VkDeviceSize& size)
{
	loaded.face = surface.face;
	loaded.model = surface.model;
	loaded.count = surface.count;
	CacheVertices(perSurface, loaded);
	if (perSurface.textureSource != surface.data)
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
			if (cached->textures.empty() || cached->currentIndex >= cached->textures.back()->layerCount - extra)
			{
				auto layerCount = GetLayerCountFor(surface.width, surface.height);
				auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				perSurface.texture = texture;
				cached->currentIndex = 0;
			}
			else
			{
				perSurface.texture = cached->textures.back();
            }
			cached->Setup(loaded.texture);
			perSurface.textureIndex = cached->currentIndex;
			loaded.texture.size = surface.size;
			loaded.texture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
			size += loaded.texture.allocated;
			loaded.texture.source = surface.data;
			loaded.texture.mips = surface.mips;
			surfaceTextureCache.insert({ surface.data, { perSurface.texture, perSurface.textureIndex } });
			cached->currentIndex++;
		}
		else
		{
			perSurface.texture = entry->second.texture;
			perSurface.textureIndex = entry->second.index;
		}
		perSurface.textureSource = surface.data;
	}
	loaded.texture.texture = perSurface.texture;
	loaded.texture.index = perSurface.textureIndex;
	if (perSurface.glowTextureSource != surface.glow_data)
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
			if (cached->textures.empty() || cached->currentIndex >= cached->textures.back()->layerCount)
			{
				auto layerCount = GetLayerCountFor(surface.width, surface.height);
				auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				perSurface.glowTexture = texture;
				cached->currentIndex = 0;
			}
			else
			{
				perSurface.glowTexture = cached->textures.back();
			}
			cached->Setup(loaded.glowTexture);
			perSurface.glowTextureIndex = cached->currentIndex;
			loaded.glowTexture.size = surface.size;
			loaded.glowTexture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
			size += loaded.glowTexture.allocated;
			loaded.glowTexture.source = surface.glow_data;
			loaded.glowTexture.mips = surface.mips;
			surfaceTextureCache.insert({ surface.glow_data, { perSurface.glowTexture, perSurface.glowTextureIndex } });
			cached->currentIndex++;
		}
		else
		{
			perSurface.glowTexture = entry->second.texture;
			perSurface.glowTextureIndex = entry->second.index;
		}
		perSurface.glowTextureSource = surface.glow_data;
	}
	loaded.glowTexture.texture = perSurface.glowTexture;
	loaded.glowTexture.index = perSurface.glowTextureIndex;
    GetStagingBufferSize(appState, surface, perSurface, loaded.lightmap, size);
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurface_t& surface, PerSurfaceData& perSurface, LoadedSurface& loaded, VkDeviceSize& size)
{
	loaded.face = surface.face;
	loaded.model = surface.model;
	loaded.count = surface.count;
	CacheVertices(perSurface, loaded);
	if (perSurface.textureSource != surface.data)
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
			if (cached->textures.empty() || cached->currentIndex >= cached->textures.back()->layerCount)
			{
				auto layerCount = GetLayerCountFor(surface.width, surface.height);
				auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				perSurface.texture = texture;
				cached->currentIndex = 0;
            }
			else
			{
				perSurface.texture = cached->textures.back();
			}
			cached->Setup(loaded.texture);
			perSurface.textureIndex = cached->currentIndex;
			loaded.texture.size = surface.size;
			loaded.texture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
			size += loaded.texture.allocated;
			loaded.texture.source = surface.data;
			loaded.texture.mips = surface.mips;
			surfaceTextureCache.insert({ surface.data, { perSurface.texture, perSurface.textureIndex } });
			cached->currentIndex++;
		}
		else
		{
			perSurface.texture = entry->second.texture;
			perSurface.textureIndex = entry->second.index;
		}
		perSurface.textureSource = surface.data;
	}
	loaded.texture.texture = perSurface.texture;
	loaded.texture.index = perSurface.textureIndex;
    GetStagingBufferSize(appState, surface, perSurface, loaded.lightmap, size);
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurface_t& surface, PerSurfaceData& perSurface, LoadedSurfaceColoredLights& loaded, VkDeviceSize& size)
{
	loaded.face = surface.face;
	loaded.model = surface.model;
	loaded.count = surface.count;
	CacheVertices(perSurface, loaded);
	if (perSurface.textureSource != surface.data)
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
			if (cached->textures.empty() || cached->currentIndex >= cached->textures.back()->layerCount)
			{
				auto layerCount = GetLayerCountFor(surface.width, surface.height);
				auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
				auto texture = new SharedMemoryTexture { };
				texture->Create(appState, surface.width, surface.height, VK_FORMAT_R8G8B8A8_UINT, mipCount, layerCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				cached->MoveToFront(texture);
				perSurface.texture = texture;
				cached->currentIndex = 0;
			}
			else
			{
				perSurface.texture = cached->textures.back();
			}
			cached->Setup(loaded.texture);
			perSurface.textureIndex = cached->currentIndex;
			loaded.texture.size = surface.size;
			loaded.texture.allocated = GetAllocatedFor(surface.width, surface.height) * sizeof(unsigned);
			size += loaded.texture.allocated;
			loaded.texture.source = surface.data;
			loaded.texture.mips = surface.mips;
			surfaceTextureCache.insert({ surface.data, { perSurface.texture, perSurface.textureIndex } });
			cached->currentIndex++;
		}
		else
		{
			perSurface.texture = entry->second.texture;
			perSurface.textureIndex = entry->second.index;
		}
		perSurface.textureSource = surface.data;
	}
	loaded.texture.texture = perSurface.texture;
	loaded.texture.index = perSurface.textureIndex;
	GetStagingBufferSize(appState, surface, perSurface, loaded.lightmap, size);
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, PerSurfaceData& perSurface, LoadedSurfaceRotated& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dsurface_t&)surface, perSurface, loaded, size);
    loaded.originX = surface.origin_x;
    loaded.originY = surface.origin_y;
    loaded.originZ = surface.origin_z;
    loaded.yaw = surface.yaw;
    loaded.pitch = surface.pitch;
    loaded.roll = surface.roll;
	loaded.alpha = surface.alpha;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacerotated_t& surface, PerSurfaceData& perSurface, LoadedSurfaceRotatedColoredLights& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dsurface_t&)surface, perSurface, (LoadedSurfaceColoredLights&)loaded, size);
    loaded.originX = surface.origin_x;
    loaded.originY = surface.origin_y;
    loaded.originZ = surface.origin_z;
    loaded.yaw = surface.yaw;
    loaded.pitch = surface.pitch;
    loaded.roll = surface.roll;
	loaded.alpha = surface.alpha;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacerotatedwithglow_t& surface, PerSurfaceData& perSurface, LoadedSurfaceRotated2Textures& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dsurfacewithglow_t&)surface, perSurface, loaded, size);
    loaded.originX = surface.origin_x;
    loaded.originY = surface.origin_y;
    loaded.originZ = surface.origin_z;
    loaded.yaw = surface.yaw;
    loaded.pitch = surface.pitch;
    loaded.roll = surface.roll;
	loaded.alpha = surface.alpha;
}

void Scene::GetStagingBufferSize(AppState& appState, const dsurfacerotatedwithglow_t& surface, PerSurfaceData& perSurface, LoadedSurfaceRotated2TexturesColoredLights& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dsurfacewithglow_t&)surface, perSurface, loaded, size);
    loaded.originX = surface.origin_x;
    loaded.originY = surface.origin_y;
    loaded.originZ = surface.origin_z;
    loaded.yaw = surface.yaw;
    loaded.pitch = surface.pitch;
    loaded.roll = surface.roll;
	loaded.alpha = surface.alpha;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurfacerotated_t& surface, PerSurfaceData& perSurface, LoadedSurfaceRotated& loaded, VkDeviceSize& size)
{
    GetStagingBufferSizeRGBANoGlow(appState, (const dsurface_t&)surface, perSurface, loaded, size);
    loaded.originX = surface.origin_x;
    loaded.originY = surface.origin_y;
    loaded.originZ = surface.origin_z;
    loaded.yaw = surface.yaw;
    loaded.pitch = surface.pitch;
    loaded.roll = surface.roll;
	loaded.alpha = surface.alpha;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dsurfacerotated_t& surface, PerSurfaceData& perSurface, LoadedSurfaceRotatedColoredLights& loaded, VkDeviceSize& size)
{
    GetStagingBufferSizeRGBANoGlow(appState, (const dsurface_t&)surface, perSurface, loaded, size);
    loaded.originX = surface.origin_x;
    loaded.originY = surface.origin_y;
    loaded.originZ = surface.origin_z;
    loaded.yaw = surface.yaw;
    loaded.pitch = surface.pitch;
    loaded.roll = surface.roll;
	loaded.alpha = surface.alpha;
}

void Scene::GetStagingBufferSize(AppState& appState, const dturbulentrotated_t& turbulent, PerSurfaceData& perSurface, LoadedTurbulentRotated& loaded, VkDeviceSize& size)
{
    GetStagingBufferSize(appState, (const dturbulent_t&)turbulent, perSurface, loaded, size);
    loaded.originX = turbulent.origin_x;
    loaded.originY = turbulent.origin_y;
    loaded.originZ = turbulent.origin_z;
    loaded.yaw = turbulent.yaw;
    loaded.pitch = turbulent.pitch;
    loaded.roll = turbulent.roll;
	loaded.alpha = turbulent.alpha;
}

void Scene::GetStagingBufferSizeRGBANoGlow(AppState& appState, const dturbulentrotated_t& turbulent, PerSurfaceData& perSurface, LoadedTurbulentRotated& loaded, VkDeviceSize& size)
{
    GetStagingBufferSizeRGBANoGlow(appState, (const dturbulent_t&)turbulent, perSurface, loaded, size);
    loaded.originX = turbulent.origin_x;
    loaded.originY = turbulent.origin_y;
    loaded.originZ = turbulent.origin_z;
    loaded.yaw = turbulent.yaw;
    loaded.pitch = turbulent.pitch;
    loaded.roll = turbulent.roll;
	loaded.alpha = turbulent.alpha;
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

void Scene::GetStagingBufferSizeAlias(AppState& appState, const daliascoloredlights_t& alias, LoadedAliasColoredLights& loaded, VkDeviceSize& size)
{
    if (previousApverts != alias.apverts)
    {
        auto entry = aliasVertexCache.find(alias.apverts);
        if (entry == aliasVertexCache.end())
        {
            auto vertexSize = 3 * sizeof(byte);
            loaded.vertices.size = alias.vertex_count * 2 * vertexSize;
            loaded.vertices.buffer = new SharedMemoryBuffer { };
            loaded.vertices.buffer->CreateVertexBuffer(appState, loaded.vertices.size);
			aliasBuffers.MoveToFront(loaded.vertices.buffer);
            size += loaded.vertices.size;
            loaded.vertices.source = alias.apverts;
			aliasBuffers.SetupAliasVertices(loaded.vertices);
            loaded.texCoords.size = alias.vertex_count * 2 * 2 * sizeof(float);
            loaded.texCoords.buffer = new SharedMemoryBuffer { };
            loaded.texCoords.buffer->CreateVertexBuffer(appState, loaded.texCoords.size);
			aliasBuffers.MoveToFront(loaded.texCoords.buffer);
            size += loaded.texCoords.size;
            loaded.texCoords.source = alias.texture_coordinates;
            loaded.texCoords.width = alias.width;
            loaded.texCoords.height = alias.height;
			aliasBuffers.SetupAliasTexCoords(loaded.texCoords);
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
    if (previousTexture != alias.data)
    {
        auto entry = aliasTextureCache.find(alias.data);
        if (entry == aliasTextureCache.end())
        {
            auto mipCount = (int)(std::floor(std::log2(std::max(alias.width, alias.height)))) + 1;
            auto texture = new SharedMemoryTexture { };
            texture->Create(appState, alias.width, alias.height, VK_FORMAT_R8_UINT, mipCount, 1, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
            textures.MoveToFront(texture);
            loaded.texture.size = alias.size;
            size += loaded.texture.size;
            loaded.texture.texture = texture;
            loaded.texture.source = alias.data;
            loaded.texture.mips = 1;
            textures.Setup(loaded.texture);
            aliasTextureCache.insert({ alias.data, texture });
        }
        else
        {
            loaded.texture.texture = entry->second;
            loaded.texture.index = 0;
        }
        previousTexture = alias.data;
        previousSharedMemoryTexture = loaded.texture.texture;
    }
    else
    {
        loaded.texture.texture = previousSharedMemoryTexture;
        loaded.texture.index = 0;
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
    loaded.count = alias.count;
}

void Scene::GetStagingBufferSize(AppState& appState, const dalias_t& alias, LoadedAlias& loaded, Texture* host_colormap, VkDeviceSize& size)
{
	GetStagingBufferSizeAlias(appState, alias, loaded, size);
    if (alias.colormap == nullptr)
    {
        loaded.colormap.size = 0;
        loaded.colormap.texture = host_colormap;
    }
    else
    {
        loaded.colormap.size = 16384;
        size += loaded.colormap.size;
    }
    loaded.isHostColormap = (alias.colormap == nullptr);
    for (auto j = 0; j < 3; j++)
    {
        for (auto i = 0; i < 4; i++)
        {
            loaded.transform[j][i] = alias.transform[j][i];
        }
    }
}

void Scene::GetStagingBufferSize(AppState& appState, const daliascoloredlights_t& alias, LoadedAliasColoredLights& loaded, VkDeviceSize& size)
{
	GetStagingBufferSizeAlias(appState, alias, loaded, size);
    for (auto j = 0; j < 3; j++)
    {
        for (auto i = 0; i < 4; i++)
        {
            loaded.transform[j][i] = alias.transform[j][i];
        }
    }
}

void Scene::GetStagingBufferSize(AppState& appState, const dviewmodel_t& viewmodel, LoadedAlias& loaded, Texture* host_colormap, VkDeviceSize& size)
{
	GetStagingBufferSizeAlias(appState, viewmodel, loaded, size);
    if (viewmodel.colormap == nullptr)
    {
        loaded.colormap.size = 0;
        loaded.colormap.texture = host_colormap;
    }
    else
    {
        loaded.colormap.size = 16384;
        size += loaded.colormap.size;
    }
    loaded.isHostColormap = (viewmodel.colormap == nullptr);
	RelocateViewmodel(appState, viewmodel, loaded);
}

void Scene::GetStagingBufferSize(AppState& appState, const dviewmodelcoloredlights_t& viewmodel, LoadedAliasColoredLights& loaded, VkDeviceSize& size)
{
	GetStagingBufferSizeAlias(appState, viewmodel, loaded, size);
	RelocateViewmodel(appState, viewmodel, loaded);
}

void Scene::RelocateViewmodel(AppState& appState, const dviewmodelcoloredlights_t& viewmodel, LoadedAliasColoredLights& loaded)
{
	if (appState.FromEngine.immersive_hands_enabled && key_dest == key_game)
	{
		vec3_t angles;
		angles[YAW] = appState.FromEngine.viewmodel_rotate1;
		angles[PITCH] = -appState.FromEngine.viewmodel_rotate0;
		angles[ROLL] = appState.FromEngine.viewmodel_rotate2;
		vec3_t forward, right, up;
		AngleVectors (angles, forward, right, up);
		float preRotate[3][4] { };
		for (auto i = 0; i < 3; i++)
		{
			preRotate[i][0] = forward[i];
			preRotate[i][1] = -right[i];
			preRotate[i][2] = up[i];
		}
		preRotate[0][0] *= appState.FromEngine.viewmodel_scale0;
		preRotate[1][1] *= appState.FromEngine.viewmodel_scale1;
		preRotate[2][2] *= appState.FromEngine.viewmodel_scale2;
		float scaling[3][4] { };
		scaling[0][0] = viewmodel.transform[0][0];
		scaling[1][1] = viewmodel.transform[1][1];
		scaling[2][2] = viewmodel.transform[2][2];
		scaling[0][3] = viewmodel.transform[0][3] + appState.FromEngine.viewmodel_offset0;
		scaling[1][3] = viewmodel.transform[1][3] + appState.FromEngine.viewmodel_offset1;
		scaling[2][3] = viewmodel.transform[2][3] + appState.FromEngine.viewmodel_offset2;
		float transform[3][4] { };
		R_ConcatTransforms (preRotate, scaling, transform);
		auto scale = appState.Scale;
		float yaw;
		float pitch;
		float roll;
		float handDeltaX;
		float handDeltaY;
		float handDeltaZ;
		if (appState.FromEngine.dominant_hand_left)
		{
			auto& pose = appState.LeftController.SpaceLocation.pose;
			AppState::AnglesFromQuaternion(pose.orientation, yaw, pitch, roll);
			handDeltaX = pose.position.x / scale;
			handDeltaY = -pose.position.z / scale;
			handDeltaZ = pose.position.y / scale;
		}
		else
		{
			auto& pose = appState.RightController.SpaceLocation.pose;
			AppState::AnglesFromQuaternion(pose.orientation, yaw, pitch, roll);
			handDeltaX = pose.position.x / scale;
			handDeltaY = -pose.position.z / scale;
			handDeltaZ = pose.position.y / scale;
		}
		angles[YAW] = yaw * 180 / M_PI + 90;
		angles[PITCH] = -pitch * 180 / M_PI;
		angles[ROLL] = -roll * 180 / M_PI;
		AngleVectors (angles, forward, right, up);
		float transform2[3][4] { };
		for (auto i = 0; i < 3; i++)
		{
			transform2[i][0] = forward[i];
			transform2[i][1] = -right[i];
			transform2[i][2] = up[i];
		}
		transform2[0][3] = appState.FromEngine.vieworg0 + handDeltaX;
		transform2[1][3] = appState.FromEngine.vieworg1 + handDeltaY;
		transform2[2][3] = appState.FromEngine.vieworg2 + handDeltaZ;
		R_ConcatTransforms (transform2, transform, loaded.transform);
		return;
	}
	R_ConcatTransforms (viewmodel.transform2, viewmodel.transform, loaded.transform);
}

VkDeviceSize Scene::GetStagingBufferSize(AppState& appState, PerFrame& perFrame)
{
	frameCount++;
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
    fencesRGBAColoredLights.Allocate(d_lists.last_fence_rgba_colored_lights);
    fencesRGBANoGlow.Allocate(d_lists.last_fence_rgba_no_glow);
    fencesRGBANoGlowColoredLights.Allocate(d_lists.last_fence_rgba_no_glow_colored_lights);
    fencesRotated.Allocate(d_lists.last_fence_rotated);
    fencesRotatedColoredLights.Allocate(d_lists.last_fence_rotated_colored_lights);
    fencesRotatedRGBA.Allocate(d_lists.last_fence_rotated_rgba);
    fencesRotatedRGBAColoredLights.Allocate(d_lists.last_fence_rotated_rgba_colored_lights);
    fencesRotatedRGBANoGlow.Allocate(d_lists.last_fence_rotated_rgba_no_glow);
    fencesRotatedRGBANoGlowColoredLights.Allocate(d_lists.last_fence_rotated_rgba_no_glow_colored_lights);
    turbulent.Allocate(d_lists.last_turbulent);
    turbulentRGBA.Allocate(d_lists.last_turbulent_rgba);
    turbulentLit.Allocate(d_lists.last_turbulent_lit);
    turbulentColoredLights.Allocate(d_lists.last_turbulent_colored_lights);
    turbulentRGBALit.Allocate(d_lists.last_turbulent_rgba_lit);
    turbulentRGBAColoredLights.Allocate(d_lists.last_turbulent_rgba_colored_lights);
    turbulentRotated.Allocate(d_lists.last_turbulent_rotated);
    turbulentRotatedRGBA.Allocate(d_lists.last_turbulent_rotated_rgba);
    turbulentRotatedLit.Allocate(d_lists.last_turbulent_rotated_lit);
    turbulentRotatedColoredLights.Allocate(d_lists.last_turbulent_rotated_colored_lights);
    turbulentRotatedRGBALit.Allocate(d_lists.last_turbulent_rotated_rgba_lit);
    turbulentRotatedRGBAColoredLights.Allocate(d_lists.last_turbulent_rotated_rgba_colored_lights);
    sprites.Allocate(d_lists.last_sprite);
    alias.Allocate(d_lists.last_alias);
	aliasAlpha.Allocate(d_lists.last_alias_alpha);
	aliasColoredLights.Allocate(d_lists.last_alias_colored_lights);
	aliasAlphaColoredLights.Allocate(d_lists.last_alias_alpha_colored_lights);
	aliasHoley.Allocate(d_lists.last_alias_holey);
	aliasHoleyAlpha.Allocate(d_lists.last_alias_holey_alpha);
	aliasHoleyColoredLights.Allocate(d_lists.last_alias_holey_colored_lights);
	aliasHoleyAlphaColoredLights.Allocate(d_lists.last_alias_holey_alpha_colored_lights);
    viewmodels.Allocate(d_lists.last_viewmodel);
	viewmodelsColoredLights.Allocate(d_lists.last_viewmodel_colored_lights);
	viewmodelsHoley.Allocate(d_lists.last_viewmodel_holey);
	viewmodelsHoleyColoredLights.Allocate(d_lists.last_viewmodel_holey_colored_lights);
    lastParticle = (d_lists.last_particle + 1) / 4 - 1;
    lastColoredIndex8 = d_lists.last_colored_index8;
    lastColoredIndex16 = d_lists.last_colored_index16;
    lastColoredIndex32 = d_lists.last_colored_index32;
	lastCutoutIndex8 = d_lists.last_cutout_index8;
	lastCutoutIndex16 = d_lists.last_cutout_index16;
	lastCutoutIndex32 = d_lists.last_cutout_index32;
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
	appState.FromEngine.time = d_lists.time;
	appState.FromEngine.immersive_hands_enabled = d_lists.immersive_hands_enabled;
	appState.FromEngine.dominant_hand_left = d_lists.dominant_hand_left;
	appState.FromEngine.viewmodel_rotate0 = d_lists.viewmodel_rotate0;
	appState.FromEngine.viewmodel_rotate1 = d_lists.viewmodel_rotate1;
	appState.FromEngine.viewmodel_rotate2 = d_lists.viewmodel_rotate2;
	appState.FromEngine.viewmodel_offset0 = d_lists.viewmodel_offset0;
	appState.FromEngine.viewmodel_offset1 = d_lists.viewmodel_offset1;
	appState.FromEngine.viewmodel_offset2 = d_lists.viewmodel_offset2;
	appState.FromEngine.viewmodel_scale0 = d_lists.viewmodel_scale0;
	appState.FromEngine.viewmodel_scale1 = d_lists.viewmodel_scale1;
	appState.FromEngine.viewmodel_scale2 = d_lists.viewmodel_scale2;

    appState.VertexTransform.m[0] = appState.Scale;
    appState.VertexTransform.m[6] = -appState.Scale;
    appState.VertexTransform.m[9] = appState.Scale;
    appState.VertexTransform.m[12] = -appState.FromEngine.vieworg0 * appState.Scale;
    appState.VertexTransform.m[13] = -appState.FromEngine.vieworg2 * appState.Scale;
    appState.VertexTransform.m[14] = appState.FromEngine.vieworg1 * appState.Scale;

    VkDeviceSize size = 0;

    if (perFrame.paletteChangedFrame != d_palchangecount)
    {
        paletteSize = paletteBufferSize;
        size += paletteSize + paletteSize;
        perFrame.paletteChangedFrame = d_palchangecount;
    }
    if (!host_colormap.empty() && colormap.image == VK_NULL_HANDLE)
    {
        colormap.Create(appState, 256, 64, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        colormapSize = 16384;
        size += colormapSize;
    }

	lightmapDescriptorWrites.clear();
	lightmapDescriptorInfos.clear();

    surfaces.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(surfaces.sorted);
    for (auto i = 0; i <= surfaces.last; i++)
    {
        auto& loaded = surfaces.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.surfaces[i].face];
        GetStagingBufferSize(appState, d_lists.surfaces[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, surfaces.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    surfacesColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(surfacesColoredLights.sorted);
    for (auto i = 0; i <= surfacesColoredLights.last; i++)
    {
        auto& loaded = surfacesColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.surfaces_colored_lights[i].face];
        GetStagingBufferSize(appState, d_lists.surfaces_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, surfacesColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    surfacesRGBA.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(surfacesRGBA.sorted);
    for (auto i = 0; i <= surfacesRGBA.last; i++)
    {
        auto& loaded = surfacesRGBA.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.surfaces_rgba[i].face];
        GetStagingBufferSize(appState, d_lists.surfaces_rgba[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, surfacesRGBA.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (20 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    surfacesRGBAColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(surfacesRGBAColoredLights.sorted);
    for (auto i = 0; i <= surfacesRGBAColoredLights.last; i++)
    {
        auto& loaded = surfacesRGBAColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.surfaces_rgba_colored_lights[i].face];
        GetStagingBufferSize(appState, d_lists.surfaces_rgba_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, surfacesRGBAColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (20 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    surfacesRGBANoGlow.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(surfacesRGBANoGlow.sorted);
    for (auto i = 0; i <= surfacesRGBANoGlow.last; i++)
    {
        auto& loaded = surfacesRGBANoGlow.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.surfaces_rgba_no_glow[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.surfaces_rgba_no_glow[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, surfacesRGBANoGlow.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    surfacesRGBANoGlowColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(surfacesRGBANoGlowColoredLights.sorted);
    for (auto i = 0; i <= surfacesRGBANoGlowColoredLights.last; i++)
    {
        auto& loaded = surfacesRGBANoGlowColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.surfaces_rgba_no_glow_colored_lights[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.surfaces_rgba_no_glow_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, surfacesRGBANoGlowColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    surfacesRotated.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(surfacesRotated.sorted);
    for (auto i = 0; i <= surfacesRotated.last; i++)
    {
        auto& loaded = surfacesRotated.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.surfaces_rotated[i].face];
        GetStagingBufferSize(appState, d_lists.surfaces_rotated[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, surfacesRotated.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    surfacesRotatedColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(surfacesRotatedColoredLights.sorted);
    for (auto i = 0; i <= surfacesRotatedColoredLights.last; i++)
    {
        auto& loaded = surfacesRotatedColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.surfaces_rotated_colored_lights[i].face];
        GetStagingBufferSize(appState, d_lists.surfaces_rotated_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, surfacesRotatedColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    surfacesRotatedRGBA.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(surfacesRotatedRGBA.sorted);
    for (auto i = 0; i <= surfacesRotatedRGBA.last; i++)
    {
        auto& loaded = surfacesRotatedRGBA.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.surfaces_rotated_rgba[i].face];
        GetStagingBufferSize(appState, d_lists.surfaces_rotated_rgba[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, surfacesRotatedRGBA.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    surfacesRotatedRGBAColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(surfacesRotatedRGBAColoredLights.sorted);
    for (auto i = 0; i <= surfacesRotatedRGBAColoredLights.last; i++)
    {
        auto& loaded = surfacesRotatedRGBAColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.surfaces_rotated_rgba_colored_lights[i].face];
        GetStagingBufferSize(appState, d_lists.surfaces_rotated_rgba_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, surfacesRotatedRGBAColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    surfacesRotatedRGBANoGlow.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(surfacesRotatedRGBANoGlow.sorted);
    for (auto i = 0; i <= surfacesRotatedRGBANoGlow.last; i++)
    {
        auto& loaded = surfacesRotatedRGBANoGlow.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.surfaces_rotated_rgba_no_glow[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.surfaces_rotated_rgba_no_glow[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, surfacesRotatedRGBANoGlow.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    surfacesRotatedRGBANoGlowColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(surfacesRotatedRGBANoGlowColoredLights.sorted);
    for (auto i = 0; i <= surfacesRotatedRGBANoGlowColoredLights.last; i++)
    {
        auto& loaded = surfacesRotatedRGBANoGlowColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.surfaces_rotated_rgba_no_glow_colored_lights[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.surfaces_rotated_rgba_no_glow_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, surfacesRotatedRGBANoGlowColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    fences.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(fences.sorted);
    for (auto i = 0; i <= fences.last; i++)
    {
        auto& loaded = fences.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.fences[i].face];
        GetStagingBufferSize(appState, d_lists.fences[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, fences.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    fencesColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(fencesColoredLights.sorted);
    for (auto i = 0; i <= fencesColoredLights.last; i++)
    {
        auto& loaded = fencesColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.fences_colored_lights[i].face];
        GetStagingBufferSize(appState, d_lists.fences_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, fencesColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    fencesRGBA.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(fencesRGBA.sorted);
    for (auto i = 0; i <= fencesRGBA.last; i++)
    {
        auto& loaded = fencesRGBA.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.fences_rgba[i].face];
        GetStagingBufferSize(appState, d_lists.fences_rgba[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, fencesRGBA.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (20 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    fencesRGBAColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(fencesRGBAColoredLights.sorted);
    for (auto i = 0; i <= fencesRGBAColoredLights.last; i++)
    {
        auto& loaded = fencesRGBAColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.fences_rgba_colored_lights[i].face];
        GetStagingBufferSize(appState, d_lists.fences_rgba_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, fencesRGBAColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (20 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    fencesRGBANoGlow.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(fencesRGBANoGlow.sorted);
    for (auto i = 0; i <= fencesRGBANoGlow.last; i++)
    {
        auto& loaded = fencesRGBANoGlow.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.fences_rgba_no_glow[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.fences_rgba_no_glow[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, fencesRGBANoGlow.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    fencesRGBANoGlowColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(fencesRGBANoGlowColoredLights.sorted);
    for (auto i = 0; i <= fencesRGBANoGlowColoredLights.last; i++)
    {
        auto& loaded = fencesRGBANoGlowColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.fences_rgba_no_glow_colored_lights[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.fences_rgba_no_glow_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, fencesRGBANoGlowColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    fencesRotated.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(fencesRotated.sorted);
    for (auto i = 0; i <= fencesRotated.last; i++)
    {
        auto& loaded = fencesRotated.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.fences_rotated[i].face];
        GetStagingBufferSize(appState, d_lists.fences_rotated[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, fencesRotated.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    fencesRotatedColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(fencesRotatedColoredLights.sorted);
    for (auto i = 0; i <= fencesRotatedColoredLights.last; i++)
    {
        auto& loaded = fencesRotatedColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.fences_rotated_colored_lights[i].face];
        GetStagingBufferSize(appState, d_lists.fences_rotated_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, fencesRotatedColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    fencesRotatedRGBA.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(fencesRotatedRGBA.sorted);
    for (auto i = 0; i <= fencesRotatedRGBA.last; i++)
    {
        auto& loaded = fencesRotatedRGBA.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.fences_rotated_rgba[i].face];
        GetStagingBufferSize(appState, d_lists.fences_rotated_rgba[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, fencesRotatedRGBA.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    fencesRotatedRGBAColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(fencesRotatedRGBAColoredLights.sorted);
    for (auto i = 0; i <= fencesRotatedRGBAColoredLights.last; i++)
    {
        auto& loaded = fencesRotatedRGBAColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.fences_rotated_rgba_colored_lights[i].face];
        GetStagingBufferSize(appState, d_lists.fences_rotated_rgba_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, fencesRotatedRGBAColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    fencesRotatedRGBANoGlow.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(fencesRotatedRGBANoGlow.sorted);
    for (auto i = 0; i <= fencesRotatedRGBANoGlow.last; i++)
    {
        auto& loaded = fencesRotatedRGBANoGlow.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.fences_rotated_rgba_no_glow[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.fences_rotated_rgba_no_glow[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, fencesRotatedRGBANoGlow.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    fencesRotatedRGBANoGlowColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(fencesRotatedRGBANoGlowColoredLights.sorted);
    for (auto i = 0; i <= fencesRotatedRGBANoGlowColoredLights.last; i++)
    {
        auto& loaded = fencesRotatedRGBANoGlowColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.fences_rotated_rgba_no_glow_colored_lights[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.fences_rotated_rgba_no_glow_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, fencesRotatedRGBANoGlowColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    turbulent.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(turbulent.sorted);
    for (auto i = 0; i <= turbulent.last; i++)
    {
        auto& loaded = turbulent.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.turbulent[i].face];
        GetStagingBufferSize(appState, d_lists.turbulent[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, turbulent.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (12 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    turbulentRGBA.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(turbulentRGBA.sorted);
    for (auto i = 0; i <= turbulentRGBA.last; i++)
    {
        auto& loaded = turbulentRGBA.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.turbulent_rgba[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rgba[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, turbulentRGBA.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (12 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    turbulentLit.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(turbulentLit.sorted);
    for (auto i = 0; i <= turbulentLit.last; i++)
    {
        auto& loaded = turbulentLit.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.turbulent_lit[i].face];
        GetStagingBufferSize(appState, d_lists.turbulent_lit[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, turbulentLit.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    turbulentColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(turbulentColoredLights.sorted);
    for (auto i = 0; i <= turbulentColoredLights.last; i++)
    {
        auto& loaded = turbulentColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.turbulent_colored_lights[i].face];
        GetStagingBufferSize(appState, d_lists.turbulent_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, turbulentColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    turbulentRGBALit.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(turbulentRGBALit.sorted);
    for (auto i = 0; i <= turbulentRGBALit.last; i++)
    {
        auto& loaded = turbulentRGBALit.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.turbulent_rgba_lit[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rgba_lit[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, turbulentRGBALit.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    turbulentRGBAColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(turbulentRGBAColoredLights.sorted);
    for (auto i = 0; i <= turbulentRGBAColoredLights.last; i++)
    {
        auto& loaded = turbulentRGBAColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.turbulent_rgba_colored_lights[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rgba_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, turbulentRGBAColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (16 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    turbulentRotated.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(turbulentRotated.sorted);
    for (auto i = 0; i <= turbulentRotated.last; i++)
    {
        auto& loaded = turbulentRotated.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.turbulent_rotated[i].face];
        GetStagingBufferSize(appState, d_lists.turbulent_rotated[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, turbulentRotated.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (20 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    turbulentRotatedRGBA.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(turbulentRotatedRGBA.sorted);
    for (auto i = 0; i <= turbulentRotatedRGBA.last; i++)
    {
        auto& loaded = turbulentRotatedRGBA.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.turbulent_rotated_rgba[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rotated_rgba[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, turbulentRotatedRGBA.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (20 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    turbulentRotatedLit.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(turbulentRotatedLit.sorted);
    for (auto i = 0; i <= turbulentRotatedLit.last; i++)
    {
        auto& loaded = turbulentRotatedLit.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.turbulent_rotated_lit[i].face];
        GetStagingBufferSize(appState, d_lists.turbulent_rotated_lit[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, turbulentRotatedLit.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    turbulentRotatedColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(turbulentRotatedColoredLights.sorted);
    for (auto i = 0; i <= turbulentRotatedColoredLights.last; i++)
    {
        auto& loaded = turbulentRotatedColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.turbulent_rotated_colored_lights[i].face];
        GetStagingBufferSize(appState, d_lists.turbulent_rotated_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, turbulentRotatedColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    turbulentRotatedRGBALit.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(turbulentRotatedRGBALit.sorted);
    for (auto i = 0; i <= turbulentRotatedRGBALit.last; i++)
    {
        auto& loaded = turbulentRotatedRGBALit.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.turbulent_rotated_rgba_lit[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rotated_rgba_lit[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, turbulentRotatedRGBALit.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
    turbulentRotatedRGBAColoredLights.SetBases(sortedVerticesSize, sortedIndicesCount);
    SortedSurfaces::Initialize(turbulentRotatedRGBAColoredLights.sorted);
    for (auto i = 0; i <= turbulentRotatedRGBAColoredLights.last; i++)
    {
        auto& loaded = turbulentRotatedRGBAColoredLights.loaded[i];
		auto& perSurface = perSurfaceCache[d_lists.turbulent_rotated_rgba_colored_lights[i].face];
        GetStagingBufferSizeRGBANoGlow(appState, d_lists.turbulent_rotated_rgba_colored_lights[i], perSurface, loaded, size);
        SortedSurfaces::Sort(appState, loaded, i, turbulentRotatedRGBAColoredLights.sorted);
        sortedVerticesCount += loaded.count;
        sortedVerticesSize += (loaded.count * 4 * sizeof(float));
        sortedAttributesSize += (28 * sizeof(float));
        sortedIndicesCount += ((loaded.count - 2) * 3);
    }
	sprites.SetBases(sortedVerticesSize, sortedIndicesCount);
	SortedSurfaces::Initialize(sprites.sorted);
    previousTexture = nullptr;
    for (auto i = 0; i <= sprites.last; i++)
    {
		auto& loaded = sprites.loaded[i];
        GetStagingBufferSize(appState, d_lists.sprites[i], loaded, size);
		SortedSurfaces::Sort(appState, loaded, i, sprites.sorted);
		sortedVerticesCount += loaded.count;
		sortedVerticesSize += (loaded.count * 5 * sizeof(float));
		sortedIndicesCount += ((loaded.count - 2) * 3);
    }
	SortedSurfaces::Initialize(alias.sorted);
    previousApverts = nullptr;
    previousTexture = nullptr;
    for (auto i = 0; i <= alias.last; i++)
    {
		auto& loaded = alias.loaded[i];
        GetStagingBufferSize(appState, d_lists.alias[i], loaded, &colormap, size);
		SortedSurfaces::Sort(appState, loaded, i, alias.sorted);
    }
	SortedSurfaces::Initialize(aliasAlpha.sorted);
	previousApverts = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= aliasAlpha.last; i++)
	{
		auto& loaded = aliasAlpha.loaded[i];
		GetStagingBufferSize(appState, d_lists.alias_alpha[i], loaded, &colormap, size);
		SortedSurfaces::Sort(appState, loaded, i, aliasAlpha.sorted);
	}
	SortedSurfaces::Initialize(aliasColoredLights.sorted);
	previousApverts = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= aliasColoredLights.last; i++)
	{
		auto& loaded = aliasColoredLights.loaded[i];
		GetStagingBufferSize(appState, d_lists.alias_colored_lights[i], loaded, size);
		SortedSurfaces::Sort(appState, loaded, i, aliasColoredLights.sorted);
	}
	SortedSurfaces::Initialize(aliasAlphaColoredLights.sorted);
	previousApverts = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= aliasAlphaColoredLights.last; i++)
	{
		auto& loaded = aliasAlphaColoredLights.loaded[i];
		GetStagingBufferSize(appState, d_lists.alias_alpha_colored_lights[i], loaded, size);
		SortedSurfaces::Sort(appState, loaded, i, aliasAlphaColoredLights.sorted);
	}
	SortedSurfaces::Initialize(aliasHoley.sorted);
	previousApverts = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= aliasHoley.last; i++)
	{
		auto& loaded = aliasHoley.loaded[i];
		GetStagingBufferSize(appState, d_lists.alias_holey[i], loaded, &colormap, size);
		SortedSurfaces::Sort(appState, loaded, i, aliasHoley.sorted);
	}
	SortedSurfaces::Initialize(aliasHoleyAlpha.sorted);
	previousApverts = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= aliasHoleyAlpha.last; i++)
	{
		auto& loaded = aliasHoleyAlpha.loaded[i];
		GetStagingBufferSize(appState, d_lists.alias_holey_alpha[i], loaded, &colormap, size);
		SortedSurfaces::Sort(appState, loaded, i, aliasHoleyAlpha.sorted);
	}
	SortedSurfaces::Initialize(aliasHoleyColoredLights.sorted);
	previousApverts = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= aliasHoleyColoredLights.last; i++)
	{
		auto& loaded = aliasHoleyColoredLights.loaded[i];
		GetStagingBufferSize(appState, d_lists.alias_holey_colored_lights[i], loaded, size);
		SortedSurfaces::Sort(appState, loaded, i, aliasHoleyColoredLights.sorted);
	}
	SortedSurfaces::Initialize(aliasHoleyAlphaColoredLights.sorted);
	previousApverts = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= aliasHoleyAlphaColoredLights.last; i++)
	{
		auto& loaded = aliasHoleyAlphaColoredLights.loaded[i];
		GetStagingBufferSize(appState, d_lists.alias_holey_alpha_colored_lights[i], loaded, size);
		SortedSurfaces::Sort(appState, loaded, i, aliasHoleyAlphaColoredLights.sorted);
	}
	SortedSurfaces::Initialize(viewmodels.sorted);
    previousApverts = nullptr;
    previousTexture = nullptr;
    for (auto i = 0; i <= viewmodels.last; i++)
    {
		auto& loaded = viewmodels.loaded[i];
        GetStagingBufferSize(appState, d_lists.viewmodels[i], loaded, &colormap, size);
		SortedSurfaces::Sort(appState, loaded, i, viewmodels.sorted);
    }
	SortedSurfaces::Initialize(viewmodelsColoredLights.sorted);
    previousApverts = nullptr;
    previousTexture = nullptr;
    for (auto i = 0; i <= viewmodelsColoredLights.last; i++)
    {
		auto& loaded = viewmodelsColoredLights.loaded[i];
        GetStagingBufferSize(appState, d_lists.viewmodels_colored_lights[i], loaded, size);
		SortedSurfaces::Sort(appState, loaded, i, viewmodelsColoredLights.sorted);
    }
	SortedSurfaces::Initialize(viewmodelsHoley.sorted);
	previousApverts = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= viewmodelsHoley.last; i++)
	{
		auto& loaded = viewmodelsHoley.loaded[i];
		GetStagingBufferSize(appState, d_lists.viewmodels_holey[i], loaded, &colormap, size);
		SortedSurfaces::Sort(appState, loaded, i, viewmodelsHoley.sorted);
	}
	SortedSurfaces::Initialize(viewmodelsHoleyColoredLights.sorted);
	previousApverts = nullptr;
	previousTexture = nullptr;
	for (auto i = 0; i <= viewmodelsHoleyColoredLights.last; i++)
	{
		auto& loaded = viewmodelsHoleyColoredLights.loaded[i];
		GetStagingBufferSize(appState, d_lists.viewmodels_holey_colored_lights[i], loaded, size);
		SortedSurfaces::Sort(appState, loaded, i, viewmodelsHoleyColoredLights.sorted);
	}
	perFrame.particleBase = sortedVerticesSize;
	if (lastParticle >= 0)
	{
		sortedVerticesSize += (d_lists.last_particle + 1) * sizeof(float);
	}
    if (lastSky >= 0)
    {
        loadedSky.width = d_lists.sky[lastSky].width;
        loadedSky.height = d_lists.sky[lastSky].height;
        loadedSky.size = d_lists.sky[lastSky].size;
        loadedSky.data = d_lists.sky[lastSky].data;
        loadedSky.firstVertex = d_lists.sky[lastSky].first_vertex;
        loadedSky.count = d_lists.sky[lastSky].count;
        if (perFrame.sky == nullptr)
        {
            perFrame.sky = new Texture { };
            perFrame.sky->Create(appState, loadedSky.width, loadedSky.height, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        }
        size += loadedSky.size;
    }
    if (lastSkyRGBA >= 0)
    {
        loadedSkyRGBA.width = d_lists.sky_rgba[lastSkyRGBA].width;
        loadedSkyRGBA.height = d_lists.sky_rgba[lastSkyRGBA].height;
        loadedSkyRGBA.size = d_lists.sky_rgba[lastSkyRGBA].size;
        loadedSkyRGBA.data = d_lists.sky_rgba[lastSkyRGBA].data;
        loadedSkyRGBA.firstVertex = d_lists.sky_rgba[lastSkyRGBA].first_vertex;
        loadedSkyRGBA.count = d_lists.sky_rgba[lastSkyRGBA].count;
        if (perFrame.skyRGBA == nullptr)
        {
            perFrame.skyRGBA = new Texture { };
            perFrame.skyRGBA->Create(appState, loadedSkyRGBA.width * 2, loadedSkyRGBA.height, VK_FORMAT_R8G8B8A8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        }
        size += loadedSkyRGBA.size;
    }
    floorVerticesSize = 0;
    if (appState.Mode != AppWorldMode)
    {
        floorVerticesSize += 3 * 4 * sizeof(float);
    }
	controllerVerticesSize = 0;
    if (appState.Focused && (key_dest == key_console || key_dest == key_menu || appState.Mode != AppWorldMode))
    {
		controllerVerticesSize += appState.LeftController.VerticesSize();
		controllerVerticesSize += appState.RightController.VerticesSize();
    }
	skyVerticesSize = 0;
	if (appState.Scene.lastSky >= 0)
	{
		skyVerticesSize += appState.Scene.loadedSky.count * 3 * sizeof(float);
	}
	if (appState.Scene.lastSkyRGBA >= 0)
	{
		skyVerticesSize += appState.Scene.loadedSkyRGBA.count * 3 * sizeof(float);
	}
    coloredVerticesSize = (d_lists.last_colored_vertex + 1) * sizeof(float);
	cutoutVerticesSize = (d_lists.last_cutout_vertex + 1) * sizeof(float);
    verticesSize = floorVerticesSize + controllerVerticesSize + skyVerticesSize + coloredVerticesSize + cutoutVerticesSize;
    if (verticesSize > 0)
    {
        perFrame.vertices = perFrame.cachedVertices.GetVertexBuffer(appState, verticesSize);
    }
    size += verticesSize;
    floorAttributesSize = 0;
    if (appState.Mode != AppWorldMode)
    {
        floorAttributesSize += 2 * 4 * sizeof(float);
    }
    controllerAttributesSize = 0;
    if (controllerVerticesSize > 0)
    {
		controllerAttributesSize += appState.LeftController.AttributesSize();
		controllerAttributesSize += appState.RightController.AttributesSize();
    }
	skyAttributesSize = 0;
	if (appState.Scene.lastSky >= 0)
	{
		skyAttributesSize += appState.Scene.loadedSky.count * 2 * sizeof(float);
	}
	if (appState.Scene.lastSkyRGBA >= 0)
	{
		skyAttributesSize += appState.Scene.loadedSkyRGBA.count * 2 * sizeof(float);
	}
	aliasAttributesSize = (d_lists.last_alias_attribute + 1) * sizeof(float);
    attributesSize = floorAttributesSize + controllerAttributesSize + skyAttributesSize + aliasAttributesSize;
    if (attributesSize > 0)
    {
        perFrame.attributes = perFrame.cachedAttributes.GetVertexBuffer(appState, attributesSize);
    }
    size += attributesSize;
    coloredColorsSize = (d_lists.last_colored_color + 1) * sizeof(float);
    colorsSize = coloredColorsSize;
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
		controllerIndicesSize += appState.LeftController.IndicesSize();
		controllerIndicesSize += appState.RightController.IndicesSize();
    }
    coloredIndices8Size = lastColoredIndex8 + 1;
    coloredIndices16Size = (lastColoredIndex16 + 1) * sizeof(uint16_t);
	coloredIndices32Size = (lastColoredIndex32 + 1) * sizeof(uint32_t);
	cutoutIndices8Size = lastCutoutIndex8 + 1;
	cutoutIndices16Size = (lastCutoutIndex16 + 1) * sizeof(uint16_t);
	cutoutIndices32Size = (lastCutoutIndex32 + 1) * sizeof(uint32_t);

    if (sortedVerticesCount < UPPER_16BIT_LIMIT)
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
        fencesRGBAColoredLights.ScaleIndexBase(sizeof(uint16_t));
        fencesRGBANoGlow.ScaleIndexBase(sizeof(uint16_t));
        fencesRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint16_t));
        fencesRotated.ScaleIndexBase(sizeof(uint16_t));
        fencesRotatedColoredLights.ScaleIndexBase(sizeof(uint16_t));
        fencesRotatedRGBA.ScaleIndexBase(sizeof(uint16_t));
        fencesRotatedRGBAColoredLights.ScaleIndexBase(sizeof(uint16_t));
        fencesRotatedRGBANoGlow.ScaleIndexBase(sizeof(uint16_t));
        fencesRotatedRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint16_t));
        turbulent.ScaleIndexBase(sizeof(uint16_t));
        turbulentRGBA.ScaleIndexBase(sizeof(uint16_t));
        turbulentLit.ScaleIndexBase(sizeof(uint16_t));
        turbulentColoredLights.ScaleIndexBase(sizeof(uint16_t));
        turbulentRGBALit.ScaleIndexBase(sizeof(uint16_t));
        turbulentRGBAColoredLights.ScaleIndexBase(sizeof(uint16_t));
        turbulentRotated.ScaleIndexBase(sizeof(uint16_t));
        turbulentRotatedRGBA.ScaleIndexBase(sizeof(uint16_t));
        turbulentRotatedLit.ScaleIndexBase(sizeof(uint16_t));
        turbulentRotatedColoredLights.ScaleIndexBase(sizeof(uint16_t));
        turbulentRotatedRGBALit.ScaleIndexBase(sizeof(uint16_t));
        turbulentRotatedRGBAColoredLights.ScaleIndexBase(sizeof(uint16_t));
		sprites.ScaleIndexBase(sizeof(uint16_t));
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
        fencesRGBAColoredLights.ScaleIndexBase(sizeof(uint32_t));
        fencesRGBANoGlow.ScaleIndexBase(sizeof(uint32_t));
        fencesRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint32_t));
        fencesRotated.ScaleIndexBase(sizeof(uint32_t));
        fencesRotatedColoredLights.ScaleIndexBase(sizeof(uint32_t));
        fencesRotatedRGBA.ScaleIndexBase(sizeof(uint32_t));
        fencesRotatedRGBAColoredLights.ScaleIndexBase(sizeof(uint32_t));
        fencesRotatedRGBANoGlow.ScaleIndexBase(sizeof(uint32_t));
        fencesRotatedRGBANoGlowColoredLights.ScaleIndexBase(sizeof(uint32_t));
        turbulent.ScaleIndexBase(sizeof(uint32_t));
        turbulentRGBA.ScaleIndexBase(sizeof(uint32_t));
        turbulentLit.ScaleIndexBase(sizeof(uint32_t));
        turbulentColoredLights.ScaleIndexBase(sizeof(uint32_t));
        turbulentRGBALit.ScaleIndexBase(sizeof(uint32_t));
        turbulentRGBAColoredLights.ScaleIndexBase(sizeof(uint32_t));
        turbulentRotated.ScaleIndexBase(sizeof(uint32_t));
        turbulentRotatedRGBA.ScaleIndexBase(sizeof(uint32_t));
        turbulentRotatedLit.ScaleIndexBase(sizeof(uint32_t));
        turbulentRotatedColoredLights.ScaleIndexBase(sizeof(uint32_t));
        turbulentRotatedRGBALit.ScaleIndexBase(sizeof(uint32_t));
        turbulentRotatedRGBAColoredLights.ScaleIndexBase(sizeof(uint32_t));
		sprites.ScaleIndexBase(sizeof(uint32_t));
    }

    if (appState.IndexTypeUInt8Enabled)
    {
        indices8Size = floorIndicesSize + controllerIndicesSize + coloredIndices8Size + cutoutIndices8Size;
        indices16Size = coloredIndices16Size + cutoutIndices16Size;
    }
    else
    {
        floorIndicesSize *= sizeof(uint16_t);
        controllerIndicesSize *= sizeof(uint16_t);
        indices8Size = 0;
        indices16Size = floorIndicesSize + controllerIndicesSize + coloredIndices8Size * sizeof(uint16_t) + coloredIndices16Size + cutoutIndices8Size * sizeof(uint16_t) + cutoutIndices16Size;
    }
	indices32Size = coloredIndices32Size + cutoutIndices32Size;

    if (indices8Size > 0)
    {
        perFrame.indices8 = perFrame.cachedIndices8.GetIndexBuffer(appState, indices8Size);
    }
    size += indices8Size;

    if (indices16Size > 0)
    {
        perFrame.indices16 = perFrame.cachedIndices16.GetIndexBuffer(appState, indices16Size);
    }
    size += indices16Size;

    if (indices32Size > 0)
    {
        perFrame.indices32 = perFrame.cachedIndices32.GetIndexBuffer(appState, indices32Size);
    }
    size += indices32Size;

    // Add extra space (and also realign to a 8-byte boundary) to compensate for alignment among 8-, 16-, 32- and 64-bit data:
    size += 32;
    while (size % 8 != 0)
    {
        size++;
    }

    return size;
}

void Scene::Reset(AppState& appState)
{
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
	for (auto& entry : perSurfaceCache)
	{
		if (entry.second.lightmap != nullptr)
		{
			appState.Scene.lightmapsToDelete.Dispose(entry.second.lightmap);
			entry.second.lightmap = nullptr;
		}
		if (entry.second.lightmapRGB != nullptr)
		{
			appState.Scene.lightmapsRGBToDelete.Dispose(entry.second.lightmapRGB);
			entry.second.lightmapRGB = nullptr;
		}
	}
	perSurfaceCache.clear();
    indexBuffers.DisposeFront();
	aliasBuffers.DisposeFront();
	if (latestTextureDescriptorSets != nullptr)
	{
		latestTextureDescriptorSets->referenceCount--;
		if (latestTextureDescriptorSets->referenceCount == 0)
		{
			vkDestroyDescriptorPool(appState.Device, latestTextureDescriptorSets->descriptorPool, nullptr);
			delete latestTextureDescriptorSets;
		}
		latestTextureDescriptorSets = nullptr;
	}
    latestIndexBuffer32 = nullptr;
    usedInLatestIndexBuffer32 = 0;
    latestIndexBuffer16 = nullptr;
    usedInLatestIndexBuffer16 = 0;
    latestIndexBuffer8 = nullptr;
    usedInLatestIndexBuffer8 = 0;
	for (auto& entry : latestMemory)
	{
		for (auto& used : entry)
		{
			used.memory->referenceCount--;
			if (used.memory->referenceCount == 0)
			{
				vkFreeMemory(appState.Device, used.memory->memory, nullptr);
			}
		}
		entry.clear();
	}
    Skybox::MoveToPrevious(*this);
    aliasIndexCache.clear();
    aliasVertexCache.clear();
}

void Scene::Destroy(AppState& appState)
{
	if (sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(appState.Device, sampler, nullptr);
		sampler = VK_NULL_HANDLE;
	}

	controllerTexture.Delete(appState);
	floorTexture.Delete(appState);

	textures.Delete(appState);

	for (auto& entry : surfaceRGBATextures)
	{
		entry.Delete(appState);
	}
	surfaceRGBATextures.clear();

	for (auto& entry : surfaceTextures)
	{
		entry.Delete(appState);
	}
	surfaceTextures.clear();

	lightmapsRGBToDelete.Delete(appState);
	lightmapRGBBuffers.clear();

	lightmapsToDelete.Delete(appState);
	lightmapBuffers.clear();

	for (auto& entry : perSurfaceCache)
	{
		if (entry.second.lightmapRGB != nullptr)
		{
			entry.second.lightmapRGB->Delete(appState);
		}
		if (entry.second.lightmap != nullptr)
		{
			entry.second.lightmap->Delete(appState);
		}
	}
	perSurfaceCache.clear();

	indexBuffers.Delete(appState);
	aliasBuffers.Delete(appState);

	if (colormap.image != VK_NULL_HANDLE)
	{
		colormap.Delete(appState);
	}

	for (auto& buffer : neutralPaletteBuffers)
	{
		if (buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(appState.Device, buffer, nullptr);
		}
	}
	for (auto& buffer : paletteBuffers)
	{
		if (buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(appState.Device, buffer, nullptr);
		}
	}
	if (paletteMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(appState.Device, paletteMemory, nullptr);
	}

	floor.Delete(appState);
	controllers.Delete(appState);
	skyRGBA.Delete(appState);
	sky.Delete(appState);
	cutout.Delete(appState);
	colored.Delete(appState);
	particles.Delete(appState);
	viewmodelsHoleyColoredLights.Delete(appState);
	viewmodelsHoley.Delete(appState);
	viewmodelsColoredLights.Delete(appState);
	viewmodels.Delete(appState);
	aliasHoleyAlphaColoredLights.Delete(appState);
	aliasHoleyColoredLights.Delete(appState);
	aliasHoleyAlpha.Delete(appState);
	aliasHoley.Delete(appState);
	aliasAlphaColoredLights.Delete(appState);
	aliasColoredLights.Delete(appState);
	aliasAlpha.Delete(appState);
	alias.Delete(appState);
	sprites.Delete(appState);
	turbulentRotatedRGBAColoredLights.Delete(appState);
	turbulentRotatedRGBALit.Delete(appState);
	turbulentRotatedColoredLights.Delete(appState);
	turbulentRotatedLit.Delete(appState);
	turbulentRotatedRGBA.Delete(appState);
	turbulentRotated.Delete(appState);
	turbulentRGBAColoredLights.Delete(appState);
	turbulentRGBALit.Delete(appState);
	turbulentColoredLights.Delete(appState);
	turbulentLit.Delete(appState);
	turbulentRGBA.Delete(appState);
	turbulent.Delete(appState);
	fencesRotatedRGBANoGlowColoredLights.Delete(appState);
	fencesRotatedRGBANoGlow.Delete(appState);
	fencesRotatedRGBAColoredLights.Delete(appState);
	fencesRotatedRGBA.Delete(appState);
	fencesRotatedColoredLights.Delete(appState);
	fencesRotated.Delete(appState);
	fencesRGBANoGlowColoredLights.Delete(appState);
	fencesRGBANoGlow.Delete(appState);
	fencesRGBAColoredLights.Delete(appState);
	fencesRGBA.Delete(appState);
	fencesColoredLights.Delete(appState);
	fences.Delete(appState);
	surfacesRotatedRGBANoGlowColoredLights.Delete(appState);
	surfacesRotatedRGBANoGlow.Delete(appState);
	surfacesRotatedRGBAColoredLights.Delete(appState);
	surfacesRotatedRGBA.Delete(appState);
	surfacesRotatedColoredLights.Delete(appState);
	surfacesRotated.Delete(appState);
	surfacesRGBANoGlowColoredLights.Delete(appState);
	surfacesRGBANoGlow.Delete(appState);
	surfacesRGBAColoredLights.Delete(appState);
	surfacesRGBA.Delete(appState);
	surfacesColoredLights.Delete(appState);
	surfaces.Delete(appState);

	vkDestroyDescriptorSetLayout(appState.Device, singleImageLayout, nullptr);
	singleImageLayout = VK_NULL_HANDLE;
	vkDestroyDescriptorSetLayout(appState.Device, twoBuffersAndImageLayout, nullptr);
	twoBuffersAndImageLayout = VK_NULL_HANDLE;
	vkDestroyDescriptorSetLayout(appState.Device, doubleBufferLayout, nullptr);
	doubleBufferLayout = VK_NULL_HANDLE;
	vkDestroyDescriptorSetLayout(appState.Device, singleBufferLayout, nullptr);
	singleBufferLayout = VK_NULL_HANDLE;
	vkDestroyDescriptorSetLayout(appState.Device, singleFragmentStorageBufferLayout, nullptr);
	singleFragmentStorageBufferLayout = VK_NULL_HANDLE;
	vkDestroyDescriptorSetLayout(appState.Device, singleStorageBufferLayout, nullptr);
	singleStorageBufferLayout = VK_NULL_HANDLE;
}