#include "PerImage.h"
#include "vid_ovr.h"
#include "AppState.h"
#include "d_lists.h"
#include "VulkanCallWrappers.h"
#include "VrApi_Helpers.h"
#include "Constants.h"

void PerImage::Reset(AppState& appState)
{
	sceneMatricesStagingBuffers.Reset(appState);
	cachedVertices.Reset(appState);
	cachedAttributes.Reset(appState);
	cachedIndices16.Reset(appState);
	cachedIndices32.Reset(appState);
	cachedColors.Reset(appState);
	stagingBuffers.Reset(appState);
	turbulent.Reset(appState);
	colormaps.Reset(appState);
	colormapCount = 0;
	vertices = nullptr;
	attributes = nullptr;
	indices16 = nullptr;
	indices32 = nullptr;
	colors = nullptr;
}

void PerImage::LoadBuffers(AppState& appState, VkBufferMemoryBarrier& bufferMemoryBarrier)
{
	if (appState.Mode != AppWorldMode)
	{
		appState.Scene.floorVerticesSize = 3 * 4 * sizeof(float);
	}
	appState.Scene.texturedVerticesSize = (d_lists.last_textured_vertex + 1) * sizeof(float);
	appState.Scene.coloredVerticesSize = (d_lists.last_colored_vertex + 1) * sizeof(float);
	appState.Scene.verticesSize = appState.Scene.texturedVerticesSize + appState.Scene.coloredVerticesSize + appState.Scene.floorVerticesSize;
	if (appState.Scene.verticesSize > 0 || d_lists.last_alias >= 0 || d_lists.last_viewmodel >= 0)
	{
		vertices = cachedVertices.GetVertexBuffer(appState, appState.Scene.verticesSize);
		VK(appState.Device.vkMapMemory(appState.Device.device, vertices->memory, 0, appState.Scene.verticesSize, 0, &vertices->mapped));
		if (appState.Scene.floorVerticesSize > 0)
		{
			auto mapped = (float*)vertices->mapped;
			(*mapped) = -0.5;
			mapped++;
			(*mapped) = appState.Scene.pose.Position.y;
			mapped++;
			(*mapped) = -0.5;
			mapped++;
			(*mapped) = 0.5;
			mapped++;
			(*mapped) = appState.Scene.pose.Position.y;
			mapped++;
			(*mapped) = -0.5;
			mapped++;
			(*mapped) = 0.5;
			mapped++;
			(*mapped) = appState.Scene.pose.Position.y;
			mapped++;
			(*mapped) = 0.5;
			mapped++;
			(*mapped) = -0.5;
			mapped++;
			(*mapped) = appState.Scene.pose.Position.y;
			mapped++;
			(*mapped) = 0.5;
		}
		texturedVertexBase = appState.Scene.floorVerticesSize;
		memcpy((unsigned char*)vertices->mapped + texturedVertexBase, d_lists.textured_vertices.data(), appState.Scene.texturedVerticesSize);
		coloredVertexBase = texturedVertexBase + appState.Scene.texturedVerticesSize;
		memcpy((unsigned char*)vertices->mapped + coloredVertexBase, d_lists.colored_vertices.data(), appState.Scene.coloredVerticesSize);
		VC(appState.Device.vkUnmapMemory(appState.Device.device, vertices->memory));
		vertices->mapped = nullptr;
		bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		bufferMemoryBarrier.buffer = vertices->buffer;
		bufferMemoryBarrier.size = vertices->size;
		VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
		if (d_lists.last_alias >= appState.Scene.aliasVerticesList.size())
		{
			appState.Scene.aliasVerticesList.resize(d_lists.last_alias + 1);
			appState.Scene.aliasTexCoordsList.resize(d_lists.last_alias + 1);
		}
		appState.Scene.newVertices.clear();
		appState.Scene.newTexCoords.clear();
		VkDeviceSize verticesOffset = 0;
		VkDeviceSize texCoordsOffset = 0;
		for (auto i = 0; i <= d_lists.last_alias; i++)
		{
			auto& alias = d_lists.alias[i];
			auto verticesEntry = appState.Scene.colormappedVerticesPerKey.find(alias.vertices);
			if (verticesEntry == appState.Scene.colormappedVerticesPerKey.end())
			{
				auto lastIndex = appState.Scene.colormappedBufferList.size();
				appState.Scene.colormappedBufferList.push_back({ verticesOffset });
				auto newEntry = appState.Scene.colormappedVerticesPerKey.insert({ alias.vertices, lastIndex });
				appState.Scene.newVertices.push_back(i);
				appState.Scene.aliasVerticesList[i] = lastIndex;
				verticesOffset += alias.vertex_count * 2 * 4 * sizeof(float);
			}
			else
			{
				appState.Scene.aliasVerticesList[i] = verticesEntry->second;
			}
			auto texCoordsEntry = appState.Scene.colormappedTexCoordsPerKey.find(alias.texture_coordinates);
			if (texCoordsEntry == appState.Scene.colormappedTexCoordsPerKey.end())
			{
				auto lastIndex = appState.Scene.colormappedBufferList.size();
				appState.Scene.colormappedBufferList.push_back({ texCoordsOffset });
				auto newEntry = appState.Scene.colormappedTexCoordsPerKey.insert({ alias.texture_coordinates, lastIndex });
				appState.Scene.newTexCoords.push_back(i);
				appState.Scene.aliasTexCoordsList[i] = lastIndex;
				texCoordsOffset += alias.vertex_count * 2 * 2 * sizeof(float);
			}
			else
			{
				appState.Scene.aliasTexCoordsList[i] = texCoordsEntry->second;
			}
		}
		if (verticesOffset > 0)
		{
			Buffer* buffer;
			if (appState.Scene.latestColormappedBuffer == nullptr || appState.Scene.usedInLatestColormappedBuffer + verticesOffset > MEMORY_BLOCK_SIZE)
			{
				VkDeviceSize size = MEMORY_BLOCK_SIZE;
				if (size < verticesOffset)
				{
					size = verticesOffset;
				}
				buffer = new Buffer();
				buffer->CreateVertexBuffer(appState, size);
				appState.Scene.colormappedBuffers.MoveToFront(buffer);
				appState.Scene.latestColormappedBuffer = buffer;
				appState.Scene.usedInLatestColormappedBuffer = 0;
			}
			else
			{
				buffer = appState.Scene.latestColormappedBuffer;
			}
			appState.Scene.colormappedVerticesSize += verticesOffset;
			VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, appState.Scene.usedInLatestColormappedBuffer, verticesOffset, 0, &buffer->mapped));
			auto mapped = (float*)buffer->mapped;
			for (auto i : appState.Scene.newVertices)
			{
				auto& alias = d_lists.alias[i];
				auto vertexFromModel = alias.vertices;
				for (auto j = 0; j < alias.vertex_count; j++)
				{
					auto x = (float)(vertexFromModel->v[0]);
					auto y = (float)(vertexFromModel->v[1]);
					auto z = (float)(vertexFromModel->v[2]);
					(*mapped) = x;
					mapped++;
					(*mapped) = z;
					mapped++;
					(*mapped) = -y;
					mapped++;
					(*mapped) = 1;
					mapped++;
					(*mapped) = x;
					mapped++;
					(*mapped) = z;
					mapped++;
					(*mapped) = -y;
					mapped++;
					(*mapped) = 1;
					mapped++;
					vertexFromModel++;
				}
				auto index = appState.Scene.aliasVerticesList[i];
				appState.Scene.colormappedBufferList[index].buffer = buffer;
				appState.Scene.colormappedBufferList[index].offset += appState.Scene.usedInLatestColormappedBuffer;
			}
			VC(appState.Device.vkUnmapMemory(appState.Device.device, buffer->memory));
			buffer->mapped = nullptr;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferMemoryBarrier.buffer = buffer->buffer;
			bufferMemoryBarrier.size = buffer->size;
			VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			appState.Scene.usedInLatestColormappedBuffer += verticesOffset;
		}
		if (texCoordsOffset > 0)
		{
			Buffer* buffer;
			if (appState.Scene.latestColormappedBuffer == nullptr || appState.Scene.usedInLatestColormappedBuffer + texCoordsOffset > MEMORY_BLOCK_SIZE)
			{
				VkDeviceSize size = MEMORY_BLOCK_SIZE;
				if (size < texCoordsOffset)
				{
					size = texCoordsOffset;
				}
				buffer = new Buffer();
				buffer->CreateVertexBuffer(appState, size);
				appState.Scene.colormappedBuffers.MoveToFront(buffer);
				appState.Scene.latestColormappedBuffer = buffer;
				appState.Scene.usedInLatestColormappedBuffer = 0;
			}
			else
			{
				buffer = appState.Scene.latestColormappedBuffer;
			}
			appState.Scene.colormappedTexCoordsSize += texCoordsOffset;
			VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, appState.Scene.usedInLatestColormappedBuffer, texCoordsOffset, 0, &buffer->mapped));
			auto mapped = (float*)buffer->mapped;
			for (auto i : appState.Scene.newTexCoords)
			{
				auto& alias = d_lists.alias[i];
				auto texCoords = alias.texture_coordinates;
				for (auto j = 0; j < alias.vertex_count; j++)
				{
					auto s = (float)(texCoords->s >> 16);
					auto t = (float)(texCoords->t >> 16);
					s /= alias.width;
					t /= alias.height;
					(*mapped) = s;
					mapped++;
					(*mapped) = t;
					mapped++;
					(*mapped) = s + 0.5;
					mapped++;
					(*mapped) = t;
					mapped++;
					texCoords++;
				}
				auto index = appState.Scene.aliasTexCoordsList[i];
				appState.Scene.colormappedBufferList[index].buffer = buffer;
				appState.Scene.colormappedBufferList[index].offset += appState.Scene.usedInLatestColormappedBuffer;
			}
			VC(appState.Device.vkUnmapMemory(appState.Device.device, buffer->memory));
			buffer->mapped = nullptr;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferMemoryBarrier.buffer = buffer->buffer;
			bufferMemoryBarrier.size = buffer->size;
			VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			appState.Scene.usedInLatestColormappedBuffer += texCoordsOffset;
		}
		if (d_lists.last_viewmodel >= appState.Scene.viewmodelVerticesList.size())
		{
			appState.Scene.viewmodelVerticesList.resize(d_lists.last_viewmodel + 1);
			appState.Scene.viewmodelTexCoordsList.resize(d_lists.last_viewmodel + 1);
		}
		appState.Scene.newVertices.clear();
		appState.Scene.newTexCoords.clear();
		verticesOffset = 0;
		texCoordsOffset = 0;
		for (auto i = 0; i <= d_lists.last_viewmodel; i++)
		{
			auto& viewmodel = d_lists.viewmodel[i];
			auto verticesEntry = appState.Scene.colormappedVerticesPerKey.find(viewmodel.vertices);
			if (verticesEntry == appState.Scene.colormappedVerticesPerKey.end())
			{
				auto lastIndex = appState.Scene.colormappedBufferList.size();
				appState.Scene.colormappedBufferList.push_back({ verticesOffset });
				auto newEntry = appState.Scene.colormappedVerticesPerKey.insert({ viewmodel.vertices, lastIndex });
				appState.Scene.newVertices.push_back(i);
				appState.Scene.viewmodelVerticesList[i] = lastIndex;
				verticesOffset += viewmodel.vertex_count * 2 * 4 * sizeof(float);
			}
			else
			{
				appState.Scene.viewmodelVerticesList[i] = verticesEntry->second;
			}
			auto texCoordsEntry = appState.Scene.colormappedTexCoordsPerKey.find(viewmodel.texture_coordinates);
			if (texCoordsEntry == appState.Scene.colormappedTexCoordsPerKey.end())
			{
				auto lastIndex = appState.Scene.colormappedBufferList.size();
				appState.Scene.colormappedBufferList.push_back({ texCoordsOffset });
				auto newEntry = appState.Scene.colormappedTexCoordsPerKey.insert({ viewmodel.texture_coordinates, lastIndex });
				appState.Scene.newTexCoords.push_back(i);
				appState.Scene.viewmodelTexCoordsList[i] = lastIndex;
				texCoordsOffset += viewmodel.vertex_count * 2 * 2 * sizeof(float);
			}
			else
			{
				appState.Scene.viewmodelTexCoordsList[i] = texCoordsEntry->second;
			}
		}
		if (verticesOffset > 0)
		{
			Buffer* buffer;
			if (appState.Scene.latestColormappedBuffer == nullptr || appState.Scene.usedInLatestColormappedBuffer + verticesOffset > MEMORY_BLOCK_SIZE)
			{
				VkDeviceSize size = MEMORY_BLOCK_SIZE;
				if (size < verticesOffset)
				{
					size = verticesOffset;
				}
				buffer = new Buffer();
				buffer->CreateVertexBuffer(appState, size);
				appState.Scene.colormappedBuffers.MoveToFront(buffer);
				appState.Scene.latestColormappedBuffer = buffer;
				appState.Scene.usedInLatestColormappedBuffer = 0;
			}
			else
			{
				buffer = appState.Scene.latestColormappedBuffer;
			}
			appState.Scene.colormappedVerticesSize += verticesOffset;
			VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, appState.Scene.usedInLatestColormappedBuffer, verticesOffset, 0, &buffer->mapped));
			auto mapped = (float*)buffer->mapped;
			for (auto i : appState.Scene.newVertices)
			{
				auto& viewmodel = d_lists.viewmodel[i];
				auto vertexFromModel = viewmodel.vertices;
				for (auto j = 0; j < viewmodel.vertex_count; j++)
				{
					auto x = (float)(vertexFromModel->v[0]);
					auto y = (float)(vertexFromModel->v[1]);
					auto z = (float)(vertexFromModel->v[2]);
					(*mapped) = x;
					mapped++;
					(*mapped) = z;
					mapped++;
					(*mapped) = -y;
					mapped++;
					(*mapped) = 1;
					mapped++;
					(*mapped) = x;
					mapped++;
					(*mapped) = z;
					mapped++;
					(*mapped) = -y;
					mapped++;
					(*mapped) = 1;
					mapped++;
					vertexFromModel++;
				}
				auto index = appState.Scene.viewmodelVerticesList[i];
				appState.Scene.colormappedBufferList[index].buffer = buffer;
				appState.Scene.colormappedBufferList[index].offset += appState.Scene.usedInLatestColormappedBuffer;
			}
			VC(appState.Device.vkUnmapMemory(appState.Device.device, buffer->memory));
			buffer->mapped = nullptr;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferMemoryBarrier.buffer = buffer->buffer;
			bufferMemoryBarrier.size = buffer->size;
			VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			appState.Scene.usedInLatestColormappedBuffer += verticesOffset;
		}
		if (texCoordsOffset > 0)
		{
			Buffer* buffer;
			if (appState.Scene.latestColormappedBuffer == nullptr || appState.Scene.usedInLatestColormappedBuffer + texCoordsOffset > MEMORY_BLOCK_SIZE)
			{
				VkDeviceSize size = MEMORY_BLOCK_SIZE;
				if (size < texCoordsOffset)
				{
					size = texCoordsOffset;
				}
				buffer = new Buffer();
				buffer->CreateVertexBuffer(appState, size);
				appState.Scene.colormappedBuffers.MoveToFront(buffer);
				appState.Scene.latestColormappedBuffer = buffer;
				appState.Scene.usedInLatestColormappedBuffer = 0;
			}
			else
			{
				buffer = appState.Scene.latestColormappedBuffer;
			}
			appState.Scene.colormappedTexCoordsSize += texCoordsOffset;
			VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, appState.Scene.usedInLatestColormappedBuffer, texCoordsOffset, 0, &buffer->mapped));
			auto mapped = (float*)buffer->mapped;
			for (auto i : appState.Scene.newTexCoords)
			{
				auto& viewmodel = d_lists.viewmodel[i];
				auto texCoords = viewmodel.texture_coordinates;
				for (auto j = 0; j < viewmodel.vertex_count; j++)
				{
					auto s = (float)(texCoords->s >> 16);
					auto t = (float)(texCoords->t >> 16);
					s /= viewmodel.width;
					t /= viewmodel.height;
					(*mapped) = s;
					mapped++;
					(*mapped) = t;
					mapped++;
					(*mapped) = s + 0.5;
					mapped++;
					(*mapped) = t;
					mapped++;
					texCoords++;
				}
				auto index = appState.Scene.viewmodelTexCoordsList[i];
				appState.Scene.colormappedBufferList[index].buffer = buffer;
				appState.Scene.colormappedBufferList[index].offset += appState.Scene.usedInLatestColormappedBuffer;
			}
			VC(appState.Device.vkUnmapMemory(appState.Device.device, buffer->memory));
			buffer->mapped = nullptr;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferMemoryBarrier.buffer = buffer->buffer;
			bufferMemoryBarrier.size = buffer->size;
			VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			appState.Scene.usedInLatestColormappedBuffer += texCoordsOffset;
		}
		if (appState.Mode != AppWorldMode)
		{
			appState.Scene.floorAttributesSize = 2 * 4 * sizeof(float);
		}
		appState.Scene.texturedAttributesSize = (d_lists.last_textured_attribute + 1) * sizeof(float);
		appState.Scene.colormappedLightsSize = (d_lists.last_colormapped_attribute + 1) * sizeof(float);
		appState.Scene.vertexTransformSize = 16 * sizeof(float);
		appState.Scene.attributesSize = appState.Scene.floorAttributesSize + appState.Scene.texturedAttributesSize + appState.Scene.colormappedLightsSize + appState.Scene.vertexTransformSize;
		attributes = cachedAttributes.GetVertexBuffer(appState, appState.Scene.attributesSize);
		VK(appState.Device.vkMapMemory(appState.Device.device, attributes->memory, 0, appState.Scene.attributesSize, 0, &attributes->mapped));
		if (appState.Scene.floorAttributesSize > 0)
		{
			auto mapped = (float*)attributes->mapped;
			(*mapped) = 0;
			mapped++;
			(*mapped) = 0;
			mapped++;
			(*mapped) = 1;
			mapped++;
			(*mapped) = 0;
			mapped++;
			(*mapped) = 1;
			mapped++;
			(*mapped) = 1;
			mapped++;
			(*mapped) = 0;
			mapped++;
			(*mapped) = 1;
		}
		texturedAttributeBase = appState.Scene.floorAttributesSize;
		memcpy((unsigned char*)attributes->mapped + texturedAttributeBase, d_lists.textured_attributes.data(), appState.Scene.texturedAttributesSize);
		colormappedAttributeBase = texturedAttributeBase + appState.Scene.texturedAttributesSize;
		memcpy((unsigned char*)attributes->mapped + colormappedAttributeBase, d_lists.colormapped_attributes.data(), appState.Scene.colormappedLightsSize);
		vertexTransformBase = colormappedAttributeBase + appState.Scene.colormappedLightsSize;
		auto mapped = (float*)attributes->mapped + vertexTransformBase / sizeof(float);
		(*mapped) = appState.Scale;
		mapped++;
		(*mapped) = 0;
		mapped++;
		(*mapped) = 0;
		mapped++;
		(*mapped) = 0;
		mapped++;
		(*mapped) = 0;
		mapped++;
		(*mapped) = appState.Scale;
		mapped++;
		(*mapped) = 0;
		mapped++;
		(*mapped) = 0;
		mapped++;
		(*mapped) = 0;
		mapped++;
		(*mapped) = 0;
		mapped++;
		(*mapped) = appState.Scale;
		mapped++;
		(*mapped) = 0;
		mapped++;
		(*mapped) = -r_refdef.vieworg[0] * appState.Scale;
		mapped++;
		(*mapped) = -r_refdef.vieworg[2] * appState.Scale;
		mapped++;
		(*mapped) = r_refdef.vieworg[1] * appState.Scale;
		mapped++;
		(*mapped) = 1;
		VC(appState.Device.vkUnmapMemory(appState.Device.device, attributes->memory));
		attributes->mapped = nullptr;
		bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		bufferMemoryBarrier.buffer = attributes->buffer;
		bufferMemoryBarrier.size = attributes->size;
		VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
		if (appState.Mode != AppWorldMode)
		{
			appState.Scene.floorIndicesSize = 6 * sizeof(uint16_t);
		}
		appState.Scene.colormappedIndices16Size = (d_lists.last_colormapped_index16 + 1) * sizeof(uint16_t);
		appState.Scene.coloredIndices16Size = (d_lists.last_colored_index16 + 1) * sizeof(uint16_t);
		appState.Scene.indices16Size = appState.Scene.floorIndicesSize + appState.Scene.colormappedIndices16Size + appState.Scene.coloredIndices16Size;
		if (appState.Scene.indices16Size > 0)
		{
			indices16 = cachedIndices16.GetIndexBuffer(appState, appState.Scene.indices16Size);
			VK(appState.Device.vkMapMemory(appState.Device.device, indices16->memory, 0, appState.Scene.indices16Size, 0, &indices16->mapped));
			if (appState.Scene.floorIndicesSize > 0)
			{
				auto mapped = (uint16_t*)indices16->mapped;
				(*mapped) = 0;
				mapped++;
				(*mapped) = 1;
				mapped++;
				(*mapped) = 2;
				mapped++;
				(*mapped) = 2;
				mapped++;
				(*mapped) = 3;
				mapped++;
				(*mapped) = 0;
			}
			colormappedIndex16Base = appState.Scene.floorIndicesSize;
			memcpy((unsigned char*)indices16->mapped + colormappedIndex16Base, d_lists.colormapped_indices16.data(), appState.Scene.colormappedIndices16Size);
			coloredIndex16Base = colormappedIndex16Base + appState.Scene.colormappedIndices16Size;
			memcpy((unsigned char*)indices16->mapped + coloredIndex16Base, d_lists.colored_indices16.data(), appState.Scene.coloredIndices16Size);
			VC(appState.Device.vkUnmapMemory(appState.Device.device, indices16->memory));
			indices16->mapped = nullptr;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
			bufferMemoryBarrier.buffer = indices16->buffer;
			bufferMemoryBarrier.size = indices16->size;
			VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
		}
		appState.Scene.colormappedIndices32Size = (d_lists.last_colormapped_index32 + 1) * sizeof(uint32_t);
		appState.Scene.coloredIndices32Size = (d_lists.last_colored_index32 + 1) * sizeof(uint32_t);
		appState.Scene.indices32Size = appState.Scene.colormappedIndices32Size + appState.Scene.coloredIndices32Size;
		if (appState.Scene.indices32Size > 0)
		{
			indices32 = cachedIndices32.GetIndexBuffer(appState, appState.Scene.indices32Size);
			VK(appState.Device.vkMapMemory(appState.Device.device, indices32->memory, 0, appState.Scene.indices32Size, 0, &indices32->mapped));
			memcpy(indices32->mapped, d_lists.colormapped_indices32.data(), appState.Scene.colormappedIndices32Size);
			coloredIndex32Base = appState.Scene.colormappedIndices32Size;
			memcpy((unsigned char*)indices32->mapped + coloredIndex32Base, d_lists.colored_indices32.data(), appState.Scene.coloredIndices32Size);
			VC(appState.Device.vkUnmapMemory(appState.Device.device, indices32->memory));
			indices32->mapped = nullptr;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
			bufferMemoryBarrier.buffer = indices32->buffer;
			bufferMemoryBarrier.size = indices32->size;
			VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
		}
		for (auto i = 0; i <= d_lists.last_colored_surfaces_index16; i++)
		{
			appState.Scene.coloredSurfaces16Size += (d_lists.colored_surfaces_index16[i].last_color + 1) * sizeof(float);
		}
		for (auto i = 0; i <= d_lists.last_colored_surfaces_index32; i++)
		{
			appState.Scene.coloredSurfaces32Size += (d_lists.colored_surfaces_index32[i].last_color + 1) * sizeof(float);
		}
		for (auto i = 0; i <= d_lists.last_particles_index16; i++)
		{
			appState.Scene.particles16Size += (d_lists.particles_index16[i].last_color + 1) * sizeof(float);
		}
		for (auto i = 0; i <= d_lists.last_particles_index32; i++)
		{
			appState.Scene.particles32Size += (d_lists.particles_index32[i].last_color + 1) * sizeof(float);
		}
		appState.Scene.colorsSize = appState.Scene.coloredSurfaces16Size + appState.Scene.coloredSurfaces32Size + appState.Scene.particles16Size + appState.Scene.particles32Size;
		if (appState.Scene.colorsSize > 0)
		{
			colors = cachedColors.GetVertexBuffer(appState, appState.Scene.colorsSize);
			VK(appState.Device.vkMapMemory(appState.Device.device, colors->memory, 0, appState.Scene.colorsSize, 0, &colors->mapped));
			auto offset = 0;
			for (auto i = 0; i <= d_lists.last_colored_surfaces_index16; i++)
			{
				auto& coloredSurfaces = d_lists.colored_surfaces_index16[i];
				auto count = (coloredSurfaces.last_color + 1) * sizeof(float);
				memcpy((unsigned char*)colors->mapped + offset, coloredSurfaces.colors.data(), count);
				offset += count;
			}
			for (auto i = 0; i <= d_lists.last_colored_surfaces_index32; i++)
			{
				auto& coloredSurfaces = d_lists.colored_surfaces_index32[i];
				auto count = (coloredSurfaces.last_color + 1) * sizeof(float);
				memcpy((unsigned char*)colors->mapped + offset, coloredSurfaces.colors.data(), count);
				offset += count;
			}
			for (auto i = 0; i <= d_lists.last_particles_index16; i++)
			{
				auto& particles = d_lists.particles_index16[i];
				auto count = (particles.last_color + 1) * sizeof(float);
				memcpy((unsigned char*)colors->mapped + offset, particles.colors.data(), count);
				offset += count;
			}
			for (auto i = 0; i <= d_lists.last_particles_index32; i++)
			{
				auto& particles = d_lists.particles_index32[i];
				auto count = (particles.last_color + 1) * sizeof(float);
				memcpy((unsigned char*)colors->mapped + offset, particles.colors.data(), count);
				offset += count;
			}
			VC(appState.Device.vkUnmapMemory(appState.Device.device, colors->memory));
			colors->mapped = nullptr;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
			bufferMemoryBarrier.buffer = colors->buffer;
			bufferMemoryBarrier.size = colors->size;
			VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
		}
	}
}

