/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// d_local.h:  private rasterization driver defs

#include "r_shared.h"

//
// TODO: fine-tune this; it's based on providing some overage even if there
// is a 2k-wide scan, with subdivision every 8, for 256 spans of 12 bytes each
//
#define SCANBUFFERPAD		0x1000

#define R_SKY_SMASK	0x007F0000
#define R_SKY_TMASK	0x007F0000

#define DS_SPAN_LIST_END	-128

#define SURFCACHE_SIZE_AT_320X200	600*1024

typedef struct surfcache_s
{
	struct surfcache_s	*next;
	struct surfcache_s 	**owner;		// NULL is an empty chunk of memory
	int					lightadj[MAXLIGHTMAPS]; // checked for strobe flush
	int					dlight;
	int					size;		// including header
	unsigned			width;
	unsigned			height;		// DEBUG only needed for debug
	float				mipscale;
	struct texture_s	*texture;	// checked for animating textures
	int 				created;	// frame at which it was created
	byte				data[4];	// width*height elements
} surfcache_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct sspan_s
{
	int				u, v, count;
} sspan_t;

extern cvar_t	d_subdiv16;

extern float	scale_for_mip;

extern qboolean		d_roverwrapped;
extern surfcache_t	*sc_rover;
extern surfcache_t	*d_initial_rover;

extern float	d_sdivzstepu, d_tdivzstepu, d_zistepu;
extern float	d_sdivzstepv, d_tdivzstepv, d_zistepv;
extern float	d_sdivzorigin, d_tdivzorigin, d_ziorigin;

extern fixed16_t	sadjust, tadjust;
extern fixed16_t	bbextents, bbextentt;

extern fixed16_t	r_turb_lm_sadjust, r_turb_lm_tadjust;
extern fixed16_t	r_turb_lm_bbextents, r_turb_lm_bbextentt;

extern int			r_turb_extents0, r_turb_extents1;

extern byte			r_ditherthresholds[];


void D_DrawLittleSpans64 (espan_t *pspans);
void D_DrawBigSpans64 (espan_t *pspans);
void D_DrawLittleZSpans64 (espan_t *pspans);
void D_DrawBigZSpans64 (espan_t *pspans);
void TurbulentLit8 (espan_t *pspan);
void TurbulentLitAlpha8 (espan_t *pspan, byte alpha);
void Turbulent8Non64 (espan_t *pspan);
void TurbulentAlpha8Non64 (espan_t *pspan, byte alpha);
void Turbulent8 (espan_t *pspan);
void TurbulentAlpha8 (espan_t *pspan, byte alpha);
void D_SpriteDrawSpans (sspan_t *pspan);

void D_DrawSkyScans8 (espan_t *pspan);
void D_DrawSkyScans16 (espan_t *pspan);

void D_DrawFenceSpans8 (espan_t *pspans);

void D_DrawAlphaSpans8 (espan_t *pspans, byte alpha);
void D_DrawFenceAlphaSpans8 (espan_t *pspans, byte alpha);

void R_ShowSubDiv (void);
extern void (*prealspandrawer)(void);
surfcache_t	*D_CacheSurface (msurface_t *surface, int miplevel);
surfcache_t* D_CacheLightmap (msurface_t *surface, texture_t *texture);
surfcache_t* D_CacheColoredLightmap (msurface_t *surface, texture_t *texture);

extern int D_MipLevelForScale (float scale);

extern void D_DrawTurbulentToLists (msurface_t* pface);
extern void D_DrawSurfaceToLists (msurface_t* pface);

#if id386
extern void D_PolysetAff8Start (void);
extern void D_PolysetAff8End (void);
#endif

extern short *d_pzbuffer;
extern unsigned int d_zrowbytes, d_zwidth;

extern int	*d_pscantable;
extern std::vector<int>	d_scantable;

extern int	d_vrectx, d_vrecty, d_vrectright_particle, d_vrectbottom_particle;

extern int	d_y_aspect_shift, d_pix_min, d_pix_max, d_pix_shift;

extern pixel_t	*d_viewbuffer;

extern std::vector<short*>	zspantable;

extern int		d_minmip;
extern float	d_scalemip[3];

extern void (*d_drawspans) (espan_t *pspan);
extern void (*d_drawzspans) (espan_t *pspan);

