#pragma once

struct Console
{
	ovrTextureSwapChain* SwapChain;
	VkCommandBuffer CommandBuffer;
	VkRenderPass RenderPass;
	ConsoleView View;
};
