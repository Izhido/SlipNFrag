#pragma once

#include "quakedef.h"

extern std::vector<unsigned char> vid_buffer;
extern int vid_width;
extern int vid_height;
extern std::vector<unsigned char> con_buffer;
extern int con_width;
extern int con_height;
extern unsigned d_8to24table[256];
extern int pal_changed;

void VID_Resize(float forced_aspect);

void VID_ReallocSurfCache();
