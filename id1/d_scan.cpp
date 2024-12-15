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
// d_scan.c
//
// Portable C scan-level rasterization code, all pixel depths.

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"

unsigned char	*r_turb_pbase, *r_turb_pdest;
fixed16_t		r_turb_s, r_turb_t, r_turb_sstep, r_turb_tstep;
long long		r_turb_lm_s, r_turb_lm_t, r_turb_lm_sstep, r_turb_lm_tstep;
int				*r_turb_turb;
int				r_turb_spancount;

void D_DrawTurbulent8Span (void);
void D_DrawTurbulent8Non64Span (void);
void D_DrawTurbulentLit8Span (void);

std::vector<std::vector<byte*>> warp_rowptr_stack;
std::vector<std::vector<int>> warp_column_stack;
int warp_stack_index = -1;

/*
=============
D_WarpScreen

// this performs a slight compression of the screen at the same time as
// the sine warp, to keep the edges from wrapping
=============
*/
void D_WarpScreen (void)
{
	int		w, h;
	int		u,v;
	byte	*dest;
	int		*turb;
	int		*col;
	byte	**row;
	float	wratio, hratio;

	w = r_refdef.vrect.width;
	h = r_refdef.vrect.height;

	warp_stack_index++;
	if (warp_stack_index >= warp_rowptr_stack.size())
	{
		warp_rowptr_stack.emplace_back(scr_vrect.height + AMP2 * 2);
	}
	else
	{
		warp_rowptr_stack.back().resize(scr_vrect.height + AMP2 * 2);
	}
	auto rowptr = warp_rowptr_stack.back().data();

	if (warp_stack_index >= warp_column_stack.size())
	{
		warp_column_stack.emplace_back(scr_vrect.width + AMP2 * 2);
	}
	else
	{
		warp_column_stack.back().resize(scr_vrect.width + AMP2 * 2);
	}
	auto column = warp_column_stack.back().data();

	wratio = w / (float)scr_vrect.width;
	hratio = h / (float)scr_vrect.height;

	for (v=0 ; v<scr_vrect.height+AMP2*2 ; v++)
	{
		rowptr[v] = d_viewbuffer + (r_refdef.vrect.y * screenwidth) +
				 (screenwidth * (int)((float)v * hratio * h / (h + AMP2 * 2)));
	}

	for (u=0 ; u<scr_vrect.width+AMP2*2 ; u++)
	{
		column[u] = r_refdef.vrect.x +
				(int)((float)u * wratio * w / (w + AMP2 * 2));
	}

	turb = intsintable.data() + ((int)(cl.time*SPEED)&(CYCLE-1));
	dest = vid.buffer + scr_vrect.y * vid.rowbytes + scr_vrect.x;

	for (v=0 ; v<scr_vrect.height ; v++, dest += vid.rowbytes)
	{
		col = &column[turb[v]];
		row = &rowptr[v];

		for (u=0 ; u<scr_vrect.width ; u+=4)
		{
			dest[u+0] = row[turb[u+0]][col[u+0]];
			dest[u+1] = row[turb[u+1]][col[u+1]];
			dest[u+2] = row[turb[u+2]][col[u+2]];
			dest[u+3] = row[turb[u+3]][col[u+3]];
		}
	}

	warp_stack_index--;
}


#if	!id386

/*
=============
D_DrawTurbulentLit8Span
=============
*/
void D_DrawTurbulentLit8Span (void)
{
	int			sturb, tturb;
	long long	lm_sturb, lm_tturb;
	unsigned	light;
	byte		pix;

	do
	{
		sturb = ((r_turb_s + r_turb_turb[(r_turb_t>>16)&(CYCLE-1)])>>16);
		if (sturb >= 0)
		{
			sturb = sturb % cachewidth;
		}
		else
		{
			sturb = cachewidth - (-sturb) % cachewidth;
		}
		tturb = ((r_turb_t + r_turb_turb[(r_turb_s>>16)&(CYCLE-1)])>>16);
		if (tturb >= 0)
		{
			tturb = tturb % r_turb_cacheheight;
		}
		else
		{
			tturb = r_turb_cacheheight - (-tturb) % r_turb_cacheheight;
		}

		if (r_turb_lm_s >= 0)
		{
			lm_sturb = r_turb_lm_s % r_turb_lightmapwidthminusone16;
		}
		else
		{
			lm_sturb = r_turb_lightmapwidthminusone16 - (-r_turb_lm_s) % r_turb_lightmapwidthminusone16;
		}
		if (r_turb_lm_t >= 0)
		{
			lm_tturb = r_turb_lm_t % r_turb_lightmapheightminusone16;
		}
		else
		{
			lm_tturb = r_turb_lightmapheightminusone16 - (-r_turb_lm_t) % r_turb_lightmapheightminusone16;
		}

		pix = *(r_turb_pbase + (tturb*cachewidth) + sturb);
		
		auto position = ((lm_tturb>>16)*(r_turb_lightmapwidthminusone + 1)) + (lm_sturb>>16);
		long long light00 = *(r_turb_lightmapblock + position);
		long long light01 = *(r_turb_lightmapblock + position + 1);
		long long light10 = *(r_turb_lightmapblock + position + r_turb_lightmapwidthminusone + 1);
		long long light11 = *(r_turb_lightmapblock + position + r_turb_lightmapwidthminusone + 1 + 1);
		auto lm_sturb_frac = lm_sturb & 0xFFFF;
		auto lm_tturb_frac = lm_tturb & 0xFFFF;
		auto light0 = light00 + (light01 - light00) * lm_sturb_frac / 0x10000;
		auto light1 = light10 + (light11 - light10) * lm_sturb_frac / 0x10000;
		light = (int)(light0 + (light1 - light0) * lm_tturb_frac / 0x10000);

		*r_turb_pdest++ = ((unsigned char *)vid.colormap)
				[(light & 0xFF00) + pix];

		r_turb_s += r_turb_sstep;
		r_turb_t += r_turb_tstep;

		r_turb_lm_s += r_turb_lm_sstep;
		r_turb_lm_t += r_turb_lm_tstep;
	} while (--r_turb_spancount > 0);
}

/*
=============
D_DrawTurbulent8Non64Span
=============
*/
void D_DrawTurbulent8Non64Span (void)
{
	int			sturb, tturb;

	do
	{
		sturb = ((r_turb_s + r_turb_turb[(r_turb_t>>16)&(CYCLE-1)])>>16);
		if (sturb >= 0)
		{
			sturb = sturb % cachewidth;
		}
		else
		{
			sturb = cachewidth - (-sturb) % cachewidth;
		}
		tturb = ((r_turb_t + r_turb_turb[(r_turb_s>>16)&(CYCLE-1)])>>16);
		if (tturb >= 0)
		{
			tturb = tturb % r_turb_cacheheight;
		}
		else
		{
			tturb = r_turb_cacheheight - (-tturb) % r_turb_cacheheight;
		}
		*r_turb_pdest++ = *(r_turb_pbase + (tturb*cachewidth) + sturb);
		r_turb_s += r_turb_sstep;
		r_turb_t += r_turb_tstep;
	} while (--r_turb_spancount > 0);
}

/*
=============
D_DrawTurbulent8Span
=============
*/
void D_DrawTurbulent8Span (void)
{
	int		sturb, tturb;

	do
	{
		sturb = ((r_turb_s + r_turb_turb[(r_turb_t>>16)&(CYCLE-1)])>>16)&63;
		tturb = ((r_turb_t + r_turb_turb[(r_turb_s>>16)&(CYCLE-1)])>>16)&63;
		*r_turb_pdest++ = *(r_turb_pbase + (tturb<<6) + sturb);
		r_turb_s += r_turb_sstep;
		r_turb_t += r_turb_tstep;
	} while (--r_turb_spancount > 0);
}

#endif	// !id386