void PerImage::GetStagingBufferSize(AppState& appState, View& view, VkDeviceSize& stagingBufferSize, VkDeviceSize& floorSize)
{
	paletteOffset = -1;
	if (palette == nullptr || paletteChanged != pal_changed)
	{
		if (palette == nullptr)
		{
			palette = new Texture();
			palette->Create(appState, commandBuffer, 256, 1, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		paletteOffset = stagingBufferSize;
		stagingBufferSize += 1024;
		paletteChanged = pal_changed;
	}
	host_colormapOffset = -1;
	if (::host_colormap.size() > 0 && host_colormap == nullptr)
	{
		host_colormap = new Texture();
		host_colormap->Create(appState, commandBuffer, 256, 64, VK_FORMAT_R8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		host_colormapOffset = stagingBufferSize;
		stagingBufferSize += 16384;
	}
	if (d_lists.last_surface >= appState.Scene.surfaceList.size())
	{
		appState.Scene.surfaceList.resize(d_lists.last_surface + 1);
	}
	for (auto i = 0; i <= d_lists.last_surface; i++)
	{
		auto& surface = d_lists.surfaces[i];
		auto entry = appState.Scene.surfaces.find({ surface.surface, surface.entity });
		if (entry == appState.Scene.surfaces.end())
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
			auto texture = new TextureFromAllocation();
			texture->Create(appState, commandBuffer, surface.width, surface.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			texture->key.first = surface.surface;
			texture->key.second = surface.entity;
			appState.Scene.surfaces.insert({ texture->key, texture });
			appState.Scene.surfaceList[i].texture = texture;
			appState.Scene.surfaceList[i].offset = stagingBufferSize;
			stagingBufferSize += surface.size;
		}
		else if (surface.created)
		{
			auto first = entry->second;
			auto available = first->unusedCount >= view.framebuffer.swapChainLength;
			if (first->next == nullptr || available)
			{
				if (available)
				{
					first->unusedCount = 0;
					appState.Scene.surfaceList[i].texture = first;
					appState.Scene.surfaceList[i].offset = stagingBufferSize;
					stagingBufferSize += surface.size;
				}
				else
				{
					auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
					auto texture = new TextureFromAllocation();
					texture->Create(appState, commandBuffer, surface.width, surface.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
					texture->key.first = surface.surface;
					texture->key.second = surface.entity;
					texture->next = first;
					entry->second = texture;
					appState.Scene.surfaceList[i].texture = texture;
					appState.Scene.surfaceList[i].offset = stagingBufferSize;
					stagingBufferSize += surface.size;
				}
			}
			else
			{
				auto found = false;
				for (auto previous = first, texture = first->next; texture != nullptr; previous = texture, texture = texture->next)
				{
					if (texture->unusedCount >= view.framebuffer.swapChainLength)
					{
						found = true;
						texture->unusedCount = 0;
						appState.Scene.surfaceList[i].texture = texture;
						appState.Scene.surfaceList[i].offset = stagingBufferSize;
						stagingBufferSize += surface.size;
						previous->next = texture->next;
						texture->next = first;
						entry->second = texture;
						break;
					}
				}
				if (!found)
				{
					auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
					auto texture = new TextureFromAllocation();
					texture->Create(appState, commandBuffer, surface.width, surface.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
					texture->key.first = surface.surface;
					texture->key.second = surface.entity;
					texture->next = entry->second;
					entry->second = texture;
					appState.Scene.surfaceList[i].texture = texture;
					appState.Scene.surfaceList[i].offset = stagingBufferSize;
					stagingBufferSize += surface.size;
				}
			}
		}
		else
		{
			auto texture = entry->second;
			texture->unusedCount = 0;
			appState.Scene.surfaceList[i].texture = texture;
			appState.Scene.surfaceList[i].offset = -1;
		}
	}
	if (d_lists.last_sprite >= appState.Scene.spriteList.size())
	{
		appState.Scene.spriteList.resize(d_lists.last_sprite + 1);
	}
	for (auto i = 0; i <= d_lists.last_sprite; i++)
	{
		auto& sprite = d_lists.sprites[i];
		auto entry = appState.Scene.spritesPerKey.find(sprite.data);
		if (entry == appState.Scene.spritesPerKey.end())
		{
			SharedMemoryTexture* texture;
			auto mipCount = (int)(std::floor(std::log2(std::max(sprite.width, sprite.height)))) + 1;
			texture = new SharedMemoryTexture();
			texture->Create(appState, commandBuffer, sprite.width, sprite.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			appState.Scene.spriteList[i].offset = stagingBufferSize;
			stagingBufferSize += sprite.size;
			appState.Scene.spriteTextures.MoveToFront(texture);
			appState.Scene.spriteTextureCount++;
			appState.Scene.spritesPerKey.insert({ sprite.data, texture });
			appState.Scene.spriteList[i].texture = texture;
		}
		else
		{
			appState.Scene.spriteList[i].offset = -1;
			appState.Scene.spriteList[i].texture = entry->second;
		}
	}
	if (d_lists.last_turbulent >= appState.Scene.turbulentList.size())
	{
		appState.Scene.turbulentList.resize(d_lists.last_turbulent + 1);
	}
	appState.Scene.turbulentPerKey.clear();
	for (auto i = 0; i <= d_lists.last_turbulent; i++)
	{
		auto& turbulent = d_lists.turbulent[i];
		auto entry = appState.Scene.turbulentPerKey.find(turbulent.texture);
		if (hostClearCount == host_clearcount && entry != appState.Scene.turbulentPerKey.end())
		{
			appState.Scene.turbulentList[i].offset = -1;
			appState.Scene.turbulentList[i].texture = entry->second;
			entry->second->unusedCount = 0;
		}
		else if (hostClearCount == host_clearcount)
		{
			Texture* texture = nullptr;
			for (Texture** t = &this->turbulent.oldTextures; *t != nullptr; t = &(*t)->next)
			{
				if ((*t)->key == turbulent.texture)
				{
					texture = *t;
					*t = (*t)->next;
					break;
				}
			}
			if (texture == nullptr)
			{
				auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
				texture = new Texture();
				texture->Create(appState, commandBuffer, turbulent.width, turbulent.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
				texture->key = turbulent.texture;
				appState.Scene.turbulentList[i].offset = stagingBufferSize;
				stagingBufferSize += turbulent.size;
			}
			else
			{
				appState.Scene.turbulentList[i].offset = -1;
			}
			appState.Scene.turbulentList[i].texture = texture;
			appState.Scene.turbulentPerKey.insert({ texture->key, texture });
			this->turbulent.MoveToFront(texture);
		}
		else
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
			auto texture = new Texture();
			texture->Create(appState, commandBuffer, turbulent.width, turbulent.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			texture->key = turbulent.texture;
			appState.Scene.turbulentList[i].offset = stagingBufferSize;
			stagingBufferSize += turbulent.size;
			this->turbulent.MoveToFront(texture);
			appState.Scene.turbulentList[i].texture = texture;
			appState.Scene.turbulentPerKey.insert({ texture->key, texture });
		}
	}
	if (d_lists.last_alias >= appState.Scene.aliasList.size())
	{
		appState.Scene.aliasList.resize(d_lists.last_alias + 1);
	}
	for (auto i = 0; i <= d_lists.last_alias; i++)
	{
		auto& alias = d_lists.alias[i];
		auto entry = appState.Scene.aliasPerKey.find(alias.data);
		if (entry == appState.Scene.aliasPerKey.end())
		{
			SharedMemoryTexture* texture;
			auto mipCount = (int)(std::floor(std::log2(std::max(alias.width, alias.height)))) + 1;
			texture = new SharedMemoryTexture();
			texture->Create(appState, commandBuffer, alias.width, alias.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			appState.Scene.aliasList[i].texture.offset = stagingBufferSize;
			stagingBufferSize += alias.size;
			appState.Scene.aliasTextures.MoveToFront(texture);
			appState.Scene.aliasTextureCount++;
			appState.Scene.aliasPerKey.insert({ alias.data, texture });
			appState.Scene.aliasList[i].texture.texture = texture;
		}
		else
		{
			appState.Scene.aliasList[i].texture.offset = -1;
			appState.Scene.aliasList[i].texture.texture = entry->second;
		}
	}
	for (auto i = 0; i <= d_lists.last_alias; i++)
	{
		auto& alias = d_lists.alias[i];
		if (alias.is_host_colormap)
		{
			appState.Scene.aliasList[i].colormap.offset = -1;
			appState.Scene.aliasList[i].colormap.texture = host_colormap;
		}
		else
		{
			appState.Scene.aliasList[i].colormap.offset = stagingBufferSize;
			stagingBufferSize += 16384;
		}
	}
	if (d_lists.last_viewmodel >= appState.Scene.viewmodelList.size())
	{
		appState.Scene.viewmodelList.resize(d_lists.last_viewmodel + 1);
	}
	for (auto i = 0; i <= d_lists.last_viewmodel; i++)
	{
		auto& viewmodel = d_lists.viewmodel[i];
		auto entry = appState.Scene.viewmodelsPerKey.find(viewmodel.data);
		if (entry == appState.Scene.viewmodelsPerKey.end())
		{
			SharedMemoryTexture* texture;
			auto mipCount = (int)(std::floor(std::log2(std::max(viewmodel.width, viewmodel.height)))) + 1;
			texture = new SharedMemoryTexture();
			texture->Create(appState, commandBuffer, viewmodel.width, viewmodel.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			appState.Scene.viewmodelList[i].texture.offset = stagingBufferSize;
			stagingBufferSize += viewmodel.size;
			appState.Scene.viewmodelTextures.MoveToFront(texture);
			appState.Scene.viewmodelTextureCount++;
			appState.Scene.viewmodelsPerKey.insert({ viewmodel.data, texture });
			appState.Scene.viewmodelList[i].texture.texture = texture;
		}
		else
		{
			appState.Scene.viewmodelList[i].texture.offset = -1;
			appState.Scene.viewmodelList[i].texture.texture = entry->second;
		}
	}
	for (auto i = 0; i <= d_lists.last_viewmodel; i++)
	{
		auto& viewmodel = d_lists.viewmodel[i];
		if (viewmodel.is_host_colormap)
		{
			appState.Scene.viewmodelList[i].colormap.offset = -1;
			appState.Scene.viewmodelList[i].colormap.texture = host_colormap;
		}
		else
		{
			appState.Scene.viewmodelList[i].colormap.offset = stagingBufferSize;
			stagingBufferSize += 16384;
		}
	}
	if (d_lists.last_sky >= 0)
	{
		skyOffset = stagingBufferSize;
		stagingBufferSize += 16384;
	}
	else
	{
		skyOffset = -1;
	}
	if (appState.Scene.floorTexture == nullptr)
	{
		appState.Scene.floorOffset = stagingBufferSize;
		floorSize = appState.FloorWidth * appState.FloorHeight * sizeof(uint32_t);
		stagingBufferSize += floorSize;
	}
	else
	{
		appState.Scene.floorOffset = -1;
	}
}

void PerImage::LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer, VkDeviceSize stagingBufferSize, int floorSize)
{
	VK(appState.Device.vkMapMemory(appState.Device.device, stagingBuffer->memory, 0, stagingBufferSize, 0, &stagingBuffer->mapped));
	auto offset = 0;
	if (paletteOffset >= 0)
	{
		memcpy(stagingBuffer->mapped, d_8to24table, 1024);
		offset += 1024;
	}
	if (host_colormapOffset >= 0)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, ::host_colormap.data(), 16384);
		offset += 16384;
	}
	for (auto i = 0; i <= d_lists.last_surface; i++)
	{
		if (appState.Scene.surfaceList[i].offset >= 0)
		{
			auto& surface = d_lists.surfaces[i];
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, surface.data.data(), surface.size);
			offset += surface.size;
		}
	}
	for (auto i = 0; i <= d_lists.last_sprite; i++)
	{
		if (appState.Scene.spriteList[i].offset >= 0)
		{
			auto& sprite = d_lists.sprites[i];
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, sprite.data, sprite.size);
			offset += sprite.size;
		}
	}
	for (auto i = 0; i <= d_lists.last_turbulent; i++)
	{
		if (appState.Scene.turbulentList[i].offset >= 0)
		{
			auto& turbulent = d_lists.turbulent[i];
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, turbulent.data, turbulent.size);
			offset += turbulent.size;
		}
	}
	for (auto i = 0; i <= d_lists.last_alias; i++)
	{
		auto& alias = d_lists.alias[i];
		if (appState.Scene.aliasList[i].texture.offset >= 0)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.data, alias.size);
			offset += alias.size;
		}
		if (!alias.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.colormap.data(), 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= d_lists.last_viewmodel; i++)
	{
		auto& viewmodel = d_lists.viewmodel[i];
		if (appState.Scene.viewmodelList[i].texture.offset >= 0)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.data, viewmodel.size);
			offset += viewmodel.size;
		}
		if (!viewmodel.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.colormap.data(), 16384);
			offset += 16384;
		}
	}
	if (d_lists.last_sky >= 0)
	{
		auto source = r_skysource;
		auto target = ((unsigned char*)stagingBuffer->mapped) + offset;
		for (auto i = 0; i < 128; i++)
		{
			memcpy(target, source, 128);
			source += 256;
			target += 128;
		}
		offset += 16384;
	}
	if (appState.Scene.floorOffset >= 0)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, appState.FloorData.data(), floorSize);
	}
	VkMappedMemoryRange mappedMemoryRange { };
	mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	mappedMemoryRange.memory = stagingBuffer->memory;
	VC(appState.Device.vkFlushMappedMemoryRanges(appState.Device.device, 1, &mappedMemoryRange));
	VC(appState.Device.vkUnmapMemory(appState.Device.device, stagingBuffer->memory));
	stagingBuffer->mapped = nullptr;
}

