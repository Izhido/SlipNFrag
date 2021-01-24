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
		Scene.ClearBuffersAndSizes();
		if (Mode != AppWorldMode)
		{
			Scene.floorVerticesSize = 3 * 4 * sizeof(float);
		}
		Scene.texturedVerticesSize = (d_lists.last_textured_vertex + 1) * sizeof(float);
		Scene.coloredVerticesSize = (d_lists.last_colored_vertex + 1) * sizeof(float);
		Scene.verticesSize = Scene.texturedVerticesSize + Scene.coloredVerticesSize + Scene.floorVerticesSize;
		if (Scene.verticesSize > 0 || d_lists.last_alias >= 0 || d_lists.last_viewmodel >= 0)
		{
			for (Buffer** b = &perImage.vertices.oldBuffers; *b != nullptr; b = &(*b)->next)
			{
				if ((*b)->size >= Scene.verticesSize && (*b)->size < Scene.verticesSize * 2)
				{
					Scene.vertices = *b;
					*b = (*b)->next;
					break;
				}
			}
			if (Scene.vertices == nullptr)
			{
				Scene.vertices = new Buffer();
				Scene.vertices->CreateVertexBuffer(*this, Scene.verticesSize + Scene.verticesSize / 4);
			}
			perImage.vertices.MoveToFront(Scene.vertices);
			VK(Device.vkMapMemory(Device.device, Scene.vertices->memory, 0, Scene.verticesSize, 0, &Scene.vertices->mapped));
			if (Scene.floorVerticesSize > 0)
			{
				auto mapped = (float*)Scene.vertices->mapped;
				(*mapped) = -0.5;
				mapped++;
				(*mapped) = Scene.pose.Position.y;
				mapped++;
				(*mapped) = -0.5;
				mapped++;
				(*mapped) = 0.5;
				mapped++;
				(*mapped) = Scene.pose.Position.y;
				mapped++;
				(*mapped) = -0.5;
				mapped++;
				(*mapped) = 0.5;
				mapped++;
				(*mapped) = Scene.pose.Position.y;
				mapped++;
				(*mapped) = 0.5;
				mapped++;
				(*mapped) = -0.5;
				mapped++;
				(*mapped) = Scene.pose.Position.y;
				mapped++;
				(*mapped) = 0.5;
			}
			perImage.texturedVertexBase = Scene.floorVerticesSize;
			memcpy((unsigned char*)Scene.vertices->mapped + perImage.texturedVertexBase, d_lists.textured_vertices.data(), Scene.texturedVerticesSize);
			perImage.coloredVertexBase = perImage.texturedVertexBase + Scene.texturedVerticesSize;
			memcpy((unsigned char*)Scene.vertices->mapped + perImage.coloredVertexBase, d_lists.colored_vertices.data(), Scene.coloredVerticesSize);
			VC(Device.vkUnmapMemory(Device.device, Scene.vertices->memory));
			Scene.vertices->mapped = nullptr;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferMemoryBarrier.buffer = Scene.vertices->buffer;
			bufferMemoryBarrier.size = Scene.vertices->size;
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
				Scene.colormappedVerticesSize += verticesOffset;
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
				Scene.colormappedTexCoordsSize += texCoordsOffset;
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
				Scene.colormappedVerticesSize += verticesOffset;
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
				Scene.colormappedTexCoordsSize += texCoordsOffset;
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
				Scene.floorAttributesSize = 2 * 4 * sizeof(float);
			}
			Scene.texturedAttributesSize = (d_lists.last_textured_attribute + 1) * sizeof(float);
			Scene.colormappedLightsSize = (d_lists.last_colormapped_attribute + 1) * sizeof(float);
			Scene.vertexTransformSize = 16 * sizeof(float);
			Scene.attributesSize = Scene.floorAttributesSize + Scene.texturedAttributesSize + Scene.colormappedLightsSize + Scene.vertexTransformSize;
			for (Buffer** b = &perImage.attributes.oldBuffers; *b != nullptr; b = &(*b)->next)
			{
				if ((*b)->size >= Scene.attributesSize && (*b)->size < Scene.attributesSize * 2)
				{
					Scene.attributes = *b;
					*b = (*b)->next;
					break;
				}
			}
			if (Scene.attributes == nullptr)
			{
				Scene.attributes = new Buffer();
				Scene.attributes->CreateVertexBuffer(*this, Scene.attributesSize + Scene.attributesSize / 4);
			}
			perImage.attributes.MoveToFront(Scene.attributes);
			VK(Device.vkMapMemory(Device.device, Scene.attributes->memory, 0, Scene.attributesSize, 0, &Scene.attributes->mapped));
			if (Scene.floorAttributesSize > 0)
			{
				auto mapped = (float*)Scene.attributes->mapped;
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
			perImage.texturedAttributeBase = Scene.floorAttributesSize;
			memcpy((unsigned char*)Scene.attributes->mapped + perImage.texturedAttributeBase, d_lists.textured_attributes.data(), Scene.texturedAttributesSize);
			perImage.colormappedAttributeBase = perImage.texturedAttributeBase + Scene.texturedAttributesSize;
			memcpy((unsigned char*)Scene.attributes->mapped + perImage.colormappedAttributeBase, d_lists.colormapped_attributes.data(), Scene.colormappedLightsSize);
			perImage.vertexTransformBase = perImage.colormappedAttributeBase + Scene.colormappedLightsSize;
			auto mapped = (float*)Scene.attributes->mapped + perImage.vertexTransformBase / sizeof(float);
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
			VC(Device.vkUnmapMemory(Device.device, Scene.attributes->memory));
			Scene.attributes->mapped = nullptr;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferMemoryBarrier.buffer = Scene.attributes->buffer;
			bufferMemoryBarrier.size = Scene.attributes->size;
			VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			if (Mode != AppWorldMode)
			{
				Scene.floorIndicesSize = 6 * sizeof(uint16_t);
			}
			Scene.colormappedIndices16Size = (d_lists.last_colormapped_index16 + 1) * sizeof(uint16_t);
			Scene.coloredIndices16Size = (d_lists.last_colored_index16 + 1) * sizeof(uint16_t);
			Scene.indices16Size = Scene.floorIndicesSize + Scene.colormappedIndices16Size + Scene.coloredIndices16Size;
			if (Scene.indices16Size > 0)
			{
				for (Buffer** b = &perImage.indices16.oldBuffers; *b != nullptr; b = &(*b)->next)
				{
					if ((*b)->size >= Scene.indices16Size && (*b)->size < Scene.indices16Size * 2)
					{
						Scene.indices16 = *b;
						*b = (*b)->next;
						break;
					}
				}
				if (Scene.indices16 == nullptr)
				{
					Scene.indices16 = new Buffer();
					Scene.indices16->CreateIndexBuffer(*this, Scene.indices16Size + Scene.indices16Size / 4);
				}
				perImage.indices16.MoveToFront(Scene.indices16);
				VK(Device.vkMapMemory(Device.device, Scene.indices16->memory, 0, Scene.indices16Size, 0, &Scene.indices16->mapped));
				if (Scene.floorIndicesSize > 0)
				{
					auto mapped = (uint16_t*)Scene.indices16->mapped;
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
				perImage.colormappedIndex16Base = Scene.floorIndicesSize;
				memcpy((unsigned char*)Scene.indices16->mapped + perImage.colormappedIndex16Base, d_lists.colormapped_indices16.data(), Scene.colormappedIndices16Size);
				perImage.coloredIndex16Base = perImage.colormappedIndex16Base + Scene.colormappedIndices16Size;
				memcpy((unsigned char*)Scene.indices16->mapped + perImage.coloredIndex16Base, d_lists.colored_indices16.data(), Scene.coloredIndices16Size);
				VC(Device.vkUnmapMemory(Device.device, Scene.indices16->memory));
				Scene.indices16->mapped = nullptr;
				bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				bufferMemoryBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
				bufferMemoryBarrier.buffer = Scene.indices16->buffer;
				bufferMemoryBarrier.size = Scene.indices16->size;
				VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			}
			Scene.colormappedIndices32Size = (d_lists.last_colormapped_index32 + 1) * sizeof(uint32_t);
			Scene.coloredIndices32Size = (d_lists.last_colored_index32 + 1) * sizeof(uint32_t);
			Scene.indices32Size = Scene.colormappedIndices32Size + Scene.coloredIndices32Size;
			if (Scene.indices32Size > 0)
			{
				for (Buffer** b = &perImage.indices32.oldBuffers; *b != nullptr; b = &(*b)->next)
				{
					if ((*b)->size >= Scene.indices32Size && (*b)->size < Scene.indices32Size * 2)
					{
						Scene.indices32 = *b;
						*b = (*b)->next;
						break;
					}
				}
				if (Scene.indices32 == nullptr)
				{
					Scene.indices32 = new Buffer();
					Scene.indices32->CreateIndexBuffer(*this, Scene.indices32Size + Scene.indices32Size / 4);
				}
				perImage.indices32.MoveToFront(Scene.indices32);
				VK(Device.vkMapMemory(Device.device, Scene.indices32->memory, 0, Scene.indices32Size, 0, &Scene.indices32->mapped));
				memcpy(Scene.indices32->mapped, d_lists.colormapped_indices32.data(), Scene.colormappedIndices32Size);
				perImage.coloredIndex32Base = Scene.colormappedIndices32Size;
				memcpy((unsigned char*)Scene.indices32->mapped + perImage.coloredIndex32Base, d_lists.colored_indices32.data(), Scene.coloredIndices32Size);
				VC(Device.vkUnmapMemory(Device.device, Scene.indices32->memory));
				Scene.indices32->mapped = nullptr;
				bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				bufferMemoryBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
				bufferMemoryBarrier.buffer = Scene.indices32->buffer;
				bufferMemoryBarrier.size = Scene.indices32->size;
				VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			}
			for (auto i = 0; i <= d_lists.last_colored_surfaces_index16; i++)
			{
				Scene.coloredSurfaces16Size += (d_lists.colored_surfaces_index16[i].last_color + 1) * sizeof(float);
			}
			for (auto i = 0; i <= d_lists.last_colored_surfaces_index32; i++)
			{
				Scene.coloredSurfaces32Size += (d_lists.colored_surfaces_index32[i].last_color + 1) * sizeof(float);
			}
			for (auto i = 0; i <= d_lists.last_particles_index16; i++)
			{
				Scene.particles16Size += (d_lists.particles_index16[i].last_color + 1) * sizeof(float);
			}
			for (auto i = 0; i <= d_lists.last_particles_index32; i++)
			{
				Scene.particles32Size += (d_lists.particles_index32[i].last_color + 1) * sizeof(float);
			}
			Scene.colorsSize = Scene.coloredSurfaces16Size + Scene.coloredSurfaces32Size + Scene.particles16Size + Scene.particles32Size;
			if (Scene.colorsSize > 0)
			{
				for (Buffer** b = &perImage.colors.oldBuffers; *b != nullptr; b = &(*b)->next)
				{
					if ((*b)->size >= Scene.colorsSize && (*b)->size < Scene.colorsSize * 2)
					{
						Scene.colors = *b;
						*b = (*b)->next;
						break;
					}
				}
				if (Scene.colors == nullptr)
				{
					Scene.colors = new Buffer();
					Scene.colors->CreateVertexBuffer(*this, Scene.colorsSize + Scene.colorsSize / 4);
				}
				perImage.colors.MoveToFront(Scene.colors);
				VK(Device.vkMapMemory(Device.device, Scene.colors->memory, 0, Scene.colorsSize, 0, &Scene.colors->mapped));
				auto offset = 0;
				for (auto i = 0; i <= d_lists.last_colored_surfaces_index16; i++)
				{
					auto& coloredSurfaces = d_lists.colored_surfaces_index16[i];
					auto count = (coloredSurfaces.last_color + 1) * sizeof(float);
					memcpy((unsigned char*)Scene.colors->mapped + offset, coloredSurfaces.colors.data(), count);
					offset += count;
				}
				for (auto i = 0; i <= d_lists.last_colored_surfaces_index32; i++)
				{
					auto& coloredSurfaces = d_lists.colored_surfaces_index32[i];
					auto count = (coloredSurfaces.last_color + 1) * sizeof(float);
					memcpy((unsigned char*)Scene.colors->mapped + offset, coloredSurfaces.colors.data(), count);
					offset += count;
				}
				for (auto i = 0; i <= d_lists.last_particles_index16; i++)
				{
					auto& particles = d_lists.particles_index16[i];
					auto count = (particles.last_color + 1) * sizeof(float);
					memcpy((unsigned char*)Scene.colors->mapped + offset, particles.colors.data(), count);
					offset += count;
				}
				for (auto i = 0; i <= d_lists.last_particles_index32; i++)
				{
					auto& particles = d_lists.particles_index32[i];
					auto count = (particles.last_color + 1) * sizeof(float);
					memcpy((unsigned char*)Scene.colors->mapped + offset, particles.colors.data(), count);
					offset += count;
				}
				VC(Device.vkUnmapMemory(Device.device, Scene.colors->memory));
				Scene.colors->mapped = nullptr;
				bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				bufferMemoryBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
				bufferMemoryBarrier.buffer = Scene.colors->buffer;
				bufferMemoryBarrier.size = Scene.colors->size;
				VC(Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			}
		}
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