/*
=============
TurbulentLit8
=============
*/	
void TurbulentLit8 (espan_t *pspan)
{
	int			count;
	fixed16_t	snext, tnext;
	long long	lm_snext, lm_tnext;
	float		sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float		sdivz16stepu, tdivz16stepu, zi16stepu;
	fixed16_t	cachewidth16, cacheheight16;
	
	r_turb_turb = sintable.data() + ((int)(cl.time*SPEED)&(CYCLE-1));

	r_turb_sstep = 0;	// keep compiler happy
	r_turb_tstep = 0;	// ditto
	r_turb_lm_sstep = 0;
	r_turb_lm_tstep = 0;

	r_turb_pbase = (unsigned char *)cacheblock;

	sdivz16stepu = d_sdivzstepu * 16;
	tdivz16stepu = d_tdivzstepu * 16;
	zi16stepu = d_zistepu * 16;

	cachewidth16 = (2*cachewidth)<<16;
	cacheheight16 = (2*r_turb_cacheheight)<<16;

	do
	{
		r_turb_pdest = (unsigned char *)((byte *)d_viewbuffer +
				(screenwidth * pspan->v) + pspan->u);

		count = pspan->count;

	// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

		r_turb_s = (int)(sdivz * z) + sadjust;
		if (r_turb_s > bbextents)
			r_turb_s = bbextents;
		else if (r_turb_s < 0)
			r_turb_s = 0;

		r_turb_t = (int)(tdivz * z) + tadjust;
		if (r_turb_t > bbextentt)
			r_turb_t = bbextentt;
		else if (r_turb_t < 0)
			r_turb_t = 0;

		r_turb_lm_s = (int)(sdivz * z) + r_turb_lm_sadjust;
		if (r_turb_lm_s > r_turb_lm_bbextents)
			r_turb_lm_s = r_turb_lm_bbextents;
		else if (r_turb_lm_s < 0)
			r_turb_lm_s = 0;
		r_turb_lm_s = r_turb_lm_s * r_turb_lightmapwidthminusone / r_turb_extents0;

		r_turb_lm_t = (int)(tdivz * z) + r_turb_lm_tadjust;
		if (r_turb_lm_t > r_turb_lm_bbextentt)
			r_turb_lm_t = r_turb_lm_bbextentt;
		else if (r_turb_lm_t < 0)
			r_turb_lm_t = 0;
		r_turb_lm_t = r_turb_lm_t * r_turb_lightmapheightminusone / r_turb_extents1;

		do
		{
		// calculate s and t at the far end of the span
			if (count >= 16)
				r_turb_spancount = 16;
			else
				r_turb_spancount = count;

			count -= r_turb_spancount;

			if (count)
			{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
				sdivz += sdivz16stepu;
				tdivz += tdivz16stepu;
				zi += zi16stepu;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				lm_snext = (int)(sdivz * z) + r_turb_lm_sadjust;
				if (lm_snext > r_turb_lm_bbextents)
					lm_snext = r_turb_lm_bbextents;
				else if (lm_snext < 16)
					lm_snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture
				lm_snext = lm_snext * r_turb_lightmapwidthminusone / r_turb_extents0;

				lm_tnext = (int)(tdivz * z) + r_turb_lm_tadjust;
				if (lm_tnext > r_turb_lm_bbextentt)
					lm_tnext = r_turb_lm_bbextentt;
				else if (lm_tnext < 16)
					lm_tnext = 16;	// guard against round-off error on <0 steps
				lm_tnext = lm_tnext * r_turb_lightmapheightminusone / r_turb_extents1;

				r_turb_sstep = (snext - r_turb_s) >> 4;
				r_turb_tstep = (tnext - r_turb_t) >> 4;

				r_turb_lm_sstep = (lm_snext - r_turb_lm_s) >> 4;
				r_turb_lm_tstep = (lm_tnext - r_turb_lm_t) >> 4;
			}
			else
			{
			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
			// can't step off polygon), clamp, calculate s and t steps across
			// span by division, biasing steps low so we don't run off the
			// texture
				spancountminus1 = (float)(r_turb_spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				lm_snext = (int)(sdivz * z) + r_turb_lm_sadjust;
				if (lm_snext > r_turb_lm_bbextents)
					lm_snext = r_turb_lm_bbextents;
				else if (lm_snext < 16)
					lm_snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture
				lm_snext = lm_snext * r_turb_lightmapwidthminusone / r_turb_extents0;

				lm_tnext = (int)(tdivz * z) + r_turb_lm_tadjust;
				if (lm_tnext > r_turb_lm_bbextentt)
					lm_tnext = r_turb_lm_bbextentt;
				else if (lm_tnext < 16)
					lm_tnext = 16;	// guard against round-off error on <0 steps
				lm_tnext = lm_tnext * r_turb_lightmapheightminusone / r_turb_extents1;

				if (r_turb_spancount > 1)
				{
					r_turb_sstep = (snext - r_turb_s) / (r_turb_spancount - 1);
					r_turb_tstep = (tnext - r_turb_t) / (r_turb_spancount - 1);

					r_turb_lm_sstep = (lm_snext - r_turb_lm_s) / (r_turb_spancount - 1);
					r_turb_lm_tstep = (lm_tnext - r_turb_lm_t) / (r_turb_spancount - 1);
				}
			}

			if (r_turb_s >= 0)
			{
				r_turb_s = r_turb_s % cachewidth16;
			}
			else
			{
				r_turb_s = cachewidth16 - (-r_turb_s) % cachewidth16;
			}
			if (r_turb_t >= 0)
			{
				r_turb_t = r_turb_t % cacheheight16;
			}
			else
			{
				r_turb_t = cacheheight16 - (-r_turb_t) & cacheheight16;
			}

			D_DrawTurbulentLit8Span ();

			r_turb_s = snext;
			r_turb_t = tnext;

			r_turb_lm_s = lm_snext;
			r_turb_lm_t = lm_tnext;
		} while (count > 0);

	} while ((pspan = pspan->pnext) != NULL);
}


/*
=============
Turbulent8Non64
=============
*/
void Turbulent8Non64 (espan_t *pspan)
{
	int				count;
	fixed16_t		snext, tnext;
	float			sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float			sdivz16stepu, tdivz16stepu, zi16stepu;
	fixed16_t		cachewidth16, cacheheight16;
	
	r_turb_turb = sintable.data() + ((int)(cl.time*SPEED)&(CYCLE-1));

	r_turb_sstep = 0;	// keep compiler happy
	r_turb_tstep = 0;	// ditto

	r_turb_pbase = (unsigned char *)cacheblock;

	sdivz16stepu = d_sdivzstepu * 16;
	tdivz16stepu = d_tdivzstepu * 16;
	zi16stepu = d_zistepu * 16;

	cachewidth16 = (2*cachewidth)<<16;
	cacheheight16 = (2*r_turb_cacheheight)<<16;

	do
	{
		r_turb_pdest = (unsigned char *)((byte *)d_viewbuffer +
				(screenwidth * pspan->v) + pspan->u);

		count = pspan->count;

	// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

		r_turb_s = (int)(sdivz * z) + sadjust;
		if (r_turb_s > bbextents)
			r_turb_s = bbextents;
		else if (r_turb_s < 0)
			r_turb_s = 0;

		r_turb_t = (int)(tdivz * z) + tadjust;
		if (r_turb_t > bbextentt)
			r_turb_t = bbextentt;
		else if (r_turb_t < 0)
			r_turb_t = 0;

		do
		{
		// calculate s and t at the far end of the span
			if (count >= 16)
				r_turb_spancount = 16;
			else
				r_turb_spancount = count;

			count -= r_turb_spancount;

			if (count)
			{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
				sdivz += sdivz16stepu;
				tdivz += tdivz16stepu;
				zi += zi16stepu;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				r_turb_sstep = (snext - r_turb_s) >> 4;
				r_turb_tstep = (tnext - r_turb_t) >> 4;
			}
			else
			{
			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
			// can't step off polygon), clamp, calculate s and t steps across
			// span by division, biasing steps low so we don't run off the
			// texture
				spancountminus1 = (float)(r_turb_spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				if (r_turb_spancount > 1)
				{
					r_turb_sstep = (snext - r_turb_s) / (r_turb_spancount - 1);
					r_turb_tstep = (tnext - r_turb_t) / (r_turb_spancount - 1);
				}
			}

			if (r_turb_s >= 0)
			{
				r_turb_s = r_turb_s % cachewidth16;
			}
			else
			{
				r_turb_s = cachewidth16 - (-r_turb_s) % cachewidth16;
			}
			if (r_turb_t >= 0)
			{
				r_turb_t = r_turb_t % cacheheight16;
			}
			else
			{
				r_turb_t = cacheheight16 - (-r_turb_t) & cacheheight16;
			}

			D_DrawTurbulent8Non64Span ();

			r_turb_s = snext;
			r_turb_t = tnext;

		} while (count > 0);

	} while ((pspan = pspan->pnext) != NULL);
}


/*
=============
Turbulent8
=============
*/
void Turbulent8 (espan_t *pspan)
{
	int				count;
	fixed16_t		snext, tnext;
	float			sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float			sdivz16stepu, tdivz16stepu, zi16stepu;
	
	r_turb_turb = sintable.data() + ((int)(cl.time*SPEED)&(CYCLE-1));

	r_turb_sstep = 0;	// keep compiler happy
	r_turb_tstep = 0;	// ditto

	r_turb_pbase = (unsigned char *)cacheblock;

	sdivz16stepu = d_sdivzstepu * 16;
	tdivz16stepu = d_tdivzstepu * 16;
	zi16stepu = d_zistepu * 16;

	do
	{
		r_turb_pdest = (unsigned char *)((byte *)d_viewbuffer +
				(screenwidth * pspan->v) + pspan->u);

		count = pspan->count;

	// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

		r_turb_s = (int)(sdivz * z) + sadjust;
		if (r_turb_s > bbextents)
			r_turb_s = bbextents;
		else if (r_turb_s < 0)
			r_turb_s = 0;

		r_turb_t = (int)(tdivz * z) + tadjust;
		if (r_turb_t > bbextentt)
			r_turb_t = bbextentt;
		else if (r_turb_t < 0)
			r_turb_t = 0;

		do
		{
		// calculate s and t at the far end of the span
			if (count >= 16)
				r_turb_spancount = 16;
			else
				r_turb_spancount = count;

			count -= r_turb_spancount;

			if (count)
			{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
				sdivz += sdivz16stepu;
				tdivz += tdivz16stepu;
				zi += zi16stepu;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				r_turb_sstep = (snext - r_turb_s) >> 4;
				r_turb_tstep = (tnext - r_turb_t) >> 4;
			}
			else
			{
			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
			// can't step off polygon), clamp, calculate s and t steps across
			// span by division, biasing steps low so we don't run off the
			// texture
				spancountminus1 = (float)(r_turb_spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				if (r_turb_spancount > 1)
				{
					r_turb_sstep = (snext - r_turb_s) / (r_turb_spancount - 1);
					r_turb_tstep = (tnext - r_turb_t) / (r_turb_spancount - 1);
				}
			}

			r_turb_s = r_turb_s & ((CYCLE<<16)-1);
			r_turb_t = r_turb_t & ((CYCLE<<16)-1);

			D_DrawTurbulent8Span ();

			r_turb_s = snext;
			r_turb_t = tnext;

		} while (count > 0);

	} while ((pspan = pspan->pnext) != NULL);
}


#if	!id386

/*
=============
D_DrawLittleSpans64
=============
*/
void D_DrawLittleSpans64 (espan_t *pspan)
{
	int				count, spancount;
	unsigned char	*pbase, *pdest;
	fixed16_t		s, t, snext, tnext, sstep, tstep;
	float			sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float			sdivz8stepu, tdivz8stepu, zi8stepu;
	uint64_t		ltemp;

	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	pbase = (unsigned char *)cacheblock;

	sdivz8stepu = d_sdivzstepu * 8;
	tdivz8stepu = d_tdivzstepu * 8;
	zi8stepu = d_zistepu * 8;

	do
	{
		pdest = (unsigned char *)((byte *)d_viewbuffer +
				(screenwidth * pspan->v) + pspan->u);

		count = pspan->count;

	// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

		s = (int)(sdivz * z) + sadjust;
		if (s > bbextents)
			s = bbextents;
		else if (s < 0)
			s = 0;

		t = (int)(tdivz * z) + tadjust;
		if (t > bbextentt)
			t = bbextentt;
		else if (t < 0)
			t = 0;

		if (count >= 8)
			spancount = 8;
		else
			spancount = count;

		auto pad = (int)((uint64_t)pdest % 8);
		if (pad > 0)
		{
			auto remaining = 8 - pad;
			if (count >= remaining)
			{
				spancount = remaining;
			}
		}
		
		count -= spancount;

		while (spancount > 0)
		{
		// calculate s and t at the far end of the span
			if (spancount == 8)
			{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
				sdivz += sdivz8stepu;
				tdivz += tdivz8stepu;
				zi += zi8stepu;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 8)
					snext = 8;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 8)
					tnext = 8;	// guard against round-off error on <0 steps

				sstep = (snext - s) >> 3;
				tstep = (tnext - t) >> 3;

				ltemp = (*(pbase + (s >> 16) + (t >> 16) * cachewidth));
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 8;
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 16;
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 24;
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 32;
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 40;
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 48;
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 56;
				s += sstep;
				t += tstep;

				*(uint64_t*)pdest = ltemp;
				pdest += 8;
			}
			else
			{
			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
			// can't step off polygon), clamp, calculate s and t steps across
			// span by division, biasing steps low so we don't run off the
			// texture
				spancountminus1 = (float)(spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 8)
					snext = 8;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 8)
					tnext = 8;	// guard against round-off error on <0 steps

				if (spancount > 1)
				{
					sstep = (snext - s) / (spancount - 1);
					tstep = (tnext - t) / (spancount - 1);
				}

				do
				{
					*pdest++ = *(pbase + (s >> 16) + (t >> 16) * cachewidth);
					s += sstep;
					t += tstep;
				} while (--spancount > 0);
			}

			s = snext;
			t = tnext;

			if (count >= 8)
				spancount = 8;
			else
				spancount = count;

			count -= spancount;
		}

	} while ((pspan = pspan->pnext) != NULL);
}

