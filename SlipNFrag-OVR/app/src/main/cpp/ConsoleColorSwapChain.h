#pragma once

struct ConsoleColorSwapChain
{
	int SwapChainLength;
	ovrTextureSwapChain *SwapChain;
	std::vector<VkImage> ColorTextures;
};
