#include "Lightmap.h"
#include "AppState.h"
#include "Utils.h"
#include "Constants.h"

bool Lightmap::Create(AppState& appState, uint32_t width, uint32_t height, bool variable)
{
	this->width = width;
	this->height = height;
	this->variable = variable;

	auto size = this->width * this->height;

	auto& buffers = (variable ? appState.Scene.variableLightmapBuffers : appState.Scene.staticLightmapBuffers);
	auto lightmapBuffer = buffers.buffers;
	bool found = false;
	while (lightmapBuffer != nullptr)
	{
		if (lightmapBuffer->used + size <= lightmapBuffer->size)
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
			lightmapBuffer = new LightmapBuffer { };
			buffers.buffers = lightmapBuffer;
		}
		else
		{
			lightmapBuffer->next = new LightmapBuffer { };
			lightmapBuffer->next->previous = lightmapBuffer;
			lightmapBuffer = lightmapBuffer->next;
		}
		buffers.count++;

		buffer = lightmapBuffer;

		buffer->size = Constants::lightmapBufferSize * std::min(256, 1 << (buffers.count / 4));
		if (buffer->size < size)
		{
			buffer->size = size;
		}

		buffer->buffer.CreateMappableStorageBuffer(appState, buffer->size * sizeof(uint32_t));

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

		return true;
	}

	return false;
}

void Lightmap::Delete(AppState& appState) const
{
	buffer->referenceCount--;
	if (buffer->referenceCount == 0)
	{
		vkDestroyDescriptorPool(appState.Device, buffer->descriptorPool, nullptr);
		buffer->buffer.Delete(appState);

		auto& buffers = (variable ? appState.Scene.variableLightmapBuffers : appState.Scene.staticLightmapBuffers);
		buffers.count--;

		if (buffer->previous != nullptr)
		{
			buffer->previous->next = buffer->next;
		}
		else
		{
			buffers.buffers = buffer->next;
		}
		if (buffer->next != nullptr)
		{
			buffer->next->previous = buffer->previous;
		}
		delete buffer;
	}
}
