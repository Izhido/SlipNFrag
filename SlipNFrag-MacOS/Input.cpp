//
//  Input.cpp
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 4/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#include "Input.h"
#include "Locks.h"

std::vector<int> Input::inputQueueKeys;

std::vector<bool> Input::inputQueuePressed;

std::vector<std::string> Input::inputQueueCommands;

int Input::lastInputQueueItem = -1;

void Input::AddKeyInput(int key, bool pressed)
{
	std::lock_guard<std::mutex> lock(Locks::InputMutex);

	lastInputQueueItem++;
	if (lastInputQueueItem >= inputQueueKeys.size())
	{
		inputQueueKeys.emplace_back();
		inputQueuePressed.emplace_back();
		inputQueueCommands.emplace_back();
	}
	inputQueueKeys[lastInputQueueItem] = key;
	inputQueuePressed[lastInputQueueItem] = pressed;
	inputQueueCommands[lastInputQueueItem].clear();
}

void Input::AddCommandInput(const char* command)
{
	std::lock_guard<std::mutex> lock(Locks::InputMutex);

	lastInputQueueItem++;
	if (lastInputQueueItem >= inputQueueKeys.size())
	{
		inputQueueKeys.emplace_back();
		inputQueuePressed.emplace_back();
		inputQueueCommands.emplace_back();
	}
	inputQueueKeys[lastInputQueueItem] = 0;
	inputQueuePressed[lastInputQueueItem] = false;
	inputQueueCommands[lastInputQueueItem] = command;
}

