//
//  Locks.cpp
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 2/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#include "Locks.h"

bool Locks::StopEngine;

std::mutex Locks::InputMutex;

std::mutex Locks::RenderMutex;

std::mutex Locks::DirectRectMutex;

std::mutex Locks::SoundMutex;
