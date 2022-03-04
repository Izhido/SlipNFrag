#include "AppState.h"
#include "PerFrame.h" // This header is specified second in the list to allow headers in AppState.h to include the core engine structs
#include "Utils.h"

void PerFrame::Reset(AppState& appState)
{
	cachedVertices.Reset(appState);
	cachedAttributes.Reset(appState);
	cachedIndices8.Reset(appState);
	cachedIndices16.Reset(appState);
	cachedIndices32.Reset(appState);
	cachedColors.Reset(appState);
	stagingBuffers.Reset(appState);
	colormaps.Reset(appState);
	colormapCount = 0;
	vertices = nullptr;
	attributes = nullptr;
	indices8 = nullptr;
	indices16 = nullptr;
	indices32 = nullptr;
	colors = nullptr;
}

void PerFrame::SetPushConstants(const LoadedAlias& loaded, float pushConstants[])
{
	pushConstants[0] = loaded.transform[0][0];
	pushConstants[1] = loaded.transform[2][0];
	pushConstants[2] = -loaded.transform[1][0];
	pushConstants[4] = loaded.transform[0][2];
	pushConstants[5] = loaded.transform[2][2];
	pushConstants[6] = -loaded.transform[1][2];
	pushConstants[8] = -loaded.transform[0][1];
	pushConstants[9] = -loaded.transform[2][1];
	pushConstants[10] = loaded.transform[1][1];
	pushConstants[12] = loaded.transform[0][3];
	pushConstants[13] = loaded.transform[2][3];
	pushConstants[14] = -loaded.transform[1][3];
}

