#pragma once

struct ColorSwapChain
{
	int SwapChainLength;
	ovrTextureSwapChain *SwapChain;
	std::vector<VkImage> ColorTextures;
	std::vector<VkImage> FragmentDensityTextures;
	std::vector<VkExtent2D> FragmentDensityTextureSizes;
};
