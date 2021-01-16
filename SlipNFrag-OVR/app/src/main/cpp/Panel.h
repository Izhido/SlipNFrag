#pragma once

struct Panel
{
	ovrTextureSwapChain* SwapChain;
	std::vector<uint32_t> Data;
	VkImage Image;
	Buffer Buffer;
};
