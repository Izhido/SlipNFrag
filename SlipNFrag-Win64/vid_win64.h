#pragma once

#include "quakedef.h"

extern std::vector<unsigned char> vid_buffer;
extern int vid_width;
extern int vid_height;
extern int vid_rowbytes;
extern std::vector<unsigned char> con_buffer;
extern int con_width;
extern int con_height;
extern int con_rowbytes;
extern unsigned d_8to24table[256];
extern qboolean vid_palettechanged;

void VID_Resize();

void VID_ReallocSurfCache();