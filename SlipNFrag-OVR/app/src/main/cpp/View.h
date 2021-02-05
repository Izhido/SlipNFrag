#pragma once

#include "Framebuffer.h"
#include "ColorSwapChain.h"
#include "PerImage.h"

struct View
{
	Framebuffer framebuffer;
	ColorSwapChain colorSwapChain;
	int index;
	std::vector<PerImage> perImage;
};
