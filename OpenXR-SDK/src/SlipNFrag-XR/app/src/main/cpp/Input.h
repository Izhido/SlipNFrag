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

	static void AddKeyInput(int key, int down);
	static void AddCommandInput(const char* command);
	static void Handle(AppState& appState, bool triggerHandled);
};
