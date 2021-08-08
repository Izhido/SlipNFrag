#pragma once

#include <jni.h>
#include <EGL/egl.h>
#include <vulkan/vulkan.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <vector>
#include "Buffer.h"

struct Screen
{
	XrSwapchain Swapchain;
	std::vector<XrSwapchainImageVulkan2KHR> VulkanImages;
	std::vector<XrSwapchainImageBaseHeader*> Images;
	std::vector<uint32_t> Data;
	Buffer StagingBuffer;
	std::vector<VkCommandBuffer> CommandBuffers;
	std::vector<VkSubmitInfo> SubmitInfo;
};