/*
=============
D_DrawBigSpans64
=============
*/
void D_DrawBigSpans64 (espan_t *pspan)
{
	int				count, spancount;
	unsigned char	*pbase, *pdest;
	fixed16_t		s, t, snext, tnext, sstep, tstep;
	float			sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float			sdivz8stepu, tdivz8stepu, zi8stepu;
	uint64_t		ltemp;

	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	pbase = (unsigned char *)cacheblock;

	sdivz8stepu = d_sdivzstepu * 8;
	tdivz8stepu = d_tdivzstepu * 8;
	zi8stepu = d_zistepu * 8;

	do
	{
		pdest = (unsigned char *)((byte *)d_viewbuffer +
				(screenwidth * pspan->v) + pspan->u);

		count = pspan->count;

	// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

		s = (int)(sdivz * z) + sadjust;
		if (s > bbextents)
			s = bbextents;
		else if (s < 0)
			s = 0;

		t = (int)(tdivz * z) + tadjust;
		if (t > bbextentt)
			t = bbextentt;
		else if (t < 0)
			t = 0;

		if (count >= 8)
			spancount = 8;
		else
			spancount = count;

		auto pad = (int)((uint64_t)pdest % 8);
		if (pad > 0)
		{
			auto remaining = 8 - pad;
			if (count >= remaining)
			{
				spancount = remaining;
			}
		}
		
		count -= spancount;

		while (spancount > 0)
		{
		// calculate s and t at the far end of the span
			if (spancount == 8)
			{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
				sdivz += sdivz8stepu;
				tdivz += tdivz8stepu;
				zi += zi8stepu;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 8)
					snext = 8;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 8)
					tnext = 8;	// guard against round-off error on <0 steps

				sstep = (snext - s) >> 3;
				tstep = (tnext - t) >> 3;

				ltemp = (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 56;
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 48;
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 40;
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 32;
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 24;
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 16;
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth)) << 8;
				s += sstep;
				t += tstep;
				ltemp |= (uint64_t)(*(pbase + (s >> 16) + (t >> 16) * cachewidth));
				s += sstep;
				t += tstep;

				*(uint64_t*)pdest = ltemp;
				pdest += 8;
			}
			else
			{
			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
			// can't step off polygon), clamp, calculate s and t steps across
			// span by division, biasing steps low so we don't run off the
			// texture
				spancountminus1 = (float)(spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 8)
					snext = 8;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 8)
					tnext = 8;	// guard against round-off error on <0 steps

				if (spancount > 1)
				{
					sstep = (snext - s) / (spancount - 1);
					tstep = (tnext - t) / (spancount - 1);
				}

				do
				{
					*pdest++ = *(pbase + (s >> 16) + (t >> 16) * cachewidth);
					s += sstep;
					t += tstep;
				} while (--spancount > 0);
			}

			s = snext;
			t = tnext;

			if (count >= 8)
				spancount = 8;
			else
				spancount = count;

			count -= spancount;
		}

	} while ((pspan = pspan->pnext) != NULL);
}

