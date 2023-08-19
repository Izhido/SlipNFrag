//
//  Locks.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 6/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include <mutex>

struct Locks
{
	static std::mutex ModeChangeMutex;
	static std::mutex InputMutex;
	static std::mutex RenderInputMutex;
	static std::mutex RenderMutex;
	static std::mutex DirectRectMutex;
	static std::mutex SoundMutex;
};
