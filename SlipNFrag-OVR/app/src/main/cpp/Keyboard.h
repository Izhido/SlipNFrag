#pragma once

#include "VrApi_Helpers.h"
#include "KeyboardLayout.h"
#include "KeyboardCell.h"
#include <vector>
#include "Texture.h"
#include "PipelineDescriptorResources.h"

struct AppState;

struct Keyboard
{
	ovrTextureSwapChain* SwapChain;
	std::vector<uint32_t> Data;
	VkImage Image;
	Buffer Buffer;
	VkCommandBuffer CommandBuffer;
	VkSubmitInfo SubmitInfo;
	KeyboardLayout layout;
	static std::vector<std::vector<KeyboardCell>> cells;
	int leftHighlighted = -1;
	int rightHighlighted = -1;
	int leftPressed = -1;
	int rightPressed = -1;
	std::vector<unsigned char> buffer;

	void Create(AppState& appState);
	void AddKeyInput(int key, bool down);
	bool Handle(AppState& appState);
	void Fill(AppState& appState, KeyboardCell& cell, unsigned char color);
	void Print(AppState& appState, KeyboardCell& cell, bool upper);
	bool Draw(AppState& appState);
};
