#pragma once

struct ConsolePerImage
{
	CachedBuffers vertices;
	CachedBuffers indices;
	CachedBuffers stagingBuffers;
	PipelineDescriptorResources descriptorResources;
	int paletteOffset;
	int textureOffset;
	int paletteChanged;
	Texture* palette;
	Texture* texture;
	VkCommandBuffer commandBuffer;
	VkFence fence;
	bool submitted;
};
