#pragma once

struct ScreenPerImage
{
	VkImage image;
	VkCommandBuffer commandBuffer;
	VkSubmitInfo submitInfo;
};
