#pragma once

#include <string>

struct AppState;

struct Input
{
	int key;
	int down;
	std::string command;

	static std::vector<Input> inputQueue;
	static int lastInputQueueItem;

	static bool LeftButtonIsDown(AppState& appState, uint32_t button);
	static bool LeftButtonIsUp(AppState& appState, uint32_t button);
	static bool RightButtonIsDown(AppState& appState, uint32_t button);
	static bool RightButtonIsUp(AppState& appState, uint32_t button);
	static void AddKeyInput(int key, int down);
	static void AddCommandInput(const char* command);
	static void Handle(AppState& appState, bool triggerHandled);
};