void PerFrame::Render(AppState& appState, VkCommandBuffer commandBuffer)
{
	if (appState.Scene.lastSurface < 0 &&
		appState.Scene.lastSurfaceRotated < 0 &&
		appState.Scene.lastFence < 0 &&
		appState.Scene.lastFenceRotated < 0 &&
		appState.Scene.lastTurbulent < 0 &&
		appState.Scene.lastTurbulentRotated < 0 &&
		appState.Scene.lastAlias < 0 &&
		appState.Scene.lastViewmodel < 0 &&
		appState.Scene.lastSky < 0 &&
		appState.Scene.verticesSize == 0)
	{
		return;
	}

	VkDescriptorPoolSize poolSizes[3] { };

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.pPoolSizes = poolSizes;

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptorSetAllocateInfo.descriptorSetCount = 1;

	VkDescriptorImageInfo textureInfo { };
	textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet writes[3] { };
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].descriptorCount = 1;
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].descriptorCount = 1;
	writes[1].dstBinding = 1;
	writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[2].descriptorCount = 1;
	writes[2].dstBinding = 2;

	if (!host_colormapResources.created && host_colormap != nullptr)
	{
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = 1;
		textureInfo.sampler = appState.Scene.samplers[host_colormap->mipCount];
		textureInfo.imageView = host_colormap->view;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo = &textureInfo;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &host_colormapResources.descriptorPool));
		descriptorSetAllocateInfo.descriptorPool = host_colormapResources.descriptorPool;
		descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
		CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &host_colormapResources.descriptorSet));
		writes[0].dstSet = host_colormapResources.descriptorSet;
		vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
		host_colormapResources.created = true;
	}
	if (!sceneMatricesResources.created || !sceneMatricesAndPaletteResources.created || (!sceneMatricesAndColormapResources.created && host_colormap != nullptr))
	{
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 1;
		VkDescriptorBufferInfo bufferInfo[2] { };
		bufferInfo[0].buffer = matrices.buffer;
		bufferInfo[0].range = matrices.size;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].pBufferInfo = bufferInfo;
		if (!sceneMatricesResources.created)
		{
			descriptorPoolCreateInfo.poolSizeCount = 1;
			CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = sceneMatricesResources.descriptorPool;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleBufferLayout;
			CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &sceneMatricesResources.descriptorSet));
			writes[0].dstSet = sceneMatricesResources.descriptorSet;
			vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
			sceneMatricesResources.created = true;
		}
		if (!sceneMatricesAndPaletteResources.created || (!sceneMatricesAndColormapResources.created && host_colormap != nullptr))
		{
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[1].descriptorCount = 1;
			bufferInfo[1].buffer = palette->buffer;
			bufferInfo[1].range = palette->size;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[1].pBufferInfo = bufferInfo + 1;
			if (!sceneMatricesAndPaletteResources.created)
			{
				descriptorPoolCreateInfo.poolSizeCount = 2;
				CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesAndPaletteResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = sceneMatricesAndPaletteResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.doubleBufferLayout;
				CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &sceneMatricesAndPaletteResources.descriptorSet));
				writes[0].dstSet = sceneMatricesAndPaletteResources.descriptorSet;
				writes[1].dstSet = sceneMatricesAndPaletteResources.descriptorSet;
				vkUpdateDescriptorSets(appState.Device, 2, writes, 0, nullptr);
				sceneMatricesAndPaletteResources.created = true;
			}
			if (!sceneMatricesAndColormapResources.created && host_colormap != nullptr)
			{
				poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolSizes[2].descriptorCount = 1;
				textureInfo.sampler = appState.Scene.samplers[host_colormap->mipCount];
				textureInfo.imageView = host_colormap->view;
				writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writes[2].pImageInfo = &textureInfo;
				descriptorPoolCreateInfo.poolSizeCount = 3;
				CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesAndColormapResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = sceneMatricesAndColormapResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.twoBuffersAndImageLayout;
				CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &sceneMatricesAndColormapResources.descriptorSet));
				writes[0].dstSet = sceneMatricesAndColormapResources.descriptorSet;
				writes[1].dstSet = sceneMatricesAndColormapResources.descriptorSet;
				writes[2].dstSet = sceneMatricesAndColormapResources.descriptorSet;
				vkUpdateDescriptorSets(appState.Device, 3, writes, 0, nullptr);
				sceneMatricesAndColormapResources.created = true;
			}
		}
	}
	if (appState.Mode == AppWorldMode)
	{
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo = &textureInfo;
		float pushConstants[24];
		if (appState.Scene.lastSky >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &texturedVertexBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &texturedAttributeBase);
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			if (!skyResources.created)
			{
				CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &skyResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = skyResources.descriptorPool;
				descriptorSetAllocateInfo.descriptorSetCount = 1;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
				CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &skyResources.descriptorSet));
				textureInfo.sampler = appState.Scene.samplers[sky->mipCount];
				textureInfo.imageView = sky->view;
				writes[0].dstSet = skyResources.descriptorSet;
				vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
				skyResources.created = true;
			}
			VkDescriptorSet descriptorSets[2];
			descriptorSets[0] = sceneMatricesAndPaletteResources.descriptorSet;
			descriptorSets[1] = skyResources.descriptorSet;
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
			XrMatrix4x4f orientation;
			XrMatrix4x4f_CreateFromQuaternion(&orientation, &appState.CameraLocation.pose.orientation);
			pushConstants[0] = -orientation.m[8];
			pushConstants[1] = orientation.m[10];
			pushConstants[2] = -orientation.m[9];
			pushConstants[3] = orientation.m[0];
			pushConstants[4] = -orientation.m[2];
			pushConstants[5] = orientation.m[1];
			pushConstants[6] = orientation.m[4];
			pushConstants[7] = -orientation.m[6];
			pushConstants[8] = orientation.m[5];
			pushConstants[9] = appState.EyeTextureWidth;
			pushConstants[10] = appState.EyeTextureHeight;
			pushConstants[11] = appState.EyeTextureMaxDimension;
			pushConstants[12] = skytime*skyspeed;
			vkCmdPushConstants(commandBuffer, appState.Scene.sky.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 13 * sizeof(float), &pushConstants);
			vkCmdDraw(commandBuffer, appState.Scene.skyCount, 1, appState.Scene.firstSkyVertex, 0);
		}
		if (appState.Scene.lastSurface >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.Scene.verticesSize);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &appState.Scene.attributesSize);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size, VK_INDEX_TYPE_UINT32);
			}
			for (auto& entry : appState.Scene.sorted.surfaces)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastSurfaceRotated >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedSurfaceRotatedVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedSurfaceRotatedAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedSurfaceRotatedIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedSurfaceRotatedIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			for (auto& entry : appState.Scene.sorted.surfacesRotated)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastFence >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedFenceVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedFenceAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedFenceIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedFenceIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			for (auto& entry : appState.Scene.sorted.fences)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastFenceRotated >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedFenceRotatedVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedFenceRotatedAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedFenceRotatedIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedFenceRotatedIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			for (auto& entry : appState.Scene.sorted.fencesRotated)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastTurbulent >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedTurbulentVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedTurbulentAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedTurbulentIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedTurbulentIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			auto time = (float)cl.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulent.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &time);
			for (auto& entry : appState.Scene.sorted.turbulent)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 1, 1, &entry.texture, 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, entry.indexCount, 1, indexBase, 0, 0);
				indexBase += entry.indexCount;
			}
		}
		if (appState.Scene.lastTurbulentRotated >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotated.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotated.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[6] = (float)cl.time;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePositions = nullptr;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastTurbulentRotated; i++)
			{
				auto& loaded = appState.Scene.loadedTurbulentRotated[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texturePositions = loaded.texturePositions.texturePositions.buffer;
				if (previousTexturePositions != texturePositions)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texturePositions->buffer, &appState.NoOffset);
					previousTexturePositions = texturePositions;
				}
				auto textureDescriptorSet = loaded.texture.texture->descriptorSet;
				if (previousTextureDescriptorSet != textureDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotated.pipelineLayout, 1, 1, &textureDescriptorSet, 0, nullptr);
					previousTextureDescriptorSet = textureDescriptorSet;
				}
				pushConstants[0] = loaded.originX;
				pushConstants[1] = loaded.originY;
				pushConstants[2] = loaded.originZ;
				pushConstants[3] = loaded.yaw * M_PI / 180;
				pushConstants[4] = loaded.pitch * M_PI / 180;
				pushConstants[5] = -loaded.roll * M_PI / 180;
				vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRotated.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 7 * sizeof(float), pushConstants);
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, loaded.texturePositions.texturePositions.firstInstance);
			}
		}
		if (appState.Scene.lastTurbulentLit >= 0)
		{
			TurbulentLitPushConstants pushConstants;
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			pushConstants.time = (float)cl.time;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePositions = nullptr;
			VkDescriptorSet previousLightmapDescriptorSet = VK_NULL_HANDLE;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastTurbulentLit; i++)
			{
				auto& loaded = appState.Scene.loadedTurbulentLit[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texturePositions = loaded.texturePositions.texturePositions.buffer;
				if (previousTexturePositions != texturePositions)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texturePositions->buffer, &appState.NoOffset);
					previousTexturePositions = texturePositions;
				}
				auto lightmapDescriptorSet = loaded.lightmap.lightmap->texture->descriptorSet;
				if (previousLightmapDescriptorSet != lightmapDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipelineLayout, 1, 1, &lightmapDescriptorSet, 0, nullptr);
					previousLightmapDescriptorSet = lightmapDescriptorSet;
				}
				auto textureDescriptorSet = loaded.texture.texture->descriptorSet;
				if (previousTextureDescriptorSet != textureDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipelineLayout, 2, 1, &textureDescriptorSet, 0, nullptr);
					previousTextureDescriptorSet = textureDescriptorSet;
				}
				pushConstants.lightmapIndex = loaded.lightmap.lightmap->allocatedIndex;
				vkCmdPushConstants(commandBuffer, appState.Scene.turbulentLit.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) + sizeof(uint32_t), &pushConstants);
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, loaded.texturePositions.texturePositions.firstInstance);
			}
		}
		if (appState.Scene.lastTurbulentLitRotated >= 0)
		{
			TurbulentLitRotatedPushConstants pushConstants;
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLitRotated.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLitRotated.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants.time = (float)cl.time;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePositions = nullptr;
			VkDescriptorSet previousLightmapDescriptorSet = VK_NULL_HANDLE;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastTurbulentLitRotated; i++)
			{
				auto& loaded = appState.Scene.loadedTurbulentLitRotated[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texturePositions = loaded.texturePositions.texturePositions.buffer;
				if (previousTexturePositions != texturePositions)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texturePositions->buffer, &appState.NoOffset);
					previousTexturePositions = texturePositions;
				}
				auto lightmapDescriptorSet = loaded.lightmap.lightmap->texture->descriptorSet;
				if (previousLightmapDescriptorSet != lightmapDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLitRotated.pipelineLayout, 1, 1, &lightmapDescriptorSet, 0, nullptr);
					previousLightmapDescriptorSet = lightmapDescriptorSet;
				}
				auto textureDescriptorSet = loaded.texture.texture->descriptorSet;
				if (previousTextureDescriptorSet != textureDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLitRotated.pipelineLayout, 2, 1, &textureDescriptorSet, 0, nullptr);
					previousTextureDescriptorSet = textureDescriptorSet;
				}
				pushConstants.lightmapIndex = loaded.lightmap.lightmap->allocatedIndex;
				pushConstants.originX = loaded.originX;
				pushConstants.originY = loaded.originY;
				pushConstants.originZ = loaded.originZ;
				pushConstants.yaw = loaded.yaw * M_PI / 180;
				pushConstants.pitch = loaded.pitch * M_PI / 180;
				pushConstants.roll = -loaded.roll * M_PI / 180;
				vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRotated.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) + 7 * sizeof(float), &pushConstants);
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, loaded.texturePositions.texturePositions.firstInstance);
			}
		}
		if (appState.Scene.lastSprite >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &texturedVertexBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &texturedAttributeBase);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			SharedMemoryTexture* previousTexture = nullptr;
			for (auto i = 0; i <= appState.Scene.lastSprite; i++)
			{
				auto& loaded = appState.Scene.loadedSprites[i];
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				vkCmdDraw(commandBuffer, loaded.count, 1, loaded.firstVertex, 0);
			}
		}
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = 1;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo = &textureInfo;
		if (colormapCount == 0)
		{
			colormapResources.Delete(appState);
		}
		else
		{
			auto size = colormapResources.descriptorSets.size();
			auto required = colormapCount;
			if (size < required || size > required * 2)
			{
				auto toCreate = std::max(4, required + required / 4);
				if (toCreate != size)
				{
					colormapResources.Delete(appState);
					poolSizes[0].descriptorCount = toCreate;
					descriptorPoolCreateInfo.maxSets = toCreate;
					CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &colormapResources.descriptorPool));
					colormapResources.descriptorSetLayouts.resize(toCreate);
					colormapResources.descriptorSets.resize(toCreate);
					colormapResources.bound.resize(toCreate);
					std::fill(colormapResources.descriptorSetLayouts.begin(), colormapResources.descriptorSetLayouts.end(), appState.Scene.singleImageLayout);
					descriptorSetAllocateInfo.descriptorPool = colormapResources.descriptorPool;
					descriptorSetAllocateInfo.descriptorSetCount = toCreate;
					descriptorSetAllocateInfo.pSetLayouts = colormapResources.descriptorSetLayouts.data();
					CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, colormapResources.descriptorSets.data()));
					colormapResources.created = true;
				}
			}
		}
		auto descriptorSetIndex = 0;
		if (appState.Scene.lastAlias >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastAlias; i++)
			{
				auto& loaded = appState.Scene.loadedAlias[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = colormappedAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.alias.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants);
				if (loaded.isHostColormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr);
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = loaded.colormapped.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo.sampler = appState.Scene.samplers[colormap->mipCount];
						textureInfo.imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.colormapped.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
		}
		if (appState.Scene.lastViewmodel >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			if (appState.NearViewmodel)
			{
				pushConstants[16] = appState.ViewmodelForwardX;
				pushConstants[17] = appState.ViewmodelForwardY;
				pushConstants[18] = appState.ViewmodelForwardZ;
				pushConstants[19] = 0;
				pushConstants[20] = 1;
				pushConstants[21] = 1;
				pushConstants[22] = 1;
				pushConstants[23] = 1;
			}
			else
			{
				pushConstants[16] = 1 / appState.Scale;
				pushConstants[17] = 0;
				pushConstants[18] = 0;
				pushConstants[19] = 8;
				pushConstants[20] = 1;
				pushConstants[21] = 0;
				pushConstants[22] = 0;
				pushConstants[23] = 0.7 + 0.3 * sin(cl.time * M_PI);
			}
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastViewmodel; i++)
			{
				auto& loaded = appState.Scene.loadedViewmodels[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = colormappedAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.viewmodel.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 24 * sizeof(float), pushConstants);
				if (loaded.isHostColormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr);
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = loaded.colormapped.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo.sampler = appState.Scene.samplers[colormap->mipCount];
						textureInfo.imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.colormapped.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
		}
		if (appState.Scene.lastParticle >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.particle.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &particlePositionBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &colors->buffer, &appState.NoOffset);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.particle.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[0] = appState.FromEngine.vright0;
			pushConstants[1] = appState.FromEngine.vright2;
			pushConstants[2] = -appState.FromEngine.vright1;
			pushConstants[3] = 0;
			pushConstants[4] = appState.FromEngine.vup0;
			pushConstants[5] = appState.FromEngine.vup2;
			pushConstants[6] = -appState.FromEngine.vup1;
			pushConstants[7] = 0;
			vkCmdPushConstants(commandBuffer, appState.Scene.particle.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 8 * sizeof(float), pushConstants);
			vkCmdDraw(commandBuffer, 6, appState.Scene.lastParticle + 1, 0, 0);
		}
		if (appState.Scene.lastColoredIndex8 >= 0 || appState.Scene.lastColoredIndex16 >= 0 || appState.Scene.lastColoredIndex32 >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.colored.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &coloredVertexBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &colors->buffer, &coloredColorBase);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.colored.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			if (appState.IndexTypeUInt8Enabled)
			{
				if (appState.Scene.lastColoredIndex8 >= 0)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices8->buffer, coloredIndex8Base, VK_INDEX_TYPE_UINT8_EXT);
					vkCmdDrawIndexed(commandBuffer, appState.Scene.lastColoredIndex8 + 1, 1, 0, 0, 0);
				}
				if (appState.Scene.lastColoredIndex16 >= 0)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, coloredIndex16Base, VK_INDEX_TYPE_UINT16);
					vkCmdDrawIndexed(commandBuffer, appState.Scene.lastColoredIndex16 + 1, 1, 0, 0, 0);
				}
			}
			else
			{
				if (appState.Scene.lastColoredIndex8 >= 0 || appState.Scene.lastColoredIndex16 >= 0)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, coloredIndex16Base, VK_INDEX_TYPE_UINT16);
					vkCmdDrawIndexed(commandBuffer, appState.Scene.lastColoredIndex8 + 1 + appState.Scene.lastColoredIndex16 + 1, 1, 0, 0, 0);
				}
			}
			if (appState.Scene.lastColoredIndex32 >= 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32);
				vkCmdDrawIndexed(commandBuffer, appState.Scene.lastColoredIndex32 + 1, 1, 0, 0, 0);
			}
		}
	}
	else
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipeline);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
		vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &appState.NoOffset);
		if (!floorResources.created)
		{
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &floorResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = floorResources.descriptorPool;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
			CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &floorResources.descriptorSet));
			textureInfo.sampler = appState.Scene.samplers[appState.Scene.floorTexture.mipCount];
			textureInfo.imageView = appState.Scene.floorTexture.view;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].pImageInfo = &textureInfo;
			writes[0].dstSet = floorResources.descriptorSet;
			vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
			floorResources.created = true;
		}
		VkDescriptorSet descriptorSets[2];
		descriptorSets[0] = sceneMatricesResources.descriptorSet;
		descriptorSets[1] = floorResources.descriptorSet;
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
		if (appState.IndexTypeUInt8Enabled)
		{
			vkCmdBindIndexBuffer(commandBuffer, indices8->buffer, 0, VK_INDEX_TYPE_UINT8_EXT);
		}
		else
		{
			vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, 0, VK_INDEX_TYPE_UINT16);
		}
		vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
	}
	if (appState.Scene.controllerVerticesSize > 0)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipeline);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &controllerVertexBase);
		vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &controllerAttributeBase);
		if (!controllerResources.created)
		{
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &controllerResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = controllerResources.descriptorPool;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
			CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &controllerResources.descriptorSet));
			textureInfo.sampler = appState.Scene.samplers[appState.Scene.controllerTexture.mipCount];
			textureInfo.imageView = appState.Scene.controllerTexture.view;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].pImageInfo = &textureInfo;
			writes[0].dstSet = controllerResources.descriptorSet;
			vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
			controllerResources.created = true;
		}
		VkDescriptorSet descriptorSets[2];
		descriptorSets[0] = sceneMatricesResources.descriptorSet;
		descriptorSets[1] = controllerResources.descriptorSet;
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
		if (appState.IndexTypeUInt8Enabled)
		{
			vkCmdBindIndexBuffer(commandBuffer, indices8->buffer, controllerIndexBase, VK_INDEX_TYPE_UINT8_EXT);
		}
		else
		{
			vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, controllerIndexBase, VK_INDEX_TYPE_UINT16);
		}
		VkDeviceSize size = 0;
		if (appState.LeftController.PoseIsValid)
		{
			size += 2 * 36;
		}
		if (appState.RightController.PoseIsValid)
		{
			size += 2 * 36;
		}
		vkCmdDrawIndexed(commandBuffer, size, 1, 0, 0, 0);
	}
}
