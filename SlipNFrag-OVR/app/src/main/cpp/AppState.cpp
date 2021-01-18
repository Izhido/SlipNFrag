#include "AppState.h"
#include "VulkanCallWrappers.h"
#include "vid_ovr.h"
#include "d_lists.h"
#include "Constants.h"
#include "VrApi_Helpers.h"
#include <android/log.h>
#include "sys_ovr.h"

void AppState::RenderScene(VkCommandBufferBeginInfo& commandBufferBeginInfo, ovrPosef& pose, const ovrQuatf& orientation, int eyeTextureWidth, int eyeTextureHeight, VkDeviceSize& noOffset)
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
		perImage.sceneMatricesStagingBuffers.Reset(*this);
		perImage.vertices.Reset(*this);
		perImage.attributes.Reset(*this);
		perImage.indices16.Reset(*this);
		perImage.indices32.Reset(*this);
		perImage.stagingBuffers.Reset(*this);
		perImage.turbulent.Reset(*this);
		perImage.colormaps.Reset(*this);
		perImage.colormapCount = 0;
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
		VkDeviceSize stagingBufferSize = 0;
		auto texturedDescriptorSetCount = 0;
		auto spriteDescriptorSetCount = 0;
		auto colormapDescriptorSetCount = 0;
		auto aliasDescriptorSetCount = 0;
		auto viewmodelDescriptorSetCount = 0;
		auto floorVerticesSize = 0;
		auto texturedVerticesSize = 0;
		auto coloredVerticesSize = 0;
		auto verticesSize = 0;
		auto colormappedVerticesSize = 0;
		auto colormappedTexCoordsSize = 0;
		auto floorAttributesSize = 0;
		auto texturedAttributesSize = 0;
		auto colormappedLightsSize = 0;
		auto vertexTransformSize = 0;
		auto attributesSize = 0;
		auto floorIndicesSize = 0;
		auto colormappedIndices16Size = 0;
		auto coloredIndices16Size = 0;
		auto indices16Size = 0;
		auto colormappedIndices32Size = 0;
		auto coloredIndices32Size = 0;
		auto indices32Size = 0;
		auto floorSize = 0;
		Buffer* vertices = nullptr;
		Buffer* attributes = nullptr;
		Buffer* indices16 = nullptr;
		Buffer* indices32 = nullptr;
		if (Mode != AppWorldMode)
		{
			floorVerticesSize = 3 * 4 * sizeof(float);
		}
		texturedVerticesSize = (d_lists.last_textured_vertex + 1) * sizeof(float);
		coloredVerticesSize = (d_lists.last_colored_vertex + 1) * sizeof(float);
		verticesSize = texturedVerticesSize + coloredVerticesSize + floorVerticesSize;
		if (verticesSize > 0 || d_lists.last_alias >= 0 || d_lists.last_viewmodel >= 0)
		{
			for (Buffer** b = &perImage.vertices.oldBuffers; *b != nullptr; b = &(*b)->next)
			{
				if ((*b)->size >= verticesSize && (*b)->size < verticesSize * 2)
				{
					vertices = *b;
					*b = (*b)->next;
					break;
				}
			}
			if (vertices == nullptr)
			{
				vertices = new Buffer();
				vertices->CreateVertexBuffer(*this, verticesSize + verticesSize / 4);
			}
			perImage.vertices.MoveToFront(vertices);
			VK(Device.vkMapMemory(Device.device, vertices->memory, 0, verticesSize, 0, &vertices->mapped));
			if (floorVerticesSize > 0)
			{
				auto mapped = (float*)vertices->mapped;
				(*mapped) = -0.5;
				mapped++;
				(*mapped) = pose.Position.y;
				mapped++;
				(*mapped) = -0.5;
				mapped++;
				(*mapped) = 0.5;
				mapped++;
				(*mapped) = pose.Position.y;
				mapped++;
				(*mapped) = -0.5;
				mapped++;
				(*mapped) = 0.5;
				mapped++;
				(*mapped) = pose.Position.y;
				mapped++;
				(*mapped) = 0.5;
				mapped++;
				(*mapped) = -0.5;
				mapped++;
				(*mapped) = pose.Position.y;
				mapped++;
				(*mapped) = 0.5;
			}
			perImage.texturedVertexBase = floorVerticesSize;
			memcpy((unsigned char*)vertices->mapped + perImage.texturedVertexBase, d_lists.textured_vertices.data(), texturedVerticesSize);
			perImage.coloredVertexBase = perImage.texturedVertexBase + texturedVerticesSize;
			memcpy((unsigned char*)vertices->mapped + perImage.coloredVertexBase, d_lists.colored_vertices.data(), coloredVerticesSize);
			VC(Device.vkUnmapMemory(Device.device, vertices->memory));
			vertices->mapped = nullptr;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferMemoryBarrier.buffer = vertices->buffer;
			bufferMemoryBarrier.size = vertices->size;
			VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			if (d_lists.last_alias >= Scene.aliasVerticesList.size())
			{
				Scene.aliasVerticesList.resize(d_lists.last_alias + 1);
				Scene.aliasTexCoordsList.resize(d_lists.last_alias + 1);
			}
			Scene.newVertices.clear();
			Scene.newTexCoords.clear();
			VkDeviceSize verticesOffset = 0;
			VkDeviceSize texCoordsOffset = 0;
			for (auto i = 0; i <= d_lists.last_alias; i++)
			{
				auto& alias = d_lists.alias[i];
				auto verticesEntry = Scene.colormappedVerticesPerKey.find(alias.vertices);
				if (verticesEntry == Scene.colormappedVerticesPerKey.end())
				{
					auto lastIndex = Scene.colormappedBufferList.size();
					Scene.colormappedBufferList.push_back({ verticesOffset });
					auto newEntry = Scene.colormappedVerticesPerKey.insert({ alias.vertices, lastIndex });
					Scene.newVertices.push_back(i);
					Scene.aliasVerticesList[i] = lastIndex;
					verticesOffset += alias.vertex_count * 2 * 4 * sizeof(float);
				}
				else
				{
					Scene.aliasVerticesList[i] = verticesEntry->second;
				}
				auto texCoordsEntry = Scene.colormappedTexCoordsPerKey.find(alias.texture_coordinates);
				if (texCoordsEntry == Scene.colormappedTexCoordsPerKey.end())
				{
					auto lastIndex = Scene.colormappedBufferList.size();
					Scene.colormappedBufferList.push_back({ texCoordsOffset });
					auto newEntry = Scene.colormappedTexCoordsPerKey.insert({ alias.texture_coordinates, lastIndex });
					Scene.newTexCoords.push_back(i);
					Scene.aliasTexCoordsList[i] = lastIndex;
					texCoordsOffset += alias.vertex_count * 2 * 2 * sizeof(float);
				}
				else
				{
					Scene.aliasTexCoordsList[i] = texCoordsEntry->second;
				}
			}
			if (verticesOffset > 0)
			{
				Buffer* buffer;
				if (Scene.latestColormappedBuffer == nullptr || Scene.usedInLatestColormappedBuffer + verticesOffset > MEMORY_BLOCK_SIZE)
				{
					VkDeviceSize size = MEMORY_BLOCK_SIZE;
					if (size < verticesOffset)
					{
						size = verticesOffset;
					}
					buffer = new Buffer();
					buffer->CreateVertexBuffer(*this, size);
					Scene.colormappedBuffers.MoveToFront(buffer);
					Scene.latestColormappedBuffer = buffer;
					Scene.usedInLatestColormappedBuffer = 0;
				}
				else
				{
					buffer = Scene.latestColormappedBuffer;
				}
				colormappedVerticesSize += verticesOffset;
				VK(Device.vkMapMemory(Device.device, buffer->memory, Scene.usedInLatestColormappedBuffer, verticesOffset, 0, &buffer->mapped));
				auto mapped = (float*)buffer->mapped;
				for (auto i : Scene.newVertices)
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
					auto index = Scene.aliasVerticesList[i];
					Scene.colormappedBufferList[index].buffer = buffer;
					Scene.colormappedBufferList[index].offset += Scene.usedInLatestColormappedBuffer;
				}
				VC(Device.vkUnmapMemory(Device.device, buffer->memory));
				buffer->mapped = nullptr;
				bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				bufferMemoryBarrier.buffer = buffer->buffer;
				bufferMemoryBarrier.size = buffer->size;
				VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
				Scene.usedInLatestColormappedBuffer += verticesOffset;
			}
			if (texCoordsOffset > 0)
			{
				Buffer* buffer;
				if (Scene.latestColormappedBuffer == nullptr || Scene.usedInLatestColormappedBuffer + texCoordsOffset > MEMORY_BLOCK_SIZE)
				{
					VkDeviceSize size = MEMORY_BLOCK_SIZE;
					if (size < texCoordsOffset)
					{
						size = texCoordsOffset;
					}
					buffer = new Buffer();
					buffer->CreateVertexBuffer(*this, size);
					Scene.colormappedBuffers.MoveToFront(buffer);
					Scene.latestColormappedBuffer = buffer;
					Scene.usedInLatestColormappedBuffer = 0;
				}
				else
				{
					buffer = Scene.latestColormappedBuffer;
				}
				colormappedTexCoordsSize += texCoordsOffset;
				VK(Device.vkMapMemory(Device.device, buffer->memory, Scene.usedInLatestColormappedBuffer, texCoordsOffset, 0, &buffer->mapped));
				auto mapped = (float*)buffer->mapped;
				for (auto i : Scene.newTexCoords)
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
					auto index = Scene.aliasTexCoordsList[i];
					Scene.colormappedBufferList[index].buffer = buffer;
					Scene.colormappedBufferList[index].offset += Scene.usedInLatestColormappedBuffer;
				}
				VC(Device.vkUnmapMemory(Device.device, buffer->memory));
				buffer->mapped = nullptr;
				bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				bufferMemoryBarrier.buffer = buffer->buffer;
				bufferMemoryBarrier.size = buffer->size;
				VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
				Scene.usedInLatestColormappedBuffer += texCoordsOffset;
			}
			if (d_lists.last_viewmodel >= Scene.viewmodelVerticesList.size())
			{
				Scene.viewmodelVerticesList.resize(d_lists.last_viewmodel + 1);
				Scene.viewmodelTexCoordsList.resize(d_lists.last_viewmodel + 1);
			}
			Scene.newVertices.clear();
			Scene.newTexCoords.clear();
			verticesOffset = 0;
			texCoordsOffset = 0;
			for (auto i = 0; i <= d_lists.last_viewmodel; i++)
			{
				auto& viewmodel = d_lists.viewmodel[i];
				auto verticesEntry = Scene.colormappedVerticesPerKey.find(viewmodel.vertices);
				if (verticesEntry == Scene.colormappedVerticesPerKey.end())
				{
					auto lastIndex = Scene.colormappedBufferList.size();
					Scene.colormappedBufferList.push_back({ verticesOffset });
					auto newEntry = Scene.colormappedVerticesPerKey.insert({ viewmodel.vertices, lastIndex });
					Scene.newVertices.push_back(i);
					Scene.viewmodelVerticesList[i] = lastIndex;
					verticesOffset += viewmodel.vertex_count * 2 * 4 * sizeof(float);
				}
				else
				{
					Scene.viewmodelVerticesList[i] = verticesEntry->second;
				}
				auto texCoordsEntry = Scene.colormappedTexCoordsPerKey.find(viewmodel.texture_coordinates);
				if (texCoordsEntry == Scene.colormappedTexCoordsPerKey.end())
				{
					auto lastIndex = Scene.colormappedBufferList.size();
					Scene.colormappedBufferList.push_back({ texCoordsOffset });
					auto newEntry = Scene.colormappedTexCoordsPerKey.insert({ viewmodel.texture_coordinates, lastIndex });
					Scene.newTexCoords.push_back(i);
					Scene.viewmodelTexCoordsList[i] = lastIndex;
					texCoordsOffset += viewmodel.vertex_count * 2 * 2 * sizeof(float);
				}
				else
				{
					Scene.viewmodelTexCoordsList[i] = texCoordsEntry->second;
				}
			}
			if (verticesOffset > 0)
			{
				Buffer* buffer;
				if (Scene.latestColormappedBuffer == nullptr || Scene.usedInLatestColormappedBuffer + verticesOffset > MEMORY_BLOCK_SIZE)
				{
					VkDeviceSize size = MEMORY_BLOCK_SIZE;
					if (size < verticesOffset)
					{
						size = verticesOffset;
					}
					buffer = new Buffer();
					buffer->CreateVertexBuffer(*this, size);
					Scene.colormappedBuffers.MoveToFront(buffer);
					Scene.latestColormappedBuffer = buffer;
					Scene.usedInLatestColormappedBuffer = 0;
				}
				else
				{
					buffer = Scene.latestColormappedBuffer;
				}
				colormappedVerticesSize += verticesOffset;
				VK(Device.vkMapMemory(Device.device, buffer->memory, Scene.usedInLatestColormappedBuffer, verticesOffset, 0, &buffer->mapped));
				auto mapped = (float*)buffer->mapped;
				for (auto i : Scene.newVertices)
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
					auto index = Scene.viewmodelVerticesList[i];
					Scene.colormappedBufferList[index].buffer = buffer;
					Scene.colormappedBufferList[index].offset += Scene.usedInLatestColormappedBuffer;
				}
				VC(Device.vkUnmapMemory(Device.device, buffer->memory));
				buffer->mapped = nullptr;
				bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				bufferMemoryBarrier.buffer = buffer->buffer;
				bufferMemoryBarrier.size = buffer->size;
				VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
				Scene.usedInLatestColormappedBuffer += verticesOffset;
			}
			if (texCoordsOffset > 0)
			{
				Buffer* buffer;
				if (Scene.latestColormappedBuffer == nullptr || Scene.usedInLatestColormappedBuffer + texCoordsOffset > MEMORY_BLOCK_SIZE)
				{
					VkDeviceSize size = MEMORY_BLOCK_SIZE;
					if (size < texCoordsOffset)
					{
						size = texCoordsOffset;
					}
					buffer = new Buffer();
					buffer->CreateVertexBuffer(*this, size);
					Scene.colormappedBuffers.MoveToFront(buffer);
					Scene.latestColormappedBuffer = buffer;
					Scene.usedInLatestColormappedBuffer = 0;
				}
				else
				{
					buffer = Scene.latestColormappedBuffer;
				}
				colormappedTexCoordsSize += texCoordsOffset;
				VK(Device.vkMapMemory(Device.device, buffer->memory, Scene.usedInLatestColormappedBuffer, texCoordsOffset, 0, &buffer->mapped));
				auto mapped = (float*)buffer->mapped;
				for (auto i : Scene.newTexCoords)
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
					auto index = Scene.viewmodelTexCoordsList[i];
					Scene.colormappedBufferList[index].buffer = buffer;
					Scene.colormappedBufferList[index].offset += Scene.usedInLatestColormappedBuffer;
				}
				VC(Device.vkUnmapMemory(Device.device, buffer->memory));
				buffer->mapped = nullptr;
				bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				bufferMemoryBarrier.buffer = buffer->buffer;
				bufferMemoryBarrier.size = buffer->size;
				VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
				Scene.usedInLatestColormappedBuffer += texCoordsOffset;
			}
			if (Mode != AppWorldMode)
			{
				floorAttributesSize = 2 * 4 * sizeof(float);
			}
			texturedAttributesSize = (d_lists.last_textured_attribute + 1) * sizeof(float);
			colormappedLightsSize = (d_lists.last_colormapped_attribute + 1) * sizeof(float);
			vertexTransformSize = 16 * sizeof(float);
			attributesSize = floorAttributesSize + texturedAttributesSize + colormappedLightsSize + vertexTransformSize;
			for (Buffer** b = &perImage.attributes.oldBuffers; *b != nullptr; b = &(*b)->next)
			{
				if ((*b)->size >= attributesSize && (*b)->size < attributesSize * 2)
				{
					attributes = *b;
					*b = (*b)->next;
					break;
				}
			}
			if (attributes == nullptr)
			{
				attributes = new Buffer();
				attributes->CreateVertexBuffer(*this, attributesSize + attributesSize / 4);
			}
			perImage.attributes.MoveToFront(attributes);
			VK(Device.vkMapMemory(Device.device, attributes->memory, 0, attributesSize, 0, &attributes->mapped));
			if (floorAttributesSize > 0)
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
			perImage.texturedAttributeBase = floorAttributesSize;
			memcpy((unsigned char*)attributes->mapped + perImage.texturedAttributeBase, d_lists.textured_attributes.data(), texturedAttributesSize);
			perImage.colormappedAttributeBase = perImage.texturedAttributeBase + texturedAttributesSize;
			memcpy((unsigned char*)attributes->mapped + perImage.colormappedAttributeBase, d_lists.colormapped_attributes.data(), colormappedLightsSize);
			perImage.vertexTransformBase = perImage.colormappedAttributeBase + colormappedLightsSize;
			auto mapped = (float*)attributes->mapped + perImage.vertexTransformBase / sizeof(float);
			(*mapped) = Scale;
			mapped++;
			(*mapped) = 0;
			mapped++;
			(*mapped) = 0;
			mapped++;
			(*mapped) = 0;
			mapped++;
			(*mapped) = 0;
			mapped++;
			(*mapped) = Scale;
			mapped++;
			(*mapped) = 0;
			mapped++;
			(*mapped) = 0;
			mapped++;
			(*mapped) = 0;
			mapped++;
			(*mapped) = 0;
			mapped++;
			(*mapped) = Scale;
			mapped++;
			(*mapped) = 0;
			mapped++;
			(*mapped) = -r_refdef.vieworg[0] * Scale;
			mapped++;
			(*mapped) = -r_refdef.vieworg[2] * Scale;
			mapped++;
			(*mapped) = r_refdef.vieworg[1] * Scale;
			mapped++;
			(*mapped) = 1;
			VC(Device.vkUnmapMemory(Device.device, attributes->memory));
			attributes->mapped = nullptr;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferMemoryBarrier.buffer = attributes->buffer;
			bufferMemoryBarrier.size = attributes->size;
			VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			if (Mode != AppWorldMode)
			{
				floorIndicesSize = 6 * sizeof(uint16_t);
			}
			colormappedIndices16Size = (d_lists.last_colormapped_index16 + 1) * sizeof(uint16_t);
			coloredIndices16Size = (d_lists.last_colored_index16 + 1) * sizeof(uint16_t);
			indices16Size = floorIndicesSize + colormappedIndices16Size + coloredIndices16Size;
			if (indices16Size > 0)
			{
				for (Buffer** b = &perImage.indices16.oldBuffers; *b != nullptr; b = &(*b)->next)
				{
					if ((*b)->size >= indices16Size && (*b)->size < indices16Size * 2)
					{
						indices16 = *b;
						*b = (*b)->next;
						break;
					}
				}
				if (indices16 == nullptr)
				{
					indices16 = new Buffer();
					indices16->CreateIndexBuffer(*this, indices16Size + indices16Size / 4);
				}
				perImage.indices16.MoveToFront(indices16);
				VK(Device.vkMapMemory(Device.device, indices16->memory, 0, indices16Size, 0, &indices16->mapped));
				if (floorIndicesSize > 0)
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
				perImage.colormappedIndex16Base = floorIndicesSize;
				memcpy((unsigned char*)indices16->mapped + perImage.colormappedIndex16Base, d_lists.colormapped_indices16.data(), colormappedIndices16Size);
				perImage.coloredIndex16Base = perImage.colormappedIndex16Base + colormappedIndices16Size;
				memcpy((unsigned char*)indices16->mapped + perImage.coloredIndex16Base, d_lists.colored_indices16.data(), coloredIndices16Size);
				VC(Device.vkUnmapMemory(Device.device, indices16->memory));
				indices16->mapped = nullptr;
				bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				bufferMemoryBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
				bufferMemoryBarrier.buffer = indices16->buffer;
				bufferMemoryBarrier.size = indices16->size;
				VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			}
			colormappedIndices32Size = (d_lists.last_colormapped_index32 + 1) * sizeof(uint32_t);
			coloredIndices32Size = (d_lists.last_colored_index32 + 1) * sizeof(uint32_t);
			indices32Size = colormappedIndices32Size + coloredIndices32Size;
			if (indices32Size > 0)
			{
				for (Buffer** b = &perImage.indices32.oldBuffers; *b != nullptr; b = &(*b)->next)
				{
					if ((*b)->size >= indices32Size && (*b)->size < indices32Size * 2)
					{
						indices32 = *b;
						*b = (*b)->next;
						break;
					}
				}
				if (indices32 == nullptr)
				{
					indices32 = new Buffer();
					indices32->CreateIndexBuffer(*this, indices32Size + indices32Size / 4);
				}
				perImage.indices32.MoveToFront(indices32);
				VK(Device.vkMapMemory(Device.device, indices32->memory, 0, indices32Size, 0, &indices32->mapped));
				memcpy(indices32->mapped, d_lists.colormapped_indices32.data(), colormappedIndices32Size);
				perImage.coloredIndex32Base = colormappedIndices32Size;
				memcpy((unsigned char*)indices32->mapped + perImage.coloredIndex32Base, d_lists.colored_indices32.data(), coloredIndices32Size);
				VC(Device.vkUnmapMemory(Device.device, indices32->memory));
				indices32->mapped = nullptr;
				bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				bufferMemoryBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
				bufferMemoryBarrier.buffer = indices32->buffer;
				bufferMemoryBarrier.size = indices32->size;
				VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			}
		}
		perImage.GetStagingBufferSize(*this, view, stagingBufferSize, floorSize);
		perImage.hostClearCount = host_clearcount;
		stagingBuffer = nullptr;
		if (stagingBufferSize > 0)
		{
			for (Buffer** b = &perImage.stagingBuffers.oldBuffers; *b != nullptr; b = &(*b)->next)
			{
				if ((*b)->size >= stagingBufferSize && (*b)->size < stagingBufferSize * 2)
				{
					stagingBuffer = *b;
					*b = (*b)->next;
					break;
				}
			}
			if (stagingBuffer == nullptr)
			{
				stagingBuffer = new Buffer();
				stagingBuffer->CreateStagingStorageBuffer(*this, stagingBufferSize + stagingBufferSize / 4);
			}
			perImage.stagingBuffers.MoveToFront(stagingBuffer);
			perImage.LoadStagingBuffer(*this, stagingBuffer, stagingBufferSize, floorSize);
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
		if (verticesSize > 0)
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
			if (Mode == AppWorldMode)
			{
				VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer, &perImage.texturedVertexBase));
				VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &attributes->buffer, &perImage.texturedAttributeBase));
				VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 2, 1, &attributes->buffer, &perImage.vertexTransformBase));
				VC(Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.textured.pipeline));
				VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.textured.pipelineLayout, 0, 1, &perImage.sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
				poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolSizes[0].descriptorCount = 1;
				descriptorPoolCreateInfo.poolSizeCount = 1;
				writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writes[0].pImageInfo = textureInfo;
				if (d_lists.last_surface < 0 && d_lists.last_turbulent < 0)
				{
					perImage.texturedResources.Delete(*this);
				}
				else
				{
					auto size = perImage.texturedResources.descriptorSets.size();
					auto required = d_lists.last_surface + 1 + d_lists.last_turbulent + 1;
					if (size < required || size > required * 2)
					{
						auto toCreate = std::max(16, required + required / 4);
						if (toCreate != size)
						{
							texturedDescriptorSetCount = toCreate;
							if (perImage.texturedResources.created)
							{
								VC(Device.vkDestroyDescriptorPool(Device.device, perImage.texturedResources.descriptorPool, nullptr));
							}
							perImage.texturedResources.bound.clear();
							poolSizes[0].descriptorCount = toCreate;
							descriptorPoolCreateInfo.maxSets = toCreate;
							VK(Device.vkCreateDescriptorPool(Device.device, &descriptorPoolCreateInfo, nullptr, &perImage.texturedResources.descriptorPool));
							descriptorSetAllocateInfo.descriptorPool = perImage.texturedResources.descriptorPool;
							descriptorSetAllocateInfo.pSetLayouts = &Scene.singleImageLayout;
							perImage.texturedResources.descriptorSets.resize(toCreate);
							perImage.texturedResources.bound.resize(toCreate);
							for (auto i = 0; i < toCreate; i++)
							{
								VK(Device.vkAllocateDescriptorSets(Device.device, &descriptorSetAllocateInfo, &perImage.texturedResources.descriptorSets[i]));
							}
							perImage.texturedResources.created = true;
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
					VC(Device.vkCmdPushConstants(perImage.commandBuffer, Scene.textured.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 3 * sizeof(float), pushConstants));
					auto texture = Scene.surfaceList[i].texture;
					if (perImage.texturedResources.bound[descriptorSetIndex] != texture)
					{
						textureInfo[0].sampler = Scene.samplers[texture->mipCount];
						textureInfo[0].imageView = texture->view;
						writes[0].dstSet = perImage.texturedResources.descriptorSets[descriptorSetIndex];
						VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
						perImage.texturedResources.bound[descriptorSetIndex] = texture;
					}
					VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.textured.pipelineLayout, 1, 1, &perImage.texturedResources.descriptorSets[descriptorSetIndex], 0, nullptr));
					descriptorSetIndex++;
					VC(Device.vkCmdDraw(perImage.commandBuffer, surface.count, 1, surface.first_vertex, 0));
				}
				VC(Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.sprites.pipeline));
				VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.sprites.pipelineLayout, 0, 1, &perImage.sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
				if (perImage.resetDescriptorSetsCount != Scene.resetDescriptorSetsCount || perImage.spriteResources.descriptorSets.size() < Scene.spriteTextureCount)
				{
					perImage.spriteResources.Delete(*this);
					spriteDescriptorSetCount = Scene.spriteTextureCount;
					if (spriteDescriptorSetCount > 0)
					{
						poolSizes[0].descriptorCount = spriteDescriptorSetCount;
						descriptorPoolCreateInfo.maxSets = spriteDescriptorSetCount;
						VK(Device.vkCreateDescriptorPool(Device.device, &descriptorPoolCreateInfo, nullptr, &perImage.spriteResources.descriptorPool));
						descriptorSetAllocateInfo.descriptorPool = perImage.spriteResources.descriptorPool;
						descriptorSetAllocateInfo.pSetLayouts = &Scene.singleImageLayout;
						perImage.spriteResources.descriptorSets.resize(spriteDescriptorSetCount);
						for (auto i = 0; i < spriteDescriptorSetCount; i++)
						{
							VK(Device.vkAllocateDescriptorSets(Device.device, &descriptorSetAllocateInfo, &perImage.spriteResources.descriptorSets[i]));
						}
						perImage.spriteResources.created = true;
					}
				}
				for (auto i = 0; i <= d_lists.last_sprite; i++)
				{
					auto& sprite = d_lists.sprites[i];
					VkDescriptorSet descriptorSet;
					auto texture = Scene.spriteList[i].texture;
					auto entry = perImage.spriteResources.cache.find(texture);
					if (entry == perImage.spriteResources.cache.end())
					{
						descriptorSet = perImage.spriteResources.descriptorSets[perImage.spriteResources.index];
						perImage.spriteResources.index++;
						textureInfo[0].sampler = Scene.samplers[texture->mipCount];
						textureInfo[0].imageView = texture->view;
						writes[0].dstSet = descriptorSet;
						VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
						perImage.spriteResources.cache.insert({ texture, descriptorSet });
					}
					else
					{
						descriptorSet = entry->second;
					}
					VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.sprites.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
					VC(Device.vkCmdDraw(perImage.commandBuffer, sprite.count, 1, sprite.first_vertex, 0));
				}
				VC(Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.turbulent.pipeline));
				VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.turbulent.pipelineLayout, 0, 1, &perImage.sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
				pushConstants[3] = (float)cl.time;
				for (auto i = 0; i <= d_lists.last_turbulent; i++)
				{
					auto& turbulent = d_lists.turbulent[i];
					pushConstants[0] = turbulent.origin_x;
					pushConstants[1] = turbulent.origin_z;
					pushConstants[2] = -turbulent.origin_y;
					VC(Device.vkCmdPushConstants(perImage.commandBuffer, Scene.turbulent.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), pushConstants));
					auto texture = Scene.turbulentList[i].texture;
					if (perImage.texturedResources.bound[descriptorSetIndex] != texture)
					{
						textureInfo[0].sampler = Scene.samplers[texture->mipCount];
						textureInfo[0].imageView = texture->view;
						writes[0].dstSet = perImage.texturedResources.descriptorSets[descriptorSetIndex];
						VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
						perImage.texturedResources.bound[descriptorSetIndex] = texture;
					}
					VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.turbulent.pipelineLayout, 1, 1, &perImage.texturedResources.descriptorSets[descriptorSetIndex], 0, nullptr));
					descriptorSetIndex++;
					VC(Device.vkCmdDraw(perImage.commandBuffer, turbulent.count, 1, turbulent.first_vertex, 0));
				}
				if (perImage.colormapCount == 0)
				{
					perImage.colormapResources.Delete(*this);
				}
				else
				{
					auto size = perImage.colormapResources.descriptorSets.size();
					auto required = perImage.colormapCount;
					if (size < required || size > required * 2)
					{
						auto toCreate = std::max(4, required + required / 4);
						if (toCreate != size)
						{
							colormapDescriptorSetCount = toCreate;
							if (perImage.colormapResources.created)
							{
								VC(Device.vkDestroyDescriptorPool(Device.device, perImage.colormapResources.descriptorPool, nullptr));
							}
							perImage.texturedResources.bound.clear();
							poolSizes[0].descriptorCount = toCreate;
							descriptorPoolCreateInfo.maxSets = toCreate;
							VK(Device.vkCreateDescriptorPool(Device.device, &descriptorPoolCreateInfo, nullptr, &perImage.colormapResources.descriptorPool));
							descriptorSetAllocateInfo.descriptorPool = perImage.colormapResources.descriptorPool;
							descriptorSetAllocateInfo.pSetLayouts = &Scene.singleImageLayout;
							perImage.colormapResources.descriptorSets.resize(toCreate);
							perImage.texturedResources.bound.resize(toCreate);
							for (auto i = 0; i < toCreate; i++)
							{
								VK(Device.vkAllocateDescriptorSets(Device.device, &descriptorSetAllocateInfo, &perImage.colormapResources.descriptorSets[i]));
							}
							perImage.colormapResources.created = true;
						}
					}
				}
				if (perImage.resetDescriptorSetsCount != Scene.resetDescriptorSetsCount || perImage.aliasResources.descriptorSets.size() < Scene.aliasTextureCount)
				{
					perImage.aliasResources.Delete(*this);
					aliasDescriptorSetCount = Scene.aliasTextureCount;
					if (aliasDescriptorSetCount > 0)
					{
						poolSizes[0].descriptorCount = aliasDescriptorSetCount;
						descriptorPoolCreateInfo.maxSets = aliasDescriptorSetCount;
						VK(Device.vkCreateDescriptorPool(Device.device, &descriptorPoolCreateInfo, nullptr, &perImage.aliasResources.descriptorPool));
						descriptorSetAllocateInfo.descriptorPool = perImage.aliasResources.descriptorPool;
						descriptorSetAllocateInfo.pSetLayouts = &Scene.singleImageLayout;
						perImage.aliasResources.descriptorSets.resize(aliasDescriptorSetCount);
						for (auto i = 0; i < aliasDescriptorSetCount; i++)
						{
							VK(Device.vkAllocateDescriptorSets(Device.device, &descriptorSetAllocateInfo, &perImage.aliasResources.descriptorSets[i]));
						}
						perImage.aliasResources.created = true;
					}
				}
				descriptorSetIndex = 0;
				if (d_lists.last_alias >= 0)
				{
					VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 3, 1, &attributes->buffer, &perImage.vertexTransformBase));
					VC(Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.alias.pipeline));
					VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.alias.pipelineLayout, 0, 1, &perImage.sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
					pushConstants[3] = 0;
					pushConstants[7] = 0;
					pushConstants[11] = 0;
					pushConstants[15] = 1;
					if (indices16 != nullptr)
					{
						VC(Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices16->buffer, perImage.colormappedIndex16Base, VK_INDEX_TYPE_UINT16));
						for (auto i = 0; i <= d_lists.last_alias; i++)
						{
							auto& alias = d_lists.alias[i];
							if (alias.first_index16 < 0)
							{
								continue;
							}
							auto index = Scene.aliasVerticesList[i];
							auto& vertices = Scene.colormappedBufferList[index];
							VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices.buffer->buffer, &vertices.offset));
							index = Scene.aliasTexCoordsList[i];
							auto& texCoords = Scene.colormappedBufferList[index];
							VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &texCoords.buffer->buffer, &texCoords.offset));
							VkDeviceSize attributeOffset = perImage.colormappedAttributeBase + alias.first_attribute * sizeof(float);
							VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
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
							VC(Device.vkCmdPushConstants(perImage.commandBuffer, Scene.alias.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants));
							VkDescriptorSet descriptorSet;
							auto texture = Scene.aliasList[i].texture.texture;
							auto entry = perImage.aliasResources.cache.find(texture);
							if (entry == perImage.aliasResources.cache.end())
							{
								descriptorSet = perImage.aliasResources.descriptorSets[perImage.aliasResources.index];
								perImage.aliasResources.index++;
								textureInfo[0].sampler = Scene.samplers[texture->mipCount];
								textureInfo[0].imageView = texture->view;
								writes[0].dstSet = descriptorSet;
								VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
								perImage.aliasResources.cache.insert({ texture, descriptorSet });
							}
							else
							{
								descriptorSet = entry->second;
							}
							VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.alias.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
							if (alias.is_host_colormap)
							{
								VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.alias.pipelineLayout, 2, 1, &perImage.host_colormapResources.descriptorSet, 0, nullptr));
							}
							else
							{
								auto colormap = Scene.aliasList[i].colormap.texture;
								if (perImage.colormapResources.bound[descriptorSetIndex] != colormap)
								{
									textureInfo[0].sampler = Scene.samplers[colormap->mipCount];
									textureInfo[0].imageView = colormap->view;
									writes[0].dstSet = perImage.colormapResources.descriptorSets[descriptorSetIndex];
									VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
									perImage.colormapResources.bound[descriptorSetIndex] = texture;
								}
								VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.alias.pipelineLayout, 2, 1, &perImage.colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr));
								descriptorSetIndex++;
							}
							VC(Device.vkCmdDrawIndexed(perImage.commandBuffer, alias.count, 1, alias.first_index16, 0, 0));
						}
					}
					if (indices32 != nullptr)
					{
						VC(Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32));
						for (auto i = 0; i <= d_lists.last_alias; i++)
						{
							auto& alias = d_lists.alias[i];
							if (alias.first_index32 < 0)
							{
								continue;
							}
							auto index = Scene.aliasVerticesList[i];
							auto& vertices = Scene.colormappedBufferList[index];
							VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices.buffer->buffer, &vertices.offset));
							index = Scene.aliasTexCoordsList[i];
							auto& texCoords = Scene.colormappedBufferList[index];
							VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &texCoords.buffer->buffer, &texCoords.offset));
							VkDeviceSize attributeOffset = perImage.colormappedAttributeBase + alias.first_attribute * sizeof(float);
							VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
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
							VC(Device.vkCmdPushConstants(perImage.commandBuffer, Scene.alias.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants));
							VkDescriptorSet descriptorSet;
							auto texture = Scene.aliasList[i].texture.texture;
							auto entry = perImage.aliasResources.cache.find(texture);
							if (entry == perImage.aliasResources.cache.end())
							{
								descriptorSet = perImage.aliasResources.descriptorSets[perImage.aliasResources.index];
								perImage.aliasResources.index++;
								textureInfo[0].sampler = Scene.samplers[texture->mipCount];
								textureInfo[0].imageView = texture->view;
								writes[0].dstSet = descriptorSet;
								VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
								perImage.aliasResources.cache.insert({ texture, descriptorSet });
							}
							else
							{
								descriptorSet = entry->second;
							}
							VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.alias.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
							if (alias.is_host_colormap)
							{
								VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.alias.pipelineLayout, 2, 1, &perImage.host_colormapResources.descriptorSet, 0, nullptr));
							}
							else
							{
								auto colormap = Scene.aliasList[i].colormap.texture;
								if (perImage.colormapResources.bound[descriptorSetIndex] != colormap)
								{
									textureInfo[0].sampler = Scene.samplers[colormap->mipCount];
									textureInfo[0].imageView = colormap->view;
									writes[0].dstSet = perImage.colormapResources.descriptorSets[descriptorSetIndex];
									VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
									perImage.colormapResources.bound[descriptorSetIndex] = texture;
								}
								VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.alias.pipelineLayout, 2, 1, &perImage.colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr));
								descriptorSetIndex++;
							}
							VC(Device.vkCmdDrawIndexed(perImage.commandBuffer, alias.count, 1, alias.first_index32, 0, 0));
						}
					}
				}
				if (perImage.resetDescriptorSetsCount != Scene.resetDescriptorSetsCount || perImage.viewmodelResources.descriptorSets.size() < Scene.viewmodelTextureCount)
				{
					perImage.viewmodelResources.Delete(*this);
					viewmodelDescriptorSetCount = Scene.viewmodelTextureCount;
					if (viewmodelDescriptorSetCount > 0)
					{
						poolSizes[0].descriptorCount = viewmodelDescriptorSetCount;
						descriptorPoolCreateInfo.maxSets = viewmodelDescriptorSetCount;
						VK(Device.vkCreateDescriptorPool(Device.device, &descriptorPoolCreateInfo, nullptr, &perImage.viewmodelResources.descriptorPool));
						descriptorSetAllocateInfo.descriptorPool = perImage.viewmodelResources.descriptorPool;
						descriptorSetAllocateInfo.pSetLayouts = &Scene.singleImageLayout;
						perImage.viewmodelResources.descriptorSets.resize(viewmodelDescriptorSetCount);
						for (auto i = 0; i < viewmodelDescriptorSetCount; i++)
						{
							VK(Device.vkAllocateDescriptorSets(Device.device, &descriptorSetAllocateInfo, &perImage.viewmodelResources.descriptorSets[i]));
						}
						perImage.viewmodelResources.created = true;
					}
				}
				if (d_lists.last_viewmodel >= 0)
				{
					VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 3, 1, &attributes->buffer, &perImage.vertexTransformBase));
					VC(Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.viewmodel.pipeline));
					VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.viewmodel.pipelineLayout, 0, 1, &perImage.sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
					pushConstants[3] = 0;
					pushConstants[7] = 0;
					pushConstants[11] = 0;
					pushConstants[15] = 1;
					if (NearViewModel)
					{
						pushConstants[16] = vpn[0] / Scale;
						pushConstants[17] = vpn[2] / Scale;
						pushConstants[18] = -vpn[1] / Scale;
						pushConstants[19] = 0;
						pushConstants[20] = 1;
						pushConstants[21] = 1;
						pushConstants[22] = 1;
						pushConstants[23] = 1;
					}
					else
					{
						pushConstants[16] = 1 / Scale;
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
						VC(Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices16->buffer, perImage.colormappedIndex16Base, VK_INDEX_TYPE_UINT16));
						for (auto i = 0; i <= d_lists.last_viewmodel; i++)
						{
							auto& viewmodel = d_lists.viewmodel[i];
							if (viewmodel.first_index16 < 0)
							{
								continue;
							}
							auto index = Scene.viewmodelVerticesList[i];
							auto& vertices = Scene.colormappedBufferList[index];
							VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices.buffer->buffer, &vertices.offset));
							index = Scene.viewmodelTexCoordsList[i];
							auto& texCoords = Scene.colormappedBufferList[index];
							VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &texCoords.buffer->buffer, &texCoords.offset));
							VkDeviceSize attributeOffset = perImage.colormappedAttributeBase + viewmodel.first_attribute * sizeof(float);
							VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
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
							VC(Device.vkCmdPushConstants(perImage.commandBuffer, Scene.viewmodel.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 24 * sizeof(float), pushConstants));
							VkDescriptorSet descriptorSet;
							auto texture = Scene.viewmodelList[i].texture.texture;
							auto entry = perImage.viewmodelResources.cache.find(texture);
							if (entry == perImage.viewmodelResources.cache.end())
							{
								descriptorSet = perImage.viewmodelResources.descriptorSets[perImage.viewmodelResources.index];
								perImage.viewmodelResources.index++;
								textureInfo[0].sampler = Scene.samplers[texture->mipCount];
								textureInfo[0].imageView = texture->view;
								writes[0].dstSet = descriptorSet;
								VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
								perImage.viewmodelResources.cache.insert({ texture, descriptorSet });
							}
							else
							{
								descriptorSet = entry->second;
							}
							VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.viewmodel.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
							if (viewmodel.is_host_colormap)
							{
								VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.viewmodel.pipelineLayout, 2, 1, &perImage.host_colormapResources.descriptorSet, 0, nullptr));
							}
							else
							{
								auto colormap = Scene.viewmodelList[i].colormap.texture;
								if (perImage.colormapResources.bound[descriptorSetIndex] != colormap)
								{
									textureInfo[0].sampler = Scene.samplers[colormap->mipCount];
									textureInfo[0].imageView = colormap->view;
									writes[0].dstSet = perImage.colormapResources.descriptorSets[descriptorSetIndex];
									VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
									perImage.colormapResources.bound[descriptorSetIndex] = texture;
								}
								VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.viewmodel.pipelineLayout, 2, 1, &perImage.colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr));
								descriptorSetIndex++;
							}
							VC(Device.vkCmdDrawIndexed(perImage.commandBuffer, viewmodel.count, 1, viewmodel.first_index16, 0, 0));
						}
					}
					if (indices32 != nullptr)
					{
						VC(Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32));
						for (auto i = 0; i <= d_lists.last_viewmodel; i++)
						{
							auto& viewmodel = d_lists.viewmodel[i];
							if (viewmodel.first_index32 < 0)
							{
								continue;
							}
							auto index = Scene.viewmodelVerticesList[i];
							auto& vertices = Scene.colormappedBufferList[index];
							VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices.buffer->buffer, &vertices.offset));
							index = Scene.viewmodelTexCoordsList[i];
							auto& texCoords = Scene.colormappedBufferList[index];
							VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &texCoords.buffer->buffer, &texCoords.offset));
							VkDeviceSize attributeOffset = perImage.colormappedAttributeBase + viewmodel.first_attribute * sizeof(float);
							VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
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
							VC(Device.vkCmdPushConstants(perImage.commandBuffer, Scene.viewmodel.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 24 * sizeof(float), pushConstants));
							VkDescriptorSet descriptorSet;
							auto texture = Scene.viewmodelList[i].texture.texture;
							auto entry = perImage.viewmodelResources.cache.find(texture);
							if (entry == perImage.viewmodelResources.cache.end())
							{
								descriptorSet = perImage.viewmodelResources.descriptorSets[perImage.viewmodelResources.index];
								perImage.viewmodelResources.index++;
								textureInfo[0].sampler = Scene.samplers[texture->mipCount];
								textureInfo[0].imageView = texture->view;
								writes[0].dstSet = descriptorSet;
								VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
								perImage.viewmodelResources.cache.insert({ texture, descriptorSet });
							}
							else
							{
								descriptorSet = entry->second;
							}
							VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.viewmodel.pipelineLayout, 1, 1, &descriptorSet, 0, nullptr));
							if (viewmodel.is_host_colormap)
							{
								VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.viewmodel.pipelineLayout, 2, 1, &perImage.host_colormapResources.descriptorSet, 0, nullptr));
							}
							else
							{
								auto colormap = Scene.viewmodelList[i].colormap.texture;
								if (perImage.colormapResources.bound[descriptorSetIndex] != colormap)
								{
									textureInfo[0].sampler = Scene.samplers[colormap->mipCount];
									textureInfo[0].imageView = colormap->view;
									writes[0].dstSet = perImage.colormapResources.descriptorSets[descriptorSetIndex];
									VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
									perImage.colormapResources.bound[descriptorSetIndex] = texture;
								}
								VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.viewmodel.pipelineLayout, 2, 1, &perImage.colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr));
								descriptorSetIndex++;
							}
							VC(Device.vkCmdDrawIndexed(perImage.commandBuffer, viewmodel.count, 1, viewmodel.first_index32, 0, 0));
						}
					}
				}
				perImage.resetDescriptorSetsCount = Scene.resetDescriptorSetsCount;
				if (d_lists.last_particle >= 0)
				{
					VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer, &perImage.coloredVertexBase));
					VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &attributes->buffer, &perImage.vertexTransformBase));
					VC(Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.colored.pipeline));
					VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.colored.pipelineLayout, 0, 1, &perImage.sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr));
					if (indices16 != nullptr)
					{
						VC(Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices16->buffer, perImage.coloredIndex16Base, VK_INDEX_TYPE_UINT16));
						for (auto i = 0; i <= d_lists.last_particle; i++)
						{
							auto& particle = d_lists.particles[i];
							if (particle.first_index16 < 0)
							{
								continue;
							}
							float color = (float)particle.color;
							VC(Device.vkCmdPushConstants(perImage.commandBuffer, Scene.colored.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &color));
							VC(Device.vkCmdDrawIndexed(perImage.commandBuffer, particle.count, 1, particle.first_index16, 0, 0));
						}
					}
					if (indices32 != nullptr)
					{
						VC(Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices32->buffer, perImage.coloredIndex32Base, VK_INDEX_TYPE_UINT32));
						for (auto i = 0; i <= d_lists.last_particle; i++)
						{
							auto& particle = d_lists.particles[i];
							if (particle.first_index32 < 0)
							{
								continue;
							}
							float color = (float)particle.color;
							VC(Device.vkCmdPushConstants(perImage.commandBuffer, Scene.colored.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &color));
							VC(Device.vkCmdDrawIndexed(perImage.commandBuffer, particle.count, 1, particle.first_index32, 0, 0));
						}
					}
				}
				if (d_lists.last_sky >= 0)
				{
					VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer, &perImage.texturedVertexBase));
					VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &attributes->buffer, &perImage.texturedAttributeBase));
					VC(Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.sky.pipeline));
					poolSizes[0].descriptorCount = 1;
					descriptorPoolCreateInfo.poolSizeCount = 1;
					if (!perImage.skyResources.created)
					{
						VK(Device.vkCreateDescriptorPool(Device.device, &descriptorPoolCreateInfo, nullptr, &perImage.skyResources.descriptorPool));
						descriptorSetAllocateInfo.descriptorPool = perImage.skyResources.descriptorPool;
						descriptorSetAllocateInfo.pSetLayouts = &Scene.singleImageLayout;
						VK(Device.vkAllocateDescriptorSets(Device.device, &descriptorSetAllocateInfo, &perImage.skyResources.descriptorSet));
						textureInfo[0].sampler = Scene.samplers[perImage.sky->mipCount];
						textureInfo[0].imageView = perImage.sky->view;
						writes[0].dstSet = perImage.skyResources.descriptorSet;
						VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
						perImage.skyResources.created = true;
					}
					VkDescriptorSet descriptorSets[2];
					descriptorSets[0] = perImage.sceneMatricesAndPaletteResources.descriptorSet;
					descriptorSets[1] = perImage.skyResources.descriptorSet;
					VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.sky.pipelineLayout, 0, 2, descriptorSets, 0, nullptr));
					float rotation[13];
					auto matrix = ovrMatrix4f_CreateFromQuaternion(&orientation);
					rotation[0] = -matrix.M[0][2];
					rotation[1] = matrix.M[2][2];
					rotation[2] = -matrix.M[1][2];
					rotation[3] = matrix.M[0][0];
					rotation[4] = -matrix.M[2][0];
					rotation[5] = matrix.M[1][0];
					rotation[6] = matrix.M[0][1];
					rotation[7] = -matrix.M[2][1];
					rotation[8] = matrix.M[1][1];
					rotation[9] = eyeTextureWidth;
					rotation[10] = eyeTextureHeight;
					rotation[11] = std::max(eyeTextureWidth, eyeTextureHeight);
					rotation[12] = skytime*skyspeed;
					VC(Device.vkCmdPushConstants(perImage.commandBuffer, Scene.sky.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 13 * sizeof(float), &rotation));
					for (auto i = 0; i <= d_lists.last_sky; i++)
					{
						auto& sky = d_lists.sky[i];
						VC(Device.vkCmdDraw(perImage.commandBuffer, sky.count, 1, sky.first_vertex, 0));
					}
				}
			}
			else
			{
				VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer, &noOffset));
				VC(Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &attributes->buffer, &noOffset));
				VC(Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.floor.pipeline));
				if (!perImage.floorResources.created)
				{
					poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					poolSizes[0].descriptorCount = 1;
					descriptorPoolCreateInfo.poolSizeCount = 1;
					VK(Device.vkCreateDescriptorPool(Device.device, &descriptorPoolCreateInfo, nullptr, &perImage.floorResources.descriptorPool));
					descriptorSetAllocateInfo.descriptorPool = perImage.floorResources.descriptorPool;
					descriptorSetAllocateInfo.pSetLayouts = &Scene.singleImageLayout;
					VK(Device.vkAllocateDescriptorSets(Device.device, &descriptorSetAllocateInfo, &perImage.floorResources.descriptorSet));
					textureInfo[0].sampler = Scene.samplers[Scene.floorTexture->mipCount];
					textureInfo[0].imageView = Scene.floorTexture->view;
					writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					writes[0].pImageInfo = textureInfo;
					writes[0].dstSet = perImage.floorResources.descriptorSet;
					VC(Device.vkUpdateDescriptorSets(Device.device, 1, writes, 0, nullptr));
					perImage.floorResources.created = true;
				}
				VkDescriptorSet descriptorSets[2];
				descriptorSets[0] = perImage.sceneMatricesResources.descriptorSet;
				descriptorSets[1] = perImage.floorResources.descriptorSet;
				VC(Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Scene.floor.pipelineLayout, 0, 2, descriptorSets, 0, nullptr));
				VC(Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices16->buffer, 0, VK_INDEX_TYPE_UINT16));
				VC(Device.vkCmdDrawIndexed(perImage.commandBuffer, 6, 1, 0, 0, 0));
			}
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
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "**** [%i, %i, %i, %i, %i] %i; %i, %i, %i = %i; %i; %i; %i, %i, %i, %i = %i; %i, %i, %i = %i; %i, %i = %i",
								texturedDescriptorSetCount, spriteDescriptorSetCount, colormapDescriptorSetCount, aliasDescriptorSetCount, viewmodelDescriptorSetCount,
								stagingBufferSize,
								floorVerticesSize, texturedVerticesSize, coloredVerticesSize, verticesSize,
								colormappedVerticesSize,
								colormappedTexCoordsSize,
								floorAttributesSize, texturedAttributesSize, colormappedLightsSize, vertexTransformSize, attributesSize,
								floorIndicesSize, colormappedIndices16Size, coloredIndices16Size, indices16Size,
								colormappedIndices32Size, coloredIndices32Size, indices32Size);
#endif
	}
}
