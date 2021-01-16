#pragma once

struct ConsoleFramebuffer
{
	int swapChainLength;
	ovrTextureSwapChain *colorTextureSwapChain;
	std::vector<Image> colorTextures;
	std::vector<VkImageMemoryBarrier> startBarriers;
	std::vector<VkImageMemoryBarrier> endBarriers;
	Image renderTexture;
	std::vector<VkFramebuffer> framebuffers;
	int width;
	int height;
};
