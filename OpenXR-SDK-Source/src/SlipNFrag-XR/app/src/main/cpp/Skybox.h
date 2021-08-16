#pragma once

#include <jni.h>
#include <EGL/egl.h>
#include <vulkan/vulkan.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <vector>

struct AppState;

struct Skybox
{
	Skybox* next;
	int unusedCount;
	XrSwapchain swapchain;
	std::vector<XrSwapchainImageVulkan2KHR> vulkanImages;
	std::vector<XrSwapchainImageBaseHeader*> images;

	void Delete(AppState& appState) const;
	static void DeleteOld(AppState& appState);
	static void DeleteAll(AppState& appState);
};
