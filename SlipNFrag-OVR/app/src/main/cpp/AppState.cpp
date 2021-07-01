#include "AppState.h"
#include "VulkanCallWrappers.h"
#include "Constants.h"
#include "VrApi_Helpers.h"
#include <android/log.h>
#include "sys_ovr.h"

void AppState::RenderScene(VkCommandBufferBeginInfo& commandBufferBeginInfo)
{
	std::lock_guard<std::mutex> lock(RenderMutex);
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
		perImage.Reset(*this);
		VK(Device.vkResetCommandBuffer(perImage.commandBuffer, 0));
		VK(Device.vkBeginCommandBuffer(perImage.commandBuffer, &commandBufferBeginInfo));
		VkMemoryBarrier memoryBarrier { };
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr));
		VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &view.framebuffer.startBarriers[view.index]));
		Buffer* stagingBuffer;
		if (perImage.sceneMatricesStagingBuffers.oldBuffers != nullptr)
		{
			stagingBuffer = perImage.sceneMatricesStagingBuffers.oldBuffers;
			perImage.sceneMatricesStagingBuffers.oldBuffers = perImage.sceneMatricesStagingBuffers.oldBuffers->next;
		}
		else
		{
			stagingBuffer = new Buffer();
			stagingBuffer->CreateStagingBuffer(*this, Scene.matrices.size);
		}
		perImage.sceneMatricesStagingBuffers.MoveToFront(stagingBuffer);
		VK(Device.vkMapMemory(Device.device, stagingBuffer->memory, 0, stagingBuffer->size, 0, &stagingBuffer->mapped));
		ovrMatrix4f *sceneMatrices = nullptr;
		*((void**)&sceneMatrices) = stagingBuffer->mapped;
		memcpy(sceneMatrices, &ViewMatrices[matrixIndex], Scene.numBuffers * sizeof(ovrMatrix4f));
		memcpy(sceneMatrices + Scene.numBuffers, &ProjectionMatrices[matrixIndex], Scene.numBuffers * sizeof(ovrMatrix4f));
		VC(Device.vkUnmapMemory(Device.device, stagingBuffer->memory));
		stagingBuffer->mapped = nullptr;
		VkBufferMemoryBarrier barrier { };
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.buffer = stagingBuffer->buffer;
		barrier.size = stagingBuffer->size;
		VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr));
		VkBufferCopy bufferCopy { };
		bufferCopy.size = Scene.matrices.size;
		VC(Device.vkCmdCopyBuffer(perImage.commandBuffer, stagingBuffer->buffer, Scene.matrices.buffer, 1, &bufferCopy));
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
		barrier.buffer = Scene.matrices.buffer;
		barrier.size = Scene.matrices.size;
		VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr));
		Scene.ClearSizes();
		perImage.LoadBuffers(*this);
		perImage.GetStagingBufferSize(*this, view, Scene.stagingBufferSize);
		perImage.hostClearCount = host_clearcount;
		stagingBuffer = nullptr;
		if (Scene.stagingBufferSize > 0)
		{
			stagingBuffer = perImage.stagingBuffers.GetStagingStorageBuffer(*this, Scene.stagingBufferSize);
			perImage.LoadStagingBuffer(*this, stagingBuffer, Scene.stagingBufferSize);
			perImage.FillTextures(*this, stagingBuffer);
		}
		double clearR;
		double clearG;
		double clearB;
		double clearA;
		if (Mode != AppWorldMode || Scene.skybox != VK_NULL_HANDLE)
		{
			clearR = 0;
			clearG = 0;
			clearB = 0;
			clearA = 0;
		}
		else if (d_lists.clear_color >= 0)
		{
			auto color = d_8to24table[d_lists.clear_color];
			clearR = (color & 255) / 255.0f;
			clearG = (color >> 8 & 255) / 255.0f;
			clearB = (color >> 16 & 255) / 255.0f;
			clearA = (color >> 24) / 255.0f;
		}
		else
		{
			clearR = 0;
			clearG = 0;
			clearB = 0;
			clearA = 1;
		}
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
		matrixIndex++;
	}
}