#endif


#if	!id386

/*
=============
D_DrawLittleZSpans64
=============
*/
void D_DrawLittleZSpans64 (espan_t *pspan)
{
	int				count, spancount;
	long long		izistep, izi;
	short			*pdest;
	uint64_t		ltemp;
	double			zi;
	float			du, dv;

// FIXME: check for clamping/range problems
// we count on FP exceptions being turned off to avoid range problems
	izistep = d_zistepu * 0x8000 * 0x10000;

	do
	{
		pdest = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u;

		count = pspan->count;

	// calculate the initial 1/z
		du = (float)pspan->u;
		dv = (float)pspan->v;

		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
	// we count on FP exceptions being turned off to avoid range problems
		izi = zi * 0x8000 * 0x10000;

		if (count >= 4)
			spancount = 4;
		else
			spancount = count;

		auto pad = (int)((uint64_t)pdest % 8);
		if (pad > 0)
		{
			auto remaining = (8 - pad) >> 1;
			if (count >= remaining)
			{
				spancount = remaining;
			}
		}
		
		count -= spancount;

		while (spancount > 0)
		{
			if (spancount == 4)
			{
				ltemp = (uint64_t)((uint16_t)(izi >> 16));
				izi += izistep;
				ltemp |= (uint64_t)((uint16_t)(izi >> 16)) << 16;
				izi += izistep;
				ltemp |= (uint64_t)((uint16_t)(izi >> 16)) << 32;
				izi += izistep;
				ltemp |= (uint64_t)((uint16_t)(izi >> 16)) << 48;
				izi += izistep;

				*(uint64_t*)pdest = ltemp;
				pdest += 4;
			}
			else
			{
				do
				{
					*pdest++ = (short)(izi >> 16);
					izi += izistep;
				} while (--spancount > 0);
			}

			if (count >= 4)
				spancount = 4;
			else
				spancount = count;

			count -= spancount;
		}

	} while ((pspan = pspan->pnext) != NULL);
}

