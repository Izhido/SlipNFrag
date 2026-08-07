#pragma once

#include <common/xr_dependencies.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <vector>
#include <SharedMemoryTexture.h>

struct Skybox
{
	Skybox* next;
	int width;
	int height;
	int unusedCount;
	std::vector<void*> sources;
	XrSwapchain swapchain;
	std::vector<XrSwapchainImageVulkan2KHR> images;
	SharedMemoryTexture* texture;
	bool needsUpload;
	uint32_t uploadSwapchainImageIndex;
	VkDeviceSize stagingBufferOffset;
	bool needsRelease;

	void Initialize(struct AppState& appState, int width, int height, struct dskybox_t& skybox, uint32_t swapchainImageIndex);
	void Initialize(struct AppState& appState, int width, int height, struct dskybox_t& skybox);
	void CopyPixels(struct AppState& appState, struct Buffer* stagingBuffer, struct dskybox_t& skybox);
	void Upload(struct AppState& appState, VkCommandBuffer commandBuffer);
	void Delete(AppState& appState) const;
	static void MoveToPrevious(struct Scene& scene);
	static void DeleteOld(AppState& appState);
	static void DeleteAll(AppState& appState);
};
