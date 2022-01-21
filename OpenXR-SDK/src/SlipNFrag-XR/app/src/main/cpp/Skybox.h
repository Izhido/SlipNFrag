#pragma once

#include <vulkan/vulkan.h>
#include <common/xr_dependencies.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <vector>

struct AppState;
struct Scene;

struct Skybox
{
	Skybox* next;
	int unusedCount;
	XrSwapchain swapchain;
	std::vector<XrSwapchainImageVulkanKHR> images;
	
	void Delete(AppState& appState) const;
	static void MoveToPrevious(Scene& scene);
	static void DeleteOld(AppState& appState);
	static void DeleteAll(AppState& appState);
};
