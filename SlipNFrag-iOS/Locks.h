//
//  Locks.h
//  SlipNFrag-iOS
//
//  Created by Heriberto Delgado on 22/12/25.
//  Copyright © 2025 Heriberto Delgado. All rights reserved.
//

#pragma once

#include <mutex>

struct Locks
{
	static std::mutex SoundMutex;
};
