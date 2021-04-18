#pragma once

#include "TwinKey.h"
#include "AllocationList.h"
#include "Buffer.h"

struct AppState;

struct Lightmap
{
	Lightmap* next = nullptr;
	TwinKey key;
	int unusedCount = 0;
	int width = 0;
	int height = 0;
	int layerCount = 0;
	VkImage image = VK_NULL_HANDLE;
	AllocationList* allocationList = nullptr;
	int allocationIndex = 0;
	int allocatedIndex = 0;
	VkImageView view = VK_NULL_HANDLE;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	static std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

	void Create(AppState& appState, VkCommandBuffer commandBuffer, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage);
	void Fill(AppState& appState, Buffer* buffer, VkDeviceSize offset, VkCommandBuffer commandBuffer);
	void Delete(AppState& appState);
	static void DeleteOld(AppState& appState, Lightmap** old);
};
