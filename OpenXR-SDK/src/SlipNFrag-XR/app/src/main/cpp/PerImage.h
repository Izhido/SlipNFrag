#pragma once

#include "CachedBuffers.h"
#include "CachedTextures.h"
#include "PipelineDescriptorResources.h"
#include "UpdatablePipelineDescriptorResources.h"
#include "SurfaceRotatedPushConstants.h"
#include "TurbulentLitPushConstants.h"
#include "TurbulentRotatedLitPushConstants.h"
#include "PerFrame.h"

struct PerImage
{
	VkImage colorImage;
	VkImage depthImage;
	VkImage resolveImage;
	VkDeviceMemory colorMemory;
	VkDeviceMemory depthMemory;
	VkImageView colorView;
	VkImageView depthView;
	VkImageView resolveView;
	VkFramebuffer framebuffer;
	VkCommandBuffer commandBuffer;
	VkSubmitInfo submitInfo;

	static float GammaCorrect(float component);
	static void LoadStagingBuffer(AppState& appState, PerFrame& perFrame, Buffer* stagingBuffer);
	static void FillColormapTextures(AppState& appState, PerFrame& perFrame, LoadedAlias& loaded);
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const;
	void FillAliasFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const;
	void FillTexturePositionsFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryTexturePositionsBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const;
	void FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryBuffer* first, VkBufferCopy& bufferCopy) const;
	void FillFromStagingBuffer(AppState& appState, PerFrame& perFrame, Buffer* stagingBuffer) const;
};
