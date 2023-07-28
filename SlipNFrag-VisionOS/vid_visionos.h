//
//  vid_visionos.h
//  SlipNFrag
//
//  Created by Heriberto Delgado on 24/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#pragma once

#include "quakedef.h"

extern std::vector<unsigned char> vid_buffer;
extern int vid_width;
extern int vid_height;
extern std::vector<unsigned char> con_buffer;
extern int con_width;
extern int con_height;
extern unsigned d_8to24table[256];

void VID_Resize();

void VID_ReallocSurfCache();
