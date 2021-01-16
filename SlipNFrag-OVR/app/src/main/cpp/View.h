#pragma once

struct View
{
	Framebuffer framebuffer;
	ColorSwapChain colorSwapChain;
	int index;
	std::vector<PerImage> perImage;
};