void PerImage::FillTextures(AppState &appState, Buffer* stagingBuffer)
{
	if (paletteOffset >= 0)
	{
		palette->Fill(appState, stagingBuffer, paletteOffset, commandBuffer);
	}
	if (host_colormapOffset >= 0)
	{
		host_colormap->Fill(appState, stagingBuffer, host_colormapOffset, commandBuffer);
	}
	for (auto i = 0; i <= d_lists.last_surface; i++)
	{
		if (appState.Scene.surfaceList[i].offset >= 0)
		{
			appState.Scene.surfaceList[i].texture->FillMipmapped(appState, stagingBuffer, appState.Scene.surfaceList[i].offset, commandBuffer);
		}
	}
	for (auto i = 0; i <= d_lists.last_sprite; i++)
	{
		if (appState.Scene.spriteList[i].offset >= 0)
		{
			appState.Scene.spriteList[i].texture->FillMipmapped(appState, stagingBuffer, appState.Scene.spriteList[i].offset, commandBuffer);
		}
	}
	for (auto i = 0; i <= d_lists.last_turbulent; i++)
	{
		if (appState.Scene.turbulentList[i].offset >= 0)
		{
			appState.Scene.turbulentList[i].texture->FillMipmapped(appState, stagingBuffer, appState.Scene.turbulentList[i].offset, commandBuffer);
		}
	}
	for (auto i = 0; i <= d_lists.last_alias; i++)
	{
		auto& alias = d_lists.alias[i];
		if (appState.Scene.aliasList[i].texture.offset >= 0)
		{
			appState.Scene.aliasList[i].texture.texture->FillMipmapped(appState, stagingBuffer, appState.Scene.aliasList[i].texture.offset, commandBuffer);
		}
		if (!alias.is_host_colormap)
		{
			Texture* texture;
			if (colormaps.oldTextures != nullptr)
			{
				texture = colormaps.oldTextures;
				colormaps.oldTextures = texture->next;
			}
			else
			{
				texture = new Texture();
				texture->Create(appState, commandBuffer, 256, 64, VK_FORMAT_R8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			}
			texture->Fill(appState, stagingBuffer, appState.Scene.aliasList[i].colormap.offset, commandBuffer);
			colormaps.MoveToFront(texture);
			appState.Scene.aliasList[i].colormap.texture = texture;
			colormapCount++;
		}
	}
	for (auto i = 0; i <= d_lists.last_viewmodel; i++)
	{
		auto& viewmodel = d_lists.viewmodel[i];
		if (appState.Scene.viewmodelList[i].texture.offset >= 0)
		{
			appState.Scene.viewmodelList[i].texture.texture->FillMipmapped(appState, stagingBuffer, appState.Scene.viewmodelList[i].texture.offset, commandBuffer);
		}
		if (!viewmodel.is_host_colormap)
		{
			Texture* texture;
			if (colormaps.oldTextures != nullptr)
			{
				texture = colormaps.oldTextures;
				colormaps.oldTextures = texture->next;
			}
			else
			{
				texture = new Texture();
				texture->Create(appState, commandBuffer, 256, 64, VK_FORMAT_R8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			}
			texture->Fill(appState, stagingBuffer, appState.Scene.viewmodelList[i].colormap.offset, commandBuffer);
			colormaps.MoveToFront(texture);
			appState.Scene.viewmodelList[i].colormap.texture = texture;
			colormapCount++;
		}
	}
	if (d_lists.last_sky >= 0)
	{
		if (sky == nullptr)
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(128, 128)))) + 1;
			sky = new Texture();
			sky->Create(appState, commandBuffer, 128, 128, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		sky->FillMipmapped(appState, stagingBuffer, skyOffset, commandBuffer);
	}
	if (appState.Mode != AppWorldMode && appState.Scene.floorTexture == nullptr)
	{
		appState.Scene.floorTexture = new Texture();
		appState.Scene.floorTexture->Create(appState, commandBuffer, appState.FloorWidth, appState.FloorHeight, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		appState.Scene.floorTexture->Fill(appState, stagingBuffer, appState.Scene.floorOffset, commandBuffer);
	}
}

void PerImage::Render(AppState& appState)
{
	if (appState.Scene.verticesSize == 0)
	{
		return;
	}
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
	if (!host_colormapResources.created && host_colormap != nullptr)
	{
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = 1;
		textureInfo[0].sampler = appState.Scene.samplers[host_colormap->mipCount];
		textureInfo[0].imageView = host_colormap->view;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo = textureInfo;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &host_colormapResources.descriptorPool));
		descriptorSetAllocateInfo.descriptorPool = host_colormapResources.descriptorPool;
		descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
		VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &host_colormapResources.descriptorSet));
		writes[0].dstSet = host_colormapResources.descriptorSet;
		VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
		host_colormapResources.created = true;
	}
	if (!sceneMatricesResources.created || !sceneMatricesAndPaletteResources.created)
	{
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 1;
		VkDescriptorBufferInfo bufferInfo { };
		bufferInfo.buffer = appState.Scene.matrices.buffer;
		bufferInfo.range = appState.Scene.matrices.size;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].pBufferInfo = &bufferInfo;
		if (!sceneMatricesResources.created)
		{
			descriptorPoolCreateInfo.poolSizeCount = 1;
			VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = sceneMatricesResources.descriptorPool;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleBufferLayout;
			VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &sceneMatricesResources.descriptorSet));
			writes[0].dstSet = sceneMatricesResources.descriptorSet;
			VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
			sceneMatricesResources.created = true;
		}
		if (!sceneMatricesAndPaletteResources.created)
		{
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[1].descriptorCount = 1;
			textureInfo[0].sampler = appState.Scene.samplers[palette->mipCount];
			textureInfo[0].imageView = palette->view;
			writes[1].dstBinding = 1;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[1].pImageInfo = textureInfo;
			descriptorPoolCreateInfo.poolSizeCount = 2;
			VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesAndPaletteResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = sceneMatricesAndPaletteResources.descriptorPool;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.bufferAndImageLayout;
			VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &sceneMatricesAndPaletteResources.descriptorSet));
			writes[0].dstSet = sceneMatricesAndPaletteResources.descriptorSet;
			writes[1].dstSet = sceneMatricesAndPaletteResources.descriptorSet;
			VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 2, writes, 0, nullptr));
			sceneMatricesAndPaletteResources.created = true;
		}
	}
	if (appState.Mode == AppWorldMode)
	{
		VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &texturedVertexBase));
		VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &texturedAttributeBase));
		VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &vertexTransformBase));
		VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.textured.pipeline));
		VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.textured.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = 1;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo = textureInfo;
		if (d_lists.last_surface < 0 && d_lists.last_turbulent < 0)
		{
			texturedResources.Delete(appState);
		}
		else
		{
			auto size = texturedResources.descriptorSets.size();
			auto required = d_lists.last_surface + 1 + d_lists.last_turbulent + 1;
			if (size < required || size > required * 2)
			{
				auto toCreate = std::max(16, required + required / 4);
				if (toCreate != size)
				{
					appState.Scene.texturedDescriptorSetCount = toCreate;
					if (texturedResources.created)
					{
						VC(appState.Device.vkDestroyDescriptorPool(appState.Device.device, texturedResources.descriptorPool, nullptr));
					}
					texturedResources.bound.clear();
					poolSizes[0].descriptorCount = toCreate;
					descriptorPoolCreateInfo.maxSets = toCreate;
					VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &texturedResources.descriptorPool));
					descriptorSetAllocateInfo.descriptorPool = texturedResources.descriptorPool;
					descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
					texturedResources.descriptorSets.resize(toCreate);
					texturedResources.bound.resize(toCreate);
					for (auto i = 0; i < toCreate; i++)
					{
						VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &texturedResources.descriptorSets[i]));
					}
					texturedResources.created = true;
				}
			}
		}
		auto descriptorSetIndex = 0;
		float pushConstants[24];
		for (auto i = 0; i <= d_lists.last_surface; i++)
		{
			auto& surface = d_lists.surfaces[i];
			pushConstants[0] = surface.origin_x;
			pushConstants[1] = surface.origin_z;
			pushConstants[2] = -surface.origin_y;
			VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.textured.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 3 * sizeof(float), pushConstants));
			auto texture = appState.Scene.surfaceList[i].texture;
			if (texturedResources.bound[descriptorSetIndex] != texture)
			{
				textureInfo[0].sampler = appState.Scene.samplers[texture->mipCount];
				textureInfo[0].imageView = texture->view;
				writes[0].dstSet = texturedResources.descriptorSets[descriptorSetIndex];
				VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
				texturedResources.bound[descriptorSetIndex] = texture;
			}
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.textured.pipelineLayout, 1, 1, &texturedResources.descriptorSets[descriptorSetIndex], 0, nullptr));
			descriptorSetIndex++;
			VC(appState.Device.vkCmdDraw(commandBuffer, surface.count, 1, surface.first_vertex, 0));
		}
		VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipeline));
		VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
		if (resetDescriptorSetsCount != appState.Scene.resetDescriptorSetsCount || spriteResources.descriptorSets.size() < appState.Scene.spriteTextureCount)
		{
			spriteResources.Delete(appState);
			appState.Scene.spriteDescriptorSetCount = appState.Scene.spriteTextureCount;
			if (appState.Scene.spriteDescriptorSetCount > 0)
			{
				poolSizes[0].descriptorCount = appState.Scene.spriteDescriptorSetCount;
				descriptorPoolCreateInfo.maxSets = appState.Scene.spriteDescriptorSetCount;
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &spriteResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = spriteResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
				spriteResources.descriptorSets.resize(appState.Scene.spriteDescriptorSetCount);
				for (auto i = 0; i < appState.Scene.spriteDescriptorSetCount; i++)
				{
					VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &spriteResources.descriptorSets[i]));
				}
				spriteResources.created = true;
			}
		}
		for (auto i = 0; i <= d_lists.last_sprite; i++)
		{
			auto& sprite = d_lists.sprites[i];
			VkDescriptorSet descriptorSet;
			auto texture = appState.Scene.spriteList[i].texture;
			auto entry = spriteResources.cache.find(texture);
			if (entry == spriteResources.cache.end())
			{
				descriptorSet = spriteResources.descriptorSets[spriteResources.index];
				spriteResources.index++;
				textureInfo[0].sampler = appState.Scene.samplers[texture->mipCount];
				textureInfo[0].imageView = texture->view;
				writes[0].dstSet = descriptorSet;
				VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
				spriteResources.cache.insert({ texture, descriptorSet });
			}
			else
			{
				descriptorSet = entry->second;
			}
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
			VC(appState.Device.vkCmdDraw(commandBuffer, sprite.count, 1, sprite.first_vertex, 0));
		}
		VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipeline));
		VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
		pushConstants[3] = (float)cl.time;
		for (auto i = 0; i <= d_lists.last_turbulent; i++)
		{
			auto& turbulent = d_lists.turbulent[i];
			pushConstants[0] = turbulent.origin_x;
			pushConstants[1] = turbulent.origin_z;
			pushConstants[2] = -turbulent.origin_y;
			VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.turbulent.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), pushConstants));
			auto texture = appState.Scene.turbulentList[i].texture;
			if (texturedResources.bound[descriptorSetIndex] != texture)
			{
				textureInfo[0].sampler = appState.Scene.samplers[texture->mipCount];
				textureInfo[0].imageView = texture->view;
				writes[0].dstSet = texturedResources.descriptorSets[descriptorSetIndex];
				VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
				texturedResources.bound[descriptorSetIndex] = texture;
			}
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 1, 1, &texturedResources.descriptorSets[descriptorSetIndex], 0, nullptr));
			descriptorSetIndex++;
			VC(appState.Device.vkCmdDraw(commandBuffer, turbulent.count, 1, turbulent.first_vertex, 0));
		}
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
					appState.Scene.colormapDescriptorSetCount = toCreate;
					if (colormapResources.created)
					{
						VC(appState.Device.vkDestroyDescriptorPool(appState.Device.device, colormapResources.descriptorPool, nullptr));
					}
					texturedResources.bound.clear();
					poolSizes[0].descriptorCount = toCreate;
					descriptorPoolCreateInfo.maxSets = toCreate;
					VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &colormapResources.descriptorPool));
					descriptorSetAllocateInfo.descriptorPool = colormapResources.descriptorPool;
					descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
					colormapResources.descriptorSets.resize(toCreate);
					texturedResources.bound.resize(toCreate);
					for (auto i = 0; i < toCreate; i++)
					{
						VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &colormapResources.descriptorSets[i]));
					}
					colormapResources.created = true;
				}
			}
		}
		if (resetDescriptorSetsCount != appState.Scene.resetDescriptorSetsCount || aliasResources.descriptorSets.size() < appState.Scene.aliasTextureCount)
		{
			aliasResources.Delete(appState);
			appState.Scene.aliasDescriptorSetCount = appState.Scene.aliasTextureCount;
			if (appState.Scene.aliasDescriptorSetCount > 0)
			{
				poolSizes[0].descriptorCount = appState.Scene.aliasDescriptorSetCount;
				descriptorPoolCreateInfo.maxSets = appState.Scene.aliasDescriptorSetCount;
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &aliasResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = aliasResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
				aliasResources.descriptorSets.resize(appState.Scene.aliasDescriptorSetCount);
				for (auto i = 0; i < appState.Scene.aliasDescriptorSetCount; i++)
				{
					VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &aliasResources.descriptorSets[i]));
				}
				aliasResources.created = true;
			}
		}
		descriptorSetIndex = 0;
		if (d_lists.last_alias >= 0)
		{
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 3, 1, &attributes->buffer, &vertexTransformBase));
			VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipeline));
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			if (indices16 != nullptr)
			{
				VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, colormappedIndex16Base, VK_INDEX_TYPE_UINT16));
				for (auto i = 0; i <= d_lists.last_alias; i++)
				{
					auto& alias = d_lists.alias[i];
					if (alias.first_index16 < 0)
					{
						continue;
					}
					auto index = appState.Scene.aliasVerticesList[i];
					auto& vertices = appState.Scene.colormappedBufferList[index];
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer->buffer, &vertices.offset));
					index = appState.Scene.aliasTexCoordsList[i];
					auto& texCoords = appState.Scene.colormappedBufferList[index];
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords.buffer->buffer, &texCoords.offset));
					VkDeviceSize attributeOffset = colormappedAttributeBase + alias.first_attribute * sizeof(float);
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
					pushConstants[0] = alias.transform[0][0];
					pushConstants[1] = alias.transform[2][0];
					pushConstants[2] = -alias.transform[1][0];
					pushConstants[4] = alias.transform[0][2];
					pushConstants[5] = alias.transform[2][2];
					pushConstants[6] = -alias.transform[1][2];
					pushConstants[8] = -alias.transform[0][1];
					pushConstants[9] = -alias.transform[2][1];
					pushConstants[10] = alias.transform[1][1];
					pushConstants[12] = alias.transform[0][3];
					pushConstants[13] = alias.transform[2][3];
					pushConstants[14] = -alias.transform[1][3];
					VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.alias.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants));
					VkDescriptorSet descriptorSet;
					auto texture = appState.Scene.aliasList[i].texture.texture;
					auto entry = aliasResources.cache.find(texture);
					if (entry == aliasResources.cache.end())
					{
						descriptorSet = aliasResources.descriptorSets[aliasResources.index];
						aliasResources.index++;
						textureInfo[0].sampler = appState.Scene.samplers[texture->mipCount];
						textureInfo[0].imageView = texture->view;
						writes[0].dstSet = descriptorSet;
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						aliasResources.cache.insert({ texture, descriptorSet });
					}
					else
					{
						descriptorSet = entry->second;
					}
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
					if (alias.is_host_colormap)
					{
						VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 2, 1, &host_colormapResources.descriptorSet, 0, nullptr));
					}
					else
					{
						auto colormap = appState.Scene.aliasList[i].colormap.texture;
						if (colormapResources.bound[descriptorSetIndex] != colormap)
						{
							textureInfo[0].sampler = appState.Scene.samplers[colormap->mipCount];
							textureInfo[0].imageView = colormap->view;
							writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
							VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
							colormapResources.bound[descriptorSetIndex] = texture;
						}
						VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 2, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr));
						descriptorSetIndex++;
					}
					VC(appState.Device.vkCmdDrawIndexed(commandBuffer, alias.count, 1, alias.first_index16, 0, 0));
				}
			}
			if (indices32 != nullptr)
			{
				VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32));
				for (auto i = 0; i <= d_lists.last_alias; i++)
				{
					auto& alias = d_lists.alias[i];
					if (alias.first_index32 < 0)
					{
						continue;
					}
					auto index = appState.Scene.aliasVerticesList[i];
					auto& vertices = appState.Scene.colormappedBufferList[index];
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer->buffer, &vertices.offset));
					index = appState.Scene.aliasTexCoordsList[i];
					auto& texCoords = appState.Scene.colormappedBufferList[index];
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords.buffer->buffer, &texCoords.offset));
					VkDeviceSize attributeOffset = colormappedAttributeBase + alias.first_attribute * sizeof(float);
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
					pushConstants[0] = alias.transform[0][0];
					pushConstants[1] = alias.transform[2][0];
					pushConstants[2] = -alias.transform[1][0];
					pushConstants[4] = alias.transform[0][2];
					pushConstants[5] = alias.transform[2][2];
					pushConstants[6] = -alias.transform[1][2];
					pushConstants[8] = -alias.transform[0][1];
					pushConstants[9] = -alias.transform[2][1];
					pushConstants[10] = alias.transform[1][1];
					pushConstants[12] = alias.transform[0][3];
					pushConstants[13] = alias.transform[2][3];
					pushConstants[14] = -alias.transform[1][3];
					VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.alias.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants));
					VkDescriptorSet descriptorSet;
					auto texture = appState.Scene.aliasList[i].texture.texture;
					auto entry = aliasResources.cache.find(texture);
					if (entry == aliasResources.cache.end())
					{
						descriptorSet = aliasResources.descriptorSets[aliasResources.index];
						aliasResources.index++;
						textureInfo[0].sampler = appState.Scene.samplers[texture->mipCount];
						textureInfo[0].imageView = texture->view;
						writes[0].dstSet = descriptorSet;
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						aliasResources.cache.insert({ texture, descriptorSet });
					}
					else
					{
						descriptorSet = entry->second;
					}
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
					if (alias.is_host_colormap)
					{
						VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 2, 1, &host_colormapResources.descriptorSet, 0, nullptr));
					}
					else
					{
						auto colormap = appState.Scene.aliasList[i].colormap.texture;
						if (colormapResources.bound[descriptorSetIndex] != colormap)
						{
							textureInfo[0].sampler = appState.Scene.samplers[colormap->mipCount];
							textureInfo[0].imageView = colormap->view;
							writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
							VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
							colormapResources.bound[descriptorSetIndex] = texture;
						}
						VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 2, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr));
						descriptorSetIndex++;
					}
					VC(appState.Device.vkCmdDrawIndexed(commandBuffer, alias.count, 1, alias.first_index32, 0, 0));
				}
			}
		}
		if (resetDescriptorSetsCount != appState.Scene.resetDescriptorSetsCount || viewmodelResources.descriptorSets.size() < appState.Scene.viewmodelTextureCount)
		{
			viewmodelResources.Delete(appState);
			appState.Scene.viewmodelDescriptorSetCount = appState.Scene.viewmodelTextureCount;
			if (appState.Scene.viewmodelDescriptorSetCount > 0)
			{
				poolSizes[0].descriptorCount = appState.Scene.viewmodelDescriptorSetCount;
				descriptorPoolCreateInfo.maxSets = appState.Scene.viewmodelDescriptorSetCount;
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &viewmodelResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = viewmodelResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
				viewmodelResources.descriptorSets.resize(appState.Scene.viewmodelDescriptorSetCount);
				for (auto i = 0; i < appState.Scene.viewmodelDescriptorSetCount; i++)
				{
					VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &viewmodelResources.descriptorSets[i]));
				}
				viewmodelResources.created = true;
			}
		}
		if (d_lists.last_viewmodel >= 0)
		{
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 3, 1, &attributes->buffer, &vertexTransformBase));
			VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipeline));
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			if (appState.NearViewModel)
			{
				pushConstants[16] = vpn[0] / appState.Scale;
				pushConstants[17] = vpn[2] / appState.Scale;
				pushConstants[18] = -vpn[1] / appState.Scale;
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
			if (indices16 != nullptr)
			{
				VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, colormappedIndex16Base, VK_INDEX_TYPE_UINT16));
				for (auto i = 0; i <= d_lists.last_viewmodel; i++)
				{
					auto& viewmodel = d_lists.viewmodel[i];
					if (viewmodel.first_index16 < 0)
					{
						continue;
					}
					auto index = appState.Scene.viewmodelVerticesList[i];
					auto& vertices = appState.Scene.colormappedBufferList[index];
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer->buffer, &vertices.offset));
					index = appState.Scene.viewmodelTexCoordsList[i];
					auto& texCoords = appState.Scene.colormappedBufferList[index];
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords.buffer->buffer, &texCoords.offset));
					VkDeviceSize attributeOffset = colormappedAttributeBase + viewmodel.first_attribute * sizeof(float);
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
					pushConstants[0] = viewmodel.transform[0][0];
					pushConstants[1] = viewmodel.transform[2][0];
					pushConstants[2] = -viewmodel.transform[1][0];
					pushConstants[4] = viewmodel.transform[0][2];
					pushConstants[5] = viewmodel.transform[2][2];
					pushConstants[6] = -viewmodel.transform[1][2];
					pushConstants[8] = -viewmodel.transform[0][1];
					pushConstants[9] = -viewmodel.transform[2][1];
					pushConstants[10] = viewmodel.transform[1][1];
					pushConstants[12] = viewmodel.transform[0][3];
					pushConstants[13] = viewmodel.transform[2][3];
					pushConstants[14] = -viewmodel.transform[1][3];
					VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.viewmodel.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 24 * sizeof(float), pushConstants));
					VkDescriptorSet descriptorSet;
					auto texture = appState.Scene.viewmodelList[i].texture.texture;
					auto entry = viewmodelResources.cache.find(texture);
					if (entry == viewmodelResources.cache.end())
					{
						descriptorSet = viewmodelResources.descriptorSets[viewmodelResources.index];
						viewmodelResources.index++;
						textureInfo[0].sampler = appState.Scene.samplers[texture->mipCount];
						textureInfo[0].imageView = texture->view;
						writes[0].dstSet = descriptorSet;
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						viewmodelResources.cache.insert({ texture, descriptorSet });
					}
					else
					{
						descriptorSet = entry->second;
					}
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
					if (viewmodel.is_host_colormap)
					{
						VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 2, 1, &host_colormapResources.descriptorSet, 0, nullptr));
					}
					else
					{
						auto colormap = appState.Scene.viewmodelList[i].colormap.texture;
						if (colormapResources.bound[descriptorSetIndex] != colormap)
						{
							textureInfo[0].sampler = appState.Scene.samplers[colormap->mipCount];
							textureInfo[0].imageView = colormap->view;
							writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
							VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
							colormapResources.bound[descriptorSetIndex] = texture;
						}
						VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 2, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr));
						descriptorSetIndex++;
					}
					VC(appState.Device.vkCmdDrawIndexed(commandBuffer, viewmodel.count, 1, viewmodel.first_index16, 0, 0));
				}
			}
			if (indices32 != nullptr)
			{
				VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32));
				for (auto i = 0; i <= d_lists.last_viewmodel; i++)
				{
					auto& viewmodel = d_lists.viewmodel[i];
					if (viewmodel.first_index32 < 0)
					{
						continue;
					}
					auto index = appState.Scene.viewmodelVerticesList[i];
					auto& vertices = appState.Scene.colormappedBufferList[index];
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer->buffer, &vertices.offset));
					index = appState.Scene.viewmodelTexCoordsList[i];
					auto& texCoords = appState.Scene.colormappedBufferList[index];
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords.buffer->buffer, &texCoords.offset));
					VkDeviceSize attributeOffset = colormappedAttributeBase + viewmodel.first_attribute * sizeof(float);
					VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
					pushConstants[0] = viewmodel.transform[0][0];
					pushConstants[1] = viewmodel.transform[2][0];
					pushConstants[2] = -viewmodel.transform[1][0];
					pushConstants[4] = viewmodel.transform[0][2];
					pushConstants[5] = viewmodel.transform[2][2];
					pushConstants[6] = -viewmodel.transform[1][2];
					pushConstants[8] = -viewmodel.transform[0][1];
					pushConstants[9] = -viewmodel.transform[2][1];
					pushConstants[10] = viewmodel.transform[1][1];
					pushConstants[12] = viewmodel.transform[0][3];
					pushConstants[13] = viewmodel.transform[2][3];
					pushConstants[14] = -viewmodel.transform[1][3];
					VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.viewmodel.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 24 * sizeof(float), pushConstants));
					VkDescriptorSet descriptorSet;
					auto texture = appState.Scene.viewmodelList[i].texture.texture;
					auto entry = viewmodelResources.cache.find(texture);
					if (entry == viewmodelResources.cache.end())
					{
						descriptorSet = viewmodelResources.descriptorSets[viewmodelResources.index];
						viewmodelResources.index++;
						textureInfo[0].sampler = appState.Scene.samplers[texture->mipCount];
						textureInfo[0].imageView = texture->view;
						writes[0].dstSet = descriptorSet;
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						viewmodelResources.cache.insert({ texture, descriptorSet });
					}
					else
					{
						descriptorSet = entry->second;
					}
					VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
					if (viewmodel.is_host_colormap)
					{
						VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 2, 1, &host_colormapResources.descriptorSet, 0, nullptr));
					}
					else
					{
						auto colormap = appState.Scene.viewmodelList[i].colormap.texture;
						if (colormapResources.bound[descriptorSetIndex] != colormap)
						{
							textureInfo[0].sampler = appState.Scene.samplers[colormap->mipCount];
							textureInfo[0].imageView = colormap->view;
							writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
							VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
							colormapResources.bound[descriptorSetIndex] = texture;
						}
						VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 2, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr));
						descriptorSetIndex++;
					}
					VC(appState.Device.vkCmdDrawIndexed(commandBuffer, viewmodel.count, 1, viewmodel.first_index32, 0, 0));
				}
			}
		}
		resetDescriptorSetsCount = appState.Scene.resetDescriptorSetsCount;
		if (d_lists.last_colored_surfaces_index16 >= 0 || d_lists.last_colored_surfaces_index32 >= 0 || d_lists.last_particles_index16 >= 0 || d_lists.last_particles_index32 >= 0)
		{
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &coloredVertexBase));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &vertexTransformBase));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 2, 1, &colors->buffer, &appState.NoOffset));
			VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.colored.pipeline));
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.colored.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
			if (d_lists.last_colored_surfaces_index16 >= 0)
			{
				VkDeviceSize indexOffset = 0;
				for (auto i = 0; i <= d_lists.last_colored_surfaces_index16; i++)
				{
					auto& coloredSurfaces = d_lists.colored_surfaces_index16[i];
					VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, coloredIndex16Base + indexOffset, VK_INDEX_TYPE_UINT16));
					auto indexCount = coloredSurfaces.count;
					VC(appState.Device.vkCmdDrawIndexed(commandBuffer, indexCount, 1, coloredSurfaces.first_index, 0, 0));
					indexOffset += indexCount;
				}
			}
			if (d_lists.last_colored_surfaces_index32 >= 0)
			{
				VkDeviceSize indexOffset = 0;
				for (auto i = 0; i <= d_lists.last_colored_surfaces_index32; i++)
				{
					auto& coloredSurfaces = d_lists.colored_surfaces_index32[i];
					VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, coloredIndex32Base + indexOffset, VK_INDEX_TYPE_UINT32));
					auto indexCount = coloredSurfaces.count;
					VC(appState.Device.vkCmdDrawIndexed(commandBuffer, indexCount, 1, coloredSurfaces.first_index, 0, 0));
					indexOffset += indexCount;
				}
			}
			if (d_lists.last_particles_index16 >= 0)
			{
				VkDeviceSize indexOffset = 0;
				for (auto i = 0; i <= d_lists.last_particles_index16; i++)
				{
					auto& particles = d_lists.particles_index16[i];
					VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, coloredIndex16Base + indexOffset, VK_INDEX_TYPE_UINT16));
					auto indexCount = particles.count;
					VC(appState.Device.vkCmdDrawIndexed(commandBuffer, indexCount, 1, particles.first_index, 0, 0));
					indexOffset += indexCount;
				}
			}
			if (d_lists.last_particles_index32 >= 0)
			{
				VkDeviceSize indexOffset = 0;
				for (auto i = 0; i <= d_lists.last_particles_index32; i++)
				{
					auto& particles = d_lists.particles_index32[i];
					VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, coloredIndex32Base + indexOffset, VK_INDEX_TYPE_UINT32));
					auto indexCount = particles.count;
					VC(appState.Device.vkCmdDrawIndexed(commandBuffer, indexCount, 1, particles.first_index, 0, 0));
					indexOffset += indexCount;
				}
			}
		}
		if (d_lists.last_sky >= 0)
		{
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &texturedVertexBase));
			VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &texturedAttributeBase));
			VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipeline));
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			if (!skyResources.created)
			{
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &skyResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = skyResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
				VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &skyResources.descriptorSet));
				textureInfo[0].sampler = appState.Scene.samplers[sky->mipCount];
				textureInfo[0].imageView = sky->view;
				writes[0].dstSet = skyResources.descriptorSet;
				VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
				skyResources.created = true;
			}
			VkDescriptorSet descriptorSets[2];
			descriptorSets[0] = sceneMatricesAndPaletteResources.descriptorSet;
			descriptorSets[1] = skyResources.descriptorSet;
			VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipelineLayout, 0, 2, descriptorSets, 0, nullptr));
			float rotation[13];
			auto matrix = ovrMatrix4f_CreateFromQuaternion(&appState.Scene.orientation);
			rotation[0] = -matrix.M[0][2];
			rotation[1] = matrix.M[2][2];
			rotation[2] = -matrix.M[1][2];
			rotation[3] = matrix.M[0][0];
			rotation[4] = -matrix.M[2][0];
			rotation[5] = matrix.M[1][0];
			rotation[6] = matrix.M[0][1];
			rotation[7] = -matrix.M[2][1];
			rotation[8] = matrix.M[1][1];
			rotation[9] = appState.EyeTextureWidth;
			rotation[10] = appState.EyeTextureHeight;
			rotation[11] = appState.EyeTextureMaxDimension;
			rotation[12] = skytime*skyspeed;
			VC(appState.Device.vkCmdPushConstants(commandBuffer, appState.Scene.sky.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 13 * sizeof(float), &rotation));
			for (auto i = 0; i <= d_lists.last_sky; i++)
			{
				auto& sky = d_lists.sky[i];
				VC(appState.Device.vkCmdDraw(commandBuffer, sky.count, 1, sky.first_vertex, 0));
			}
		}
	}
	else
	{
		VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset));
		VC(appState.Device.vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &appState.NoOffset));
		VC(appState.Device.vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipeline));
		if (!floorResources.created)
		{
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &floorResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = floorResources.descriptorPool;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
			VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &floorResources.descriptorSet));
			textureInfo[0].sampler = appState.Scene.samplers[appState.Scene.floorTexture->mipCount];
			textureInfo[0].imageView = appState.Scene.floorTexture->view;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].pImageInfo = textureInfo;
			writes[0].dstSet = floorResources.descriptorSet;
			VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
			floorResources.created = true;
		}
		VkDescriptorSet descriptorSets[2];
		descriptorSets[0] = sceneMatricesResources.descriptorSet;
		descriptorSets[1] = floorResources.descriptorSet;
		VC(appState.Device.vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipelineLayout, 0, 2, descriptorSets, 0, nullptr));
		VC(appState.Device.vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, 0, VK_INDEX_TYPE_UINT16));
		VC(appState.Device.vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0));
	}
}