/*
=============
D_DrawBigZSpans64
=============
*/
void D_DrawBigZSpans64 (espan_t *pspan)
{
	int				count, spancount;
	long long		izistep, izi;
	short			*pdest;
	uint64_t		ltemp;
	double			zi;
	float			du, dv;

// FIXME: check for clamping/range problems
// we count on FP exceptions being turned off to avoid range problems
	izistep = d_zistepu * 0x8000 * 0x10000;

	do
	{
		pdest = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u;

		count = pspan->count;

	// calculate the initial 1/z
		du = (float)pspan->u;
		dv = (float)pspan->v;

		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
	// we count on FP exceptions being turned off to avoid range problems
		izi = zi * 0x8000 * 0x10000;

		if (count >= 4)
			spancount = 4;
		else
			spancount = count;

		auto pad = (int)((uint64_t)pdest % 8);
		if (pad > 0)
		{
			auto remaining = (8 - pad) >> 1;
			if (count >= remaining)
			{
				spancount = remaining;
			}
		}

		count -= spancount;

		while (spancount > 0)
		{
			if (spancount == 4)
			{
				ltemp = (uint64_t)((uint16_t)(izi >> 16)) << 48;
				izi += izistep;
				ltemp |= (uint64_t)((uint16_t)(izi >> 16)) << 32;
				izi += izistep;
				ltemp |= (uint64_t)((uint16_t)(izi >> 16)) << 16;
				izi += izistep;
				ltemp |= (uint64_t)((uint16_t)(izi >> 16));
				izi += izistep;

				*(uint64_t*)pdest = ltemp;
				pdest += 4;
			}
			else
			{
				do
				{
					*pdest++ = (short)(izi >> 16);
					izi += izistep;
				} while (--spancount > 0);
			}

			if (count >= 8)
				spancount = 8;
			else
				spancount = count;

			count -= spancount;
		}

	} while ((pspan = pspan->pnext) != NULL);
}

