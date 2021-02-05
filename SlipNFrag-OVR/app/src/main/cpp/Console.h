#pragma once

#include "ConsoleView.h"

struct Console
{
	ovrTextureSwapChain* SwapChain;
	VkCommandBuffer CommandBuffer;
	VkRenderPass RenderPass;
	ConsoleView View;
};
