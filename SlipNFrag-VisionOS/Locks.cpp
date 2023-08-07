//
//  Locks.cpp
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 6/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#include "Locks.h"

std::mutex Locks::InputMutex;

std::mutex Locks::RenderMutex;

std::mutex Locks::DirectRectMutex;

std::mutex Locks::SoundMutex;
