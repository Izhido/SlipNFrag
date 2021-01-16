#pragma once

struct Framebuffer
{
	int swapChainLength;
	ovrTextureSwapChain *colorTextureSwapChain;
	std::vector<Image> colorTextures;
	std::vector<Image> fragmentDensityTextures;
	std::vector<VkImageMemoryBarrier> startBarriers;
	std::vector<VkImageMemoryBarrier> endBarriers;
	VkImage depthImage;
	VkDeviceMemory depthMemory;
	VkImageView depthImageView;
	Image renderTexture;
	std::vector<VkFramebuffer> framebuffers;
	int width;
	int height;
};
