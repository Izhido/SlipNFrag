#pragma once

struct Screen
{
	ovrTextureSwapChain* SwapChain;
	std::vector<uint32_t> Data;
	VkImage Image;
	Buffer Buffer;
	VkCommandBuffer CommandBuffer;
	VkSubmitInfo SubmitInfo;
};
