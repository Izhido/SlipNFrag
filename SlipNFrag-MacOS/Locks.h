//
//  Locks.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 31/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include <mutex>

struct Locks
{
	static bool EngineStarted;
	static bool StopEngine;
	
	static std::mutex InputMutex;
	static std::mutex RenderMutex;
	static std::mutex DirectRectMutex;
	static std::mutex SoundMutex;
};
