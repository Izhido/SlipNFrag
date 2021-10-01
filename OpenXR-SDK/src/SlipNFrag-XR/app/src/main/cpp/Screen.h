#pragma once

#include <jni.h>
#include <EGL/egl.h>
#include <vulkan/vulkan.h>
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
