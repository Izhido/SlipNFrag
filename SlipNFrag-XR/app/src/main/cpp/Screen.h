#pragma once

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_VULKAN

#include <vulkan/vulkan.h>
#include <common/xr_dependencies.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <vector>
#include "Buffer.h"
#include "Texture.h"
#include "ScreenPerImage.h"

struct Screen
{
	XrSwapchain Swapchain;
	std::vector<ScreenPerImage> PerImage;
};
