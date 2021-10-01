#pragma once

struct ScreenPerImage
{
	VkImage image;
	Buffer stagingBuffer;
	VkCommandBuffer commandBuffer;
	VkSubmitInfo submitInfo;
};
