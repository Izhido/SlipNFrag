#pragma once

#include <string>
#include <vector>

struct AppState;

struct AppInput
{
	int key;
	int down;
	std::string command;

	static std::vector<AppInput> inputQueue;
	static int lastInputQueueItem;

	static void AddKeyInput(int key, int down);
	static void AddCommandInput(const char* command);
	static void Handle(AppState& appState, bool triggerHandled);
};
