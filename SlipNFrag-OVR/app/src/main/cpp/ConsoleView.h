#pragma once

struct ConsoleView
{
	ConsoleFramebuffer framebuffer;
	ConsoleColorSwapChain colorSwapChain;
	int index;
	std::vector<ConsolePerImage> perImage;
};
