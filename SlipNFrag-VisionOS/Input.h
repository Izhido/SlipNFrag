//
//  Input.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 6/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include <vector>
#include <string>

struct Input
{
	static std::vector<int> inputQueueKeys;
	static std::vector<bool> inputQueuePressed;
	static std::vector<std::string> inputQueueCommands;

	static int lastInputQueueItem;
	
	static void AddKeyInput(int key, bool pressed);
	static void AddCommandInput(const char* command);
};
