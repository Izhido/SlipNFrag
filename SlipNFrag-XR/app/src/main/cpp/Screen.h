#pragma once

#include <vulkan/vulkan.h>
#include <common/xr_dependencies.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <unordered_map>
#include "Buffer.h"
#include "Texture.h"
#include "ScreenPerFrame.h"

struct Screen
{
	XrSwapchain swapchain;
	std::vector<XrSwapchainImageVulkan2KHR> swapchainImages;
	std::unordered_map<uint32_t, ScreenPerFrame> perFrame;
};
