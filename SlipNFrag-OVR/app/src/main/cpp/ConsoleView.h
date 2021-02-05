#pragma once

#include "ConsoleFramebuffer.h"
#include "ConsoleColorSwapChain.h"
#include "ConsolePerImage.h"

struct ConsoleView
{
	ConsoleFramebuffer framebuffer;
	ConsoleColorSwapChain colorSwapChain;
	int index;
	std::vector<ConsolePerImage> perImage;
};
