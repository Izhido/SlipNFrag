//
//  AppState.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 16/8/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include "AppMode.h"

struct AppState
{
	AppMode Mode;
	AppMode PreviousMode;
	int DefaultFOV;
	int FOV;
	float Yaw;
	float Pitch;
	float Roll;
	float Scale;
	float PositionX;
	float PositionY;
	float PositionZ;
};

extern AppState appState;