#endif


#if	!id386

/*
=============
D_DrawFenceSpans8
=============
*/
void D_DrawFenceSpans8 (espan_t *pspan)
{
	int				count, spancount;
	unsigned char	*pbase, *pdest;
	short			*pz;
	fixed16_t		s, t, snext, tnext, sstep, tstep;
	float			sdivz, tdivz, zi, z, du, dv, spancountminus1;
	float			sdivz8stepu, tdivz8stepu, zi8stepu;
	long long		izistep, izi;
	byte			btemp;

	sstep = 0;	// keep compiler happy
	tstep = 0;	// ditto

	pbase = (unsigned char *)cacheblock;

	sdivz8stepu = d_sdivzstepu * 8;
	tdivz8stepu = d_tdivzstepu * 8;
	zi8stepu = d_zistepu * 8;

// FIXME: check for clamping/range problems
// we count on FP exceptions being turned off to avoid range problems
	izistep = d_zistepu * 0x8000 * 0x10000;

	do
	{
		pdest = (unsigned char *)((byte *)d_viewbuffer +
				(screenwidth * pspan->v) + pspan->u);
		pz = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u;

		count = pspan->count;

	// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
	// we count on FP exceptions being turned off to avoid range problems
		izi = zi * 0x8000 * 0x10000;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

		s = (int)(sdivz * z) + sadjust;
		if (s > bbextents)
			s = bbextents;
		else if (s < 0)
			s = 0;

		t = (int)(tdivz * z) + tadjust;
		if (t > bbextentt)
			t = bbextentt;
		else if (t < 0)
			t = 0;

		do
		{
		// calculate s and t at the far end of the span
			if (count >= 8)
				spancount = 8;
			else
				spancount = count;

			count -= spancount;

			if (count)
			{
			// calculate s/z, t/z, zi->fixed s and t at far end of span,
			// calculate s and t steps across span by shifting
				sdivz += sdivz8stepu;
				tdivz += tdivz8stepu;
				zi += zi8stepu;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 8)
					snext = 8;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 8)
					tnext = 8;	// guard against round-off error on <0 steps

				sstep = (snext - s) >> 3;
				tstep = (tnext - t) >> 3;
			}
			else
			{
			// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
			// can't step off polygon), clamp, calculate s and t steps across
			// span by division, biasing steps low so we don't run off the
			// texture
				spancountminus1 = (float)(spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 8)
					snext = 8;	// prevent round-off error on <0 steps from
								//  from causing overstepping & running off the
								//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 8)
					tnext = 8;	// guard against round-off error on <0 steps

				if (spancount > 1)
				{
					sstep = (snext - s) / (spancount - 1);
					tstep = (tnext - t) / (spancount - 1);
				}
			}

			do
			{
				btemp = *(pbase + (s >> 16) + (t >> 16) * cachewidth);
				if (btemp != 255)
				{
					if (*pz <= (izi >> 16))
					{
						*pz = izi >> 16;
						*pdest = btemp;
					}
				}

				izi += izistep;
				pdest++;
				pz++;
				s += sstep;
				t += tstep;
			} while (--spancount > 0);

			s = snext;
			t = tnext;

		} while (count > 0);

	} while ((pspan = pspan->pnext) != NULL);
}

#endif

