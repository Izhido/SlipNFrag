#include "Buffer.h"
#include "AppState.h"
#include "MemoryAllocateInfo.h"
#include "VulkanCallWrappers.h"

void Buffer::Create(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage)
{
	this->size = size;
	VkBufferCreateInfo bufferCreateInfo { };
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK(appState.Device.vkCreateBuffer(appState.Device.device, &bufferCreateInfo, nullptr, &buffer));
	flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	VkMemoryRequirements memoryRequirements;
	VC(appState.Device.vkGetBufferMemoryRequirements(appState.Device.device, buffer, &memoryRequirements));
	VkMemoryAllocateInfo memoryAllocateInfo { };
	createMemoryAllocateInfo(appState, memoryRequirements, flags, memoryAllocateInfo);
	VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryAllocateInfo, nullptr, &memory));
	VK(appState.Device.vkBindBufferMemory(appState.Device.device, buffer, memory, 0));
}

void Buffer::CreateVertexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void Buffer::CreateIndexBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void Buffer::CreateStagingBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
}

void Buffer::CreateStagingStorageBuffer(AppState& appState, VkDeviceSize size)
{
	Create(appState, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void Buffer::Submit(AppState& appState, VkCommandBuffer commandBuffer, VkAccessFlags flags, VkBufferMemoryBarrier& bufferMemoryBarrier)
{
	bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	bufferMemoryBarrier.dstAccessMask = flags;
	bufferMemoryBarrier.buffer = buffer;
	bufferMemoryBarrier.size = size;
	VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
}

void Buffer::SubmitVertexBuffer(AppState& appState, VkCommandBuffer commandBuffer, VkBufferMemoryBarrier& bufferMemoryBarrier)
{
	Submit(appState, commandBuffer, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, bufferMemoryBarrier);
}

void Buffer::SubmitIndexBuffer(AppState& appState, VkCommandBuffer commandBuffer, VkBufferMemoryBarrier& bufferMemoryBarrier)
{
	Submit(appState, commandBuffer, VK_ACCESS_INDEX_READ_BIT, bufferMemoryBarrier);
}
