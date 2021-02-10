#include "AppState.h"
#include "VulkanCallWrappers.h"
#include "vid_ovr.h"
#include "d_lists.h"
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
		VkBufferMemoryBarrier bufferMemoryBarrier { };
		bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		bufferMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		bufferMemoryBarrier.buffer = stagingBuffer->buffer;
		bufferMemoryBarrier.size = stagingBuffer->size;
		VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
		VkBufferCopy bufferCopy { };
		bufferCopy.size = Scene.matrices.size;
		VC(Device.vkCmdCopyBuffer(perImage.commandBuffer, stagingBuffer->buffer, Scene.matrices.buffer, 1, &bufferCopy));
		bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		bufferMemoryBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
		bufferMemoryBarrier.buffer = Scene.matrices.buffer;
		bufferMemoryBarrier.size = Scene.matrices.size;
		VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
		Scene.ClearSizes();
		perImage.LoadBuffers(*this, bufferMemoryBarrier);
		perImage.GetStagingBufferSize(*this, view, Scene.stagingBufferSize, Scene.floorSize);
		perImage.hostClearCount = host_clearcount;
		stagingBuffer = nullptr;
		if (Scene.stagingBufferSize > 0)
		{
			for (Buffer** b = &perImage.stagingBuffers.oldBuffers; *b != nullptr; b = &(*b)->next)
			{
				if ((*b)->size >= Scene.stagingBufferSize && (*b)->size < Scene.stagingBufferSize * 2)
				{
					stagingBuffer = *b;
					*b = (*b)->next;
					break;
				}
			}
			if (stagingBuffer == nullptr)
			{
				stagingBuffer = new Buffer();
				stagingBuffer->CreateStagingStorageBuffer(*this, Scene.stagingBufferSize + Scene.stagingBufferSize / 4);
			}
			perImage.stagingBuffers.MoveToFront(stagingBuffer);
			perImage.LoadStagingBuffer(*this, stagingBuffer, Scene.stagingBufferSize, Scene.floorSize);
			perImage.FillTextures(*this, stagingBuffer);
		}
		double clearR;
		double clearG;
		double clearB;
		double clearA;
		if (Scene.skybox != VK_NULL_HANDLE)
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
		if (Scene.verticesSize > 0)
		{
			VkDescriptorPoolSize poolSizes[2] { };
			VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { };
			descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolCreateInfo.maxSets = 1;
			descriptorPoolCreateInfo.pPoolSizes = poolSizes;
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { };
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			VkDescriptorImageInfo textureInfo[2] { };
			textureInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			textureInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			VkWriteDescriptorSet writes[2] { };
			writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].descriptorCount = 1;
			writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].descriptorCount = 1;
			writes[1].dstBinding = 1;
			if (!perImage.host_colormapResources.created && perImage.host_colormap != nullptr)
			{
				poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolSizes[0].descriptorCount = 1;
				textureInfo[0].sampler = Scene.samplers[perImage.host_colormap->mipCount];
				textureInfo[0].imageView = perImage.host_colormap->view;
				writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writes[0].pImageInfo = textureInfo;
				descriptorPoolCreateInfo.poolSizeCount = 1;
				VK(Device.vkCreateDescriptorPool(Device.device, &descriptorPoolCreateInfo, nullptr, &perImage.host_colormapResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = perImage.host_colormapResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &Scene.singleImageLayout;
				VK(Device.vkAllocateDescriptorSets(Device.device, &descriptorSetAllocateInfo, &perImage.host_colormapResources.descriptorSet));
				writes[0].dstSet = perImage.host_colormapResources.descriptorSet;
				VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
				perImage.host_colormapResources.created = true;
			}
			if (!perImage.sceneMatricesResources.created || !perImage.sceneMatricesAndPaletteResources.created)
			{
				poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				poolSizes[0].descriptorCount = 1;
				VkDescriptorBufferInfo bufferInfo { };
				bufferInfo.buffer = Scene.matrices.buffer;
				bufferInfo.range = Scene.matrices.size;
				writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writes[0].pBufferInfo = &bufferInfo;
				if (!perImage.sceneMatricesResources.created)
				{
					descriptorPoolCreateInfo.poolSizeCount = 1;
					VK(Device.vkCreateDescriptorPool(Device.device, &descriptorPoolCreateInfo, nullptr, &perImage.sceneMatricesResources.descriptorPool));
					descriptorSetAllocateInfo.descriptorPool = perImage.sceneMatricesResources.descriptorPool;
					descriptorSetAllocateInfo.pSetLayouts = &Scene.singleBufferLayout;
					VK(Device.vkAllocateDescriptorSets(Device.device, &descriptorSetAllocateInfo, &perImage.sceneMatricesResources.descriptorSet));
					writes[0].dstSet = perImage.sceneMatricesResources.descriptorSet;
					VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
					perImage.sceneMatricesResources.created = true;
				}
				if (!perImage.sceneMatricesAndPaletteResources.created)
				{
					poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					poolSizes[1].descriptorCount = 1;
					textureInfo[0].sampler = Scene.samplers[perImage.palette->mipCount];
					textureInfo[0].imageView = perImage.palette->view;
					writes[1].dstBinding = 1;
					writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					writes[1].pImageInfo = textureInfo;
					descriptorPoolCreateInfo.poolSizeCount = 2;
					VK(Device.vkCreateDescriptorPool(Device.device, &descriptorPoolCreateInfo, nullptr, &perImage.sceneMatricesAndPaletteResources.descriptorPool));
					descriptorSetAllocateInfo.descriptorPool = perImage.sceneMatricesAndPaletteResources.descriptorPool;
					descriptorSetAllocateInfo.pSetLayouts = &Scene.bufferAndImageLayout;
					VK(Device.vkAllocateDescriptorSets(Device.device, &descriptorSetAllocateInfo, &perImage.sceneMatricesAndPaletteResources.descriptorSet));
					writes[0].dstSet = perImage.sceneMatricesAndPaletteResources.descriptorSet;
					writes[1].dstSet = perImage.sceneMatricesAndPaletteResources.descriptorSet;
					VC(Device.vkUpdateDescriptorSets(Device.device, 2, writes, 0, nullptr));
					perImage.sceneMatricesAndPaletteResources.created = true;
				}
			}
			perImage.Render(*this, poolSizes, descriptorPoolCreateInfo, writes, descriptorSetAllocateInfo, textureInfo);
		}
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
#if defined(_DEBUG)
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "**** [%i, %i, %i, %i, %i] %i; %i, %i, %i = %i; %i; %i; %i, %i, %i, %i = %i; %i, %i, %i = %i; %i, %i = %i; %i, %i, %i, %i = %i",
			Scene.texturedDescriptorSetCount, Scene.spriteDescriptorSetCount, Scene.colormapDescriptorSetCount, Scene.aliasDescriptorSetCount, Scene.viewmodelDescriptorSetCount,
			Scene.stagingBufferSize,
			Scene.floorVerticesSize, Scene.texturedVerticesSize, Scene.coloredVerticesSize, Scene.verticesSize,
			Scene.colormappedVerticesSize,
			Scene.colormappedTexCoordsSize,
			Scene.floorAttributesSize, Scene.texturedAttributesSize, Scene.colormappedLightsSize, Scene.vertexTransformSize, Scene.attributesSize,
			Scene.floorIndicesSize, Scene.colormappedIndices16Size, Scene.coloredIndices16Size, Scene.indices16Size,
			Scene.colormappedIndices32Size, Scene.coloredIndices32Size, Scene.indices32Size,
			Scene.coloredSurfaces16Size, Scene.coloredSurfaces32Size, Scene.particles16Size, Scene.particles32Size, Scene.colorsSize);
#endif
	}
}
