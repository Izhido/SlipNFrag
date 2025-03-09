#include "LightmapRGB.h"
#include "AppState.h"
#include "Utils.h"
#include "MemoryAllocateInfo.h"
#include "Constants.h"

void LightmapRGB::Create(AppState& appState, uint32_t width, uint32_t height)
{
	this->width = width;
	this->height = height;

	auto slot0 = std::ceil(std::log2(width));
	auto slot1 = std::ceil(std::log2(height));
	auto slot = (size_t)std::log2(std::pow(2, slot0) * std::pow(2, slot1));
	if (appState.Scene.lightmapRGBBuffers.size() <= slot)
	{
		appState.Scene.lightmapRGBBuffers.resize(slot + 1);
	}
	auto lightmapBuffer = appState.Scene.lightmapRGBBuffers[slot];
	bool found = false;
	while (lightmapBuffer != nullptr)
	{
		if (lightmapBuffer->allocatedCount < lightmapBuffer->allocated.size())
		{
			for (auto i = lightmapBuffer->firstFreeCandidate; i < lightmapBuffer->allocated.size(); i++)
			{
				if (!lightmapBuffer->allocated[i])
				{
					lightmapBuffer->allocated[i] = true;
					buffer = lightmapBuffer;
					allocatedIndex = i;
					offset = allocatedIndex * lightmapBuffer->size;
					lightmapBuffer->allocatedCount++;
					lightmapBuffer->firstFreeCandidate = i + 1;
					found = true;
					break;
				}
			}
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
			appState.Scene.lightmapRGBBuffers[slot] = lightmapBuffer;
		}
		else
		{
			lightmapBuffer->next = new LightmapRGBBuffer { };
			lightmapBuffer->next->previous = lightmapBuffer;
			lightmapBuffer = lightmapBuffer->next;
		}
		buffer = lightmapBuffer;

		buffer->width = (int)std::pow(2, slot0);
		buffer->height = (int)std::pow(2, slot1);

		auto maxSlot = std::max(slot0, slot1);
		size_t arrayLayers;
		if (maxSlot <= 3)
		{
			arrayLayers = 256;
		}
		else if (maxSlot >= 11)
		{
			arrayLayers = 1;
		}
		else
		{
			arrayLayers = (size_t)std::pow(2, 11 - maxSlot);
		}

		buffer->size = buffer->width * buffer->height * 3;

		buffer->buffer.CreateStorageBuffer(appState, buffer->size * arrayLayers * sizeof(uint32_t));

		buffer->allocated.resize(arrayLayers);

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

		buffer->allocated[0] = true;

		buffer->allocatedCount++;

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
	buffer->allocated[allocatedIndex] = false;
	buffer->allocatedCount--;
	if (buffer->firstFreeCandidate > allocatedIndex)
	{
		buffer->firstFreeCandidate = allocatedIndex;
	}
	if (buffer->allocatedCount == 0)
	{
		vkDestroyDescriptorPool(appState.Device, buffer->descriptorPool, nullptr);
		buffer->buffer.Delete(appState);
		if (buffer->previous != nullptr)
		{
			buffer->previous->next = buffer->next;
		}
		else
		{
			auto slot0 = std::ceil(std::log2(width));
			auto slot1 = std::ceil(std::log2(height));
			auto slot = (size_t)std::log2(std::pow(2, slot0) * std::pow(2, slot1));
			appState.Scene.lightmapRGBBuffers[slot] = buffer->next;
		}
		if (buffer->next != nullptr)
		{
			buffer->next->previous = buffer->previous;
		}
		delete buffer;
	}
}
