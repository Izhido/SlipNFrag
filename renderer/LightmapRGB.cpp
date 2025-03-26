#include "LightmapRGB.h"
#include "AppState.h"
#include "Utils.h"
#include "MemoryAllocateInfo.h"
#include "Constants.h"

void LightmapRGB::Create(AppState& appState, uint32_t width, uint32_t height, void* texture)
{
	this->width = width;
	this->height = height;
	this->texture = texture;

	auto size = this->width * this->height * 3;

	auto& perTexture = appState.Scene.lightmapRGBBuffers[this->texture];
	auto lightmapBuffer = perTexture.buffers;
	bool found = false;
	while (lightmapBuffer != nullptr)
	{
		if (lightmapBuffer->used + size < lightmapBuffer->size)
		{
			buffer = lightmapBuffer;
			offset = lightmapBuffer->used;
			lightmapBuffer->used += size;
			lightmapBuffer->referenceCount++;
			found = true;
			break;
		}
		if (lightmapBuffer->next == nullptr)
		{
			break;
		}
		lightmapBuffer = lightmapBuffer->next;
	}

	if (!found)
	{
		if (lightmapBuffer == nullptr)
		{
			lightmapBuffer = new LightmapRGBBuffer { };
			perTexture.buffers = lightmapBuffer;
		}
		else
		{
			lightmapBuffer->next = new LightmapRGBBuffer { };
			lightmapBuffer->next->previous = lightmapBuffer;
			lightmapBuffer = lightmapBuffer->next;
		}
		buffer = lightmapBuffer;

		buffer->size = Constants::lightmapBufferSize;
		if (buffer->size < size)
		{
			buffer->size = size;
		}

		buffer->buffer.CreateStorageBuffer(appState, buffer->size * sizeof(uint32_t));

		VkDescriptorPoolSize poolSizes { };
		poolSizes.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes.descriptorCount = 1;

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		descriptorPoolCreateInfo.maxSets = 1;
		descriptorPoolCreateInfo.pPoolSizes = &poolSizes;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &buffer->descriptorPool));

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		descriptorSetAllocateInfo.descriptorPool = buffer->descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleFragmentStorageBufferLayout;
		CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &buffer->descriptorSet));

		buffer->used = size;

		buffer->referenceCount++;

		VkDescriptorBufferInfo bufferInfo { };
		bufferInfo.range = VK_WHOLE_SIZE;
		bufferInfo.buffer = buffer->buffer.buffer;

		VkWriteDescriptorSet write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.descriptorCount = 1;
		write.dstSet = buffer->descriptorSet;
		write.pBufferInfo = &bufferInfo;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vkUpdateDescriptorSets(appState.Device, 1, &write, 0, nullptr);
	}
}

void LightmapRGB::Delete(AppState& appState) const
{
	buffer->referenceCount--;
	if (buffer->referenceCount == 0)
	{
		vkDestroyDescriptorPool(appState.Device, buffer->descriptorPool, nullptr);
		buffer->buffer.Delete(appState);
		if (buffer->previous != nullptr)
		{
			buffer->previous->next = buffer->next;
		}
		else if (buffer->next != nullptr)
		{
			appState.Scene.lightmapRGBBuffers[texture].buffers = buffer->next;
		}
		else
		{
			appState.Scene.lightmapRGBBuffers.erase(texture);
		}
		if (buffer->next != nullptr)
		{
			buffer->next->previous = buffer->previous;
		}
		delete buffer;
	}
}
