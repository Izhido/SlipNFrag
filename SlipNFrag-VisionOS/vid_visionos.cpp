//
//  vid_visionos.cpp
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 24/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#include "vid_visionos.h"
#include "d_local.h"

extern viddef_t vid;

std::vector<unsigned char> vid_buffer;
int vid_width;
int vid_height;
std::vector<unsigned char> con_buffer;
int con_width;
int con_height;
std::vector<short> zbuffer;
std::vector<byte> surfcache;

unsigned short d_8to16table[256];
unsigned d_8to24table[256];

void VID_SetPalette(unsigned char *palette)
{
	byte    *pal;
	unsigned r,g,b;
	unsigned v;
	unsigned short i;
	unsigned    *table;
	
	//
	// 8 8 8 encoding
	//
	pal = palette;
	table = d_8to24table;
	for (i=0 ; i<256 ; i++)
	{
		r = pal[0];
		g = pal[1];
		b = pal[2];
		pal += 3;
		
		v = (255 << 24) | (b << 16) | (g << 8) | r;
		*table++ = v;
	}
	d_8to24table[255] &= 0xFFFFFF;    // 255 is transparent
}

void VID_ShiftPalette(unsigned char *palette)
{
	VID_SetPalette(palette);
}

void VID_Init(unsigned char *palette)
{
	vid_buffer.resize(vid_width * vid_height);
	con_buffer.resize(con_width * con_height);
	vid.maxwarpwidth = WARP_WIDTH;
	vid.width = vid_width;
	vid.conwidth = con_width;
	vid.maxwarpheight = WARP_HEIGHT;
	vid.height = vid_height;
	vid.conheight = con_height;
	vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
	vid.numpages = 1;
	vid.colormap = host_colormap.data();
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.buffer = vid_buffer.data();
	vid.conbuffer = con_buffer.data();
	vid.rowbytes = vid_width;
	vid.conrowbytes = con_width;
	zbuffer.resize(vid_width * vid_height);
	d_pzbuffer = zbuffer.data();
	srand(time(nullptr));
	int surfcachesize = D_SurfaceCacheForRes(vid_width, vid_height);
	surfcache.resize(surfcachesize);
	D_InitCaches(surfcache.data(), (int)surfcache.size());
}

void VID_Resize()
{
	D_FlushCaches();
	vid_buffer.resize(vid_width * vid_height);
	con_buffer.resize(con_width * con_height);
	vid.width = vid_width;
	vid.conwidth = con_width;
	vid.height = vid_height;
	vid.conheight = con_height;
	vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);
	vid.buffer = vid_buffer.data();
	vid.conbuffer = con_buffer.data();
	vid.rowbytes = vid_width;
	vid.conrowbytes = con_width;
	zbuffer.resize(vid_width * vid_height);
	d_pzbuffer = zbuffer.data();
	int surfcachesize = D_SurfaceCacheForRes(vid_width, vid_height);
	surfcache.resize(surfcachesize);
	Draw_ResizeScanTables();
	D_InitCaches (surfcache.data(), (int)surfcache.size());
	R_ResizeTurb();
	R_ResizeEdges();
	vid.recalc_refdef = 1;
}

void VID_ReallocSurfCache()
{
	int surfcachesize = D_SurfaceCacheForRes(vid_width, vid_height);
	surfcache.resize(surfcache.size() + surfcachesize);
	Draw_ResizeScanTables();
	D_InitCaches (surfcache.data(), (int)surfcache.size());
}

void VID_Shutdown(void)
{
}

void VID_Update(vrect_t *rects)
{
}

void D_BeginDirectRect(int x, int y, byte *pbitmap, int width, int height)
{
}

void D_EndDirectRect(int x, int y, int width, int height)
{
}
