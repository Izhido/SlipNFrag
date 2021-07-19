#include "AppState.h"
#include "VulkanCallWrappers.h"
#include "Constants.h"
#include "VrApi_Helpers.h"
#include "sys_ovr.h"
//#include <android/log.h>

void AppState::RenderScene(VkCommandBufferBeginInfo& commandBufferBeginInfo)
{
	auto matrixIndex = 0;
	for (auto& view : Views)
	{
		VkRect2D screenRect { };
		screenRect.extent.width = view.framebuffer.width;
		screenRect.extent.height = view.framebuffer.height;
		view.index = (view.index + 1) % view.framebuffer.swapChainLength;
		auto& perImage = view.perImage[view.index];
		if (perImage.submitted)
		{
			VK(Device.vkWaitForFences(Device.device, 1, &perImage.fence, VK_TRUE, 1ULL * 1000 * 1000 * 1000));
			VK(Device.vkResetFences(Device.device, 1, &perImage.fence));
			perImage.submitted = false;
		}
		VK(Device.vkResetCommandBuffer(perImage.commandBuffer, 0));
		VK(Device.vkBeginCommandBuffer(perImage.commandBuffer, &commandBufferBeginInfo));
		VkMemoryBarrier memoryBarrier { };
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr));
		VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &view.framebuffer.startBarriers[view.index]));
		double clearR = 0;
		double clearG = 0;
		double clearB = 0;
		double clearA = 1;
		auto readClearColor = false;
		if (Mode != AppWorldMode || Scene.skybox != VK_NULL_HANDLE)
		{
			clearA = 0;
		}
		else
		{
			readClearColor = true;
		}
		//double elapsed = 0;
		{
			std::lock_guard<std::mutex> lock(RenderMutex);
			//auto startTime = Sys_FloatTime();
			if (readClearColor && d_lists.clear_color >= 0)
			{
				auto color = d_8to24table[d_lists.clear_color];
				clearR = (color & 255) / 255.0f;
				clearG = (color >> 8 & 255) / 255.0f;
				clearB = (color >> 16 & 255) / 255.0f;
				clearA = (color >> 24) / 255.0f;
			}
			perImage.Reset(*this);
			Scene.Initialize();
			auto stagingBufferSize = perImage.GetStagingBufferSize(*this, view);
			auto stagingBuffer = perImage.stagingBuffers.GetStorageBuffer(*this, stagingBufferSize);
			perImage.LoadStagingBuffer(*this, matrixIndex, stagingBuffer);
			perImage.FillFromStagingBuffer(*this, stagingBuffer);
			perImage.LoadRemainingBuffers(*this);
			perImage.hostClearCount = host_clearcount;
			//auto endTime = Sys_FloatTime();
			//elapsed += (endTime - startTime);
		}
		auto startTime = Sys_FloatTime();
		uint32_t clearValueCount = 0;
		VkClearValue clearValues[3] { };
		clearValues[clearValueCount].color.float32[0] = clearR;
		clearValues[clearValueCount].color.float32[1] = clearG;
		clearValues[clearValueCount].color.float32[2] = clearB;
		clearValues[clearValueCount].color.float32[3] = clearA;
		clearValueCount++;
		clearValues[clearValueCount].color.float32[0] = clearR;
		clearValues[clearValueCount].color.float32[1] = clearG;
		clearValues[clearValueCount].color.float32[2] = clearB;
		clearValues[clearValueCount].color.float32[3] = clearA;
		clearValueCount++;
		clearValues[clearValueCount].depthStencil.depth = 1;
		clearValueCount++;
		VkRenderPassBeginInfo renderPassBeginInfo { };
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = RenderPass;
		renderPassBeginInfo.framebuffer = view.framebuffer.framebuffers[view.index];
		renderPassBeginInfo.renderArea = screenRect;
		renderPassBeginInfo.clearValueCount = clearValueCount;
		renderPassBeginInfo.pClearValues = clearValues;
		VC(Device.vkCmdBeginRenderPass(perImage.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE));
		VkViewport viewport;
		viewport.x = (float) screenRect.offset.x;
		viewport.y = (float) screenRect.offset.y;
		viewport.width = (float) screenRect.extent.width;
		viewport.height = (float) screenRect.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VC(Device.vkCmdSetViewport(perImage.commandBuffer, 0, 1, &viewport));
		VC(Device.vkCmdSetScissor(perImage.commandBuffer, 0, 1, &screenRect));
		perImage.Render(*this);
		VC(Device.vkCmdEndRenderPass(perImage.commandBuffer));
		VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &view.framebuffer.endBarriers[view.index]));
		VK(Device.vkEndCommandBuffer(perImage.commandBuffer));
		VkSubmitInfo submitInfo { };
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &perImage.commandBuffer;
		VK(Device.vkQueueSubmit(Context.queue, 1, &submitInfo, perImage.fence));
		perImage.submitted = true;
		//auto endTime = Sys_FloatTime();
		//elapsed += (endTime - startTime);
		//__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "AppState::RenderScene(): %.6f s.", elapsed);
		matrixIndex++;
	}
}
