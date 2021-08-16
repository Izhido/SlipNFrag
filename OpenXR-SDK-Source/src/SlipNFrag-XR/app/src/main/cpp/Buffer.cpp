#include "Buffer.h"
#include "AppState.h"
#include "MemoryAllocateInfo.h"
#include "Utils.h"

void Buffer::Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
	this->size = size;
	VkBufferCreateInfo bufferCreateInfo { };
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	CHECK_VKCMD(vkCreateBuffer(appState.Device, &bufferCreateInfo, nullptr, &buffer));
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(appState.Device, buffer, &memoryRequirements);
	VkMemoryAllocateInfo memoryAllocateInfo { };
	createMemoryAllocateInfo(appState, memoryRequirements, properties, memoryAllocateInfo);
	CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &memory));
	CHECK_VKCMD(vkBindBufferMemory(appState.Device, buffer, memory, 0));
}

void Buffer::CreateVertexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void Buffer::CreateIndexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void Buffer::CreateStagingBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void Buffer::CreateStorageBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void Buffer::CreateUniformBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void Buffer::Submit(AppState& appState, VkCommandBuffer commandBuffer, VkAccessFlags access) const
{
	VkBufferMemoryBarrier barrier { };
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	barrier.dstAccessMask = access;
	barrier.buffer = buffer;
	barrier.size = size;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

void Buffer::SubmitVertexBuffer(AppState& appState, VkCommandBuffer commandBuffer) const
{
	Submit(appState, commandBuffer, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
}

void Buffer::SubmitIndexBuffer(AppState& appState, VkCommandBuffer commandBuffer) const
{
	Submit(appState, commandBuffer, VK_ACCESS_INDEX_READ_BIT);
}

void Buffer::Delete(AppState& appState) const
{
	if (mapped != nullptr)
	{
		vkUnmapMemory(appState.Device, memory);
	}
	if (buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(appState.Device, buffer, nullptr);
	}
	if (memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(appState.Device, memory, nullptr);
	}
}
