#pragma once

#include <vulkan/vulkan.h>
#include <common/xr_dependencies.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <vector>

struct Skybox
{
	Skybox* next;
	int unusedCount;
	std::vector<void*> sources;
	XrSwapchain swapchain;
	std::vector<XrSwapchainImageVulkan2KHR> images;

	void Create(struct AppState& appState, int width, int height, struct dskybox_t& skybox, uint32_t swapchainImageIndex);
	void Delete(AppState& appState) const;
	static void MoveToPrevious(struct Scene& scene);
	static void DeleteOld(AppState& appState);
	static void DeleteAll(AppState& appState);
};
