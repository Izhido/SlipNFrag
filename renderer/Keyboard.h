#pragma once

#include "Screen.h"
#include "KeyboardLayout.h"
#include "KeyboardCell.h"
#include <vector>
#include "Texture.h"

struct AppState;

struct Keyboard
{
	Screen Screen;
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
