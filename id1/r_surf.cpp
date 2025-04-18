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
// r_surf.c: surface-related refresh code

#include "quakedef.h"
#include "r_local.h"

drawsurf_t	r_drawsurf;

int				lightleft, sourcesstep, blocksize, sourcetstep;
int				lightdelta, lightdeltastep;
int				lightright, lightleftstep, lightrightstep, blockdivshift;
unsigned		blockdivmask;
void			*prowdestbase;
unsigned char	*pbasesource;
byte			*pbasepal;
int				surfrowbytes;	// used by ASM files
unsigned		*r_lightptr;
int				r_stepback;
int				r_lightwidth;
int				r_numhblocks, r_numvblocks;
unsigned char	*r_source, *r_sourcemax;

std::vector<byte>	r_24to8table;
byte*				r_24to8tableptr;

void R_DrawSurfaceBlock8_mip0 (void);
void R_DrawSurfaceBlock8_mip1 (void);
void R_DrawSurfaceBlock8_mip2 (void);
void R_DrawSurfaceBlock8_mip3 (void);

static void	(*surfmiptable[4])(void) = {
	R_DrawSurfaceBlock8_mip0,
	R_DrawSurfaceBlock8_mip1,
	R_DrawSurfaceBlock8_mip2,
	R_DrawSurfaceBlock8_mip3
};

void R_DrawSurfaceBlock8_coloredmip0 (void);
void R_DrawSurfaceBlock8_coloredmip1 (void);
void R_DrawSurfaceBlock8_coloredmip2 (void);
void R_DrawSurfaceBlock8_coloredmip3 (void);

static void	(*surfmiptablecolored[4])(void) = {
	R_DrawSurfaceBlock8_coloredmip0,
	R_DrawSurfaceBlock8_coloredmip1,
	R_DrawSurfaceBlock8_coloredmip2,
	R_DrawSurfaceBlock8_coloredmip3
};



std::vector<unsigned>		r_blocklights_base(18*18);
unsigned*				blocklights;
int 					r_blocklights_smax;
int 					r_blocklights_tmax;
int 					r_blocklights_size;

/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights (void)
{
	msurface_t *surf;
	int			lnum;
	int			sd, td;
	float		dist, rad, minlight;
	vec3_t		impact, local;
	int			s, t;
	int			i;
	mtexinfo_t	*tex;

	surf = r_drawsurf.surf;
	tex = surf->texinfo;

	auto smax = r_blocklights_smax * 16;
	auto tmax = r_blocklights_tmax * 16;

	for (lnum=0 ; lnum<cl_dlights.size() ; lnum++)
	{
		if ( surf->dlightbits.size() <= lnum || !surf->dlightbits[lnum] )
			continue;		// not lit by this light

		rad = cl_dlights[lnum].radius;
		dist = DotProduct (cl_dlights[lnum].origin, surf->plane->normal) -
				surf->plane->dist;
		rad -= fabs(dist);
		minlight = cl_dlights[lnum].minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		for (i=0 ; i<3 ; i++)
		{
			impact[i] = cl_dlights[lnum].origin[i] -
					surf->plane->normal[i]*dist;
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];
		
		auto target = blocklights;
		for (t = 0 ; t<tmax ; t += 16)
		{
			td = local[1] - t;
			if (td < 0)
				td = -td;
			for (s=0 ; s<smax ; s += 16)
			{
				sd = local[0] - s;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td>>1);
				else
					dist = td + (sd>>1);
				if (dist < minlight)
#ifdef QUAKE2
				{
					unsigned temp;
					temp = (rad - dist)*256;
					i = t*smax + s;
					if (!cl_dlights[lnum].dark)
						blocklights[i] += temp;
					else
					{
						if (blocklights[i] > temp)
							blocklights[i] -= temp;
						else
							blocklights[i] = 0;
					}
				}
#else
					*target += (rad - dist)*256;
#endif
				target++;
			}
		}
	}
}

/*
===============
R_AddDynamicColoredLights
===============
*/
void R_AddDynamicColoredLights (void)
{
	msurface_t *surf;
	int			lnum;
	int			sd, td;
	float		dist, rad, minlight;
	vec3_t		impact, local;
	int			s, t;
	int			i;
	mtexinfo_t	*tex;

	surf = r_drawsurf.surf;
	tex = surf->texinfo;

	auto smax = r_blocklights_smax * 16;
	auto tmax = r_blocklights_tmax * 16;

	for (lnum=0 ; lnum<cl_dlights.size() ; lnum++)
	{
		if ( surf->dlightbits.size() <= lnum || !surf->dlightbits[lnum] )
			continue;		// not lit by this light

		rad = cl_dlights[lnum].radius;
		dist = DotProduct (cl_dlights[lnum].origin, surf->plane->normal) -
				surf->plane->dist;
		rad -= fabs(dist);
		minlight = cl_dlights[lnum].minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		for (i=0 ; i<3 ; i++)
		{
			impact[i] = cl_dlights[lnum].origin[i] -
					surf->plane->normal[i]*dist;
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];

		auto target = blocklights;
		for (t = 0 ; t<tmax ; t += 16)
		{
			td = local[1] - t;
			if (td < 0)
				td = -td;
			for (s=0 ; s<smax ; s += 16)
			{
				sd = local[0] - s;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td>>1);
				else
					dist = td + (sd>>1);
				if (dist < minlight)
				{
					unsigned inc = (rad - dist)*256;
					*target += inc;
					target++;
					*target += inc;
					target++;
					*target += inc;
					target++;
				}
				else
				{
					target += 3;
				}
			}
		}
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
void R_BuildLightMap (void)
{
	int			t;
	int			i;
	byte		*lightmap;
	unsigned	scale;
	int			maps;
	msurface_t	*surf;

	surf = r_drawsurf.surf;

	lightmap = surf->samples;

	if (r_fullbright.value || !cl.worldmodel->lightdata)
	{
		std::fill(blocklights, blocklights + r_blocklights_size, 0);
		return;
	}

// clear to ambient
	std::fill(blocklights, blocklights + r_blocklights_size, r_refdef.ambientlight_shift8);


	auto blocklights_trailing = r_blocklights_size - r_blocklights_size % 4;

// add all the lightmaps
	if (lightmap)
	{
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			scale = r_drawsurf.lightadj[maps];	// 8.8 fraction
			for (i=0 ; i<blocklights_trailing ; i+=4)
			{
				blocklights[i] += lightmap[i] * scale;
				blocklights[i + 1] += lightmap[i + 1] * scale;
				blocklights[i + 2] += lightmap[i + 2] * scale;
				blocklights[i + 3] += lightmap[i + 3] * scale;
			}
			for (i=blocklights_trailing ; i<r_blocklights_size; i++)
			{
				blocklights[i] += lightmap[i] * scale;
			}
			lightmap += r_blocklights_size;	// skip to next lightmap
		}
	}

// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights ();

// bound, invert, and shift
	for (i=0 ; i<blocklights_trailing ; i+=4)
	{
		t = (255*256 - (int)blocklights[i]) >> (8 - VID_CBITS);

		if (t < (1 << 6))
			t = (1 << 6);

		blocklights[i] = t;

		t = (255*256 - (int)blocklights[i+1]) >> (8 - VID_CBITS);

		if (t < (1 << 6))
			t = (1 << 6);

		blocklights[i+1] = t;

		t = (255*256 - (int)blocklights[i+2]) >> (8 - VID_CBITS);

		if (t < (1 << 6))
			t = (1 << 6);

		blocklights[i+2] = t;

		t = (255*256 - (int)blocklights[i+3]) >> (8 - VID_CBITS);

		if (t < (1 << 6))
			t = (1 << 6);

		blocklights[i+3] = t;
	}
	for (i=blocklights_trailing ; i<r_blocklights_size ; i++)
	{
		t = (255*256 - (int)blocklights[i]) >> (8 - VID_CBITS);

		if (t < (1 << 6))
			t = (1 << 6);

		blocklights[i] = t;
	}
}


/*
===============
R_BuildColoredLightMap

Combine and scale multiple colored lightmaps into the 8.8 format in blocklights
===============
*/
void R_BuildColoredLightMap (void)
{
	int			t;
	int			i;
	byte		*lightmap;
	unsigned	scale;
	int			maps;
	msurface_t	*surf;

	surf = r_drawsurf.surf;

	lightmap = surf->samplesRGB;

	if (r_fullbright.value || !cl.worldmodel->lightRGBdata)
	{
		std::fill(blocklights, blocklights + r_blocklights_size, 65535);
		return;
	}

// clear to ambient
	std::fill(blocklights, blocklights + r_blocklights_size, r_refdef.ambientlight_shift8);


	auto blocklights_trailing = r_blocklights_size - r_blocklights_size % 4;

// add all the lightmaps
	if (lightmap)
	{
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			scale = r_drawsurf.lightadj[maps];	// 8.8 fraction
			for (i=0 ; i<blocklights_trailing ; i+=4)
			{
				blocklights[i] += lightmap[i] * scale;
				blocklights[i + 1] += lightmap[i + 1] * scale;
				blocklights[i + 2] += lightmap[i + 2] * scale;
				blocklights[i + 3] += lightmap[i + 3] * scale;
			}
			for (i=blocklights_trailing ; i<r_blocklights_size; i++)
			{
				blocklights[i] += lightmap[i] * scale;
			}
			lightmap += r_blocklights_size;	// skip to next lightmap
		}
	}

// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicColoredLights ();
	

// bound
	for (i=0 ; i<blocklights_trailing ; i+=4)
	{
		t = (int)blocklights[i];

		if (t < 255)
			t = 255;
		if (t > 65535)
			t = 65535;

		blocklights[i] = t;

		t = (int)blocklights[i+1];

		if (t < 255)
			t = 255;
		if (t > 65535)
			t = 65535;

		blocklights[i+1] = t;

		t = (int)blocklights[i+2];

		if (t < 255)
			t = 255;
		if (t > 65535)
			t = 65535;

		blocklights[i+2] = t;

		t = (int)blocklights[i+3];

		if (t < 255)
			t = 255;
		if (t > 65535)
			t = 65535;

		blocklights[i+3] = t;
	}
	for (i=blocklights_trailing ; i<r_blocklights_size ; i++)
	{
		t = (int)blocklights[i];

		if (t < 255)
			t = 255;
		if (t > 65535)
			t = 65535;

		blocklights[i] = t;
	}
}


/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
texture_t *R_TextureAnimation (texture_t *base)
{
	int		reletive;
	int		count;

	if (currententity->frame)
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}
	
	if (!base->anim_total)
		return base;

	reletive = (int)(cl.time*10) % base->anim_total;

	count = 0;	
	while (base->anim_min > reletive || base->anim_max <= reletive)
	{
		base = base->anim_next;
		if (!base)
			Sys_Error ("R_TextureAnimation: broken cycle");
		if (++count > 100)
			Sys_Error ("R_TextureAnimation: infinite cycle");
	}

	return base;
}


/*
===============
R_DrawSurface
===============
*/
void R_DrawSurface (void)
{
	unsigned char	*basetptr;
	int				smax, tmax, twidth;
	int				u;
	int				soffset, basetoffset, texwidth;
	int				horzblockstep;
	unsigned char	*pcolumndest;
	void			(*pblockdrawer)(void);
	texture_t		*mt;

// calculate the lightings
	R_BuildLightMap ();
	
	surfrowbytes = r_drawsurf.rowbytes;

	mt = r_drawsurf.texture;
	
	r_source = (byte *)mt + mt->offsets[r_drawsurf.surfmip];
	
// the fractional light values should range from 0 to (VID_GRADES - 1) << 16
// from a source range of 0 - 255
	
	texwidth = mt->width >> r_drawsurf.surfmip;

	blocksize = 16 >> r_drawsurf.surfmip;
	blockdivshift = 4 - r_drawsurf.surfmip;
	blockdivmask = (1 << blockdivshift) - 1;
	
	r_lightwidth = (r_drawsurf.surf->extents[0]>>4)+1;

	r_numhblocks = r_drawsurf.surfwidth >> blockdivshift;
	r_numvblocks = r_drawsurf.surfheight >> blockdivshift;

//==============================

	if (r_pixbytes == 1)
	{
		pblockdrawer = surfmiptable[r_drawsurf.surfmip];
	// TODO: only needs to be set when there is a display settings change
		horzblockstep = blocksize;
	}
	else
	{
		pblockdrawer = R_DrawSurfaceBlock16;
	// TODO: only needs to be set when there is a display settings change
		horzblockstep = blocksize << 1;
	}

	smax = mt->width >> r_drawsurf.surfmip;
	twidth = texwidth;
	tmax = mt->height >> r_drawsurf.surfmip;
	sourcetstep = texwidth;
	r_stepback = tmax * twidth;

	r_sourcemax = r_source + (tmax * smax);

	soffset = r_drawsurf.surf->texturemins[0];
	basetoffset = r_drawsurf.surf->texturemins[1];

// << 16 components are to guarantee positive values for %
	soffset = ((soffset >> r_drawsurf.surfmip) + (smax << 16)) % smax;
	basetptr = &r_source[((((basetoffset >> r_drawsurf.surfmip) 
		+ (tmax << 16)) % tmax) * twidth)];

	pcolumndest = r_drawsurf.surfdat;

	for (u=0 ; u<r_numhblocks; u++)
	{
		r_lightptr = blocklights + u;

		prowdestbase = pcolumndest;

		pbasesource = basetptr + soffset;

		(*pblockdrawer)();

		soffset = soffset + blocksize;
		if (soffset >= smax)
			soffset = 0;

		pcolumndest += horzblockstep;
	}
}


/*
===============
R_DrawSurfaceColored
===============
*/
void R_DrawSurfaceColored (void)
{
	unsigned char	*basetptr;
	int				smax, tmax, twidth;
	int				u;
	int				soffset, basetoffset, texwidth;
	int				horzblockstep;
	unsigned char	*pcolumndest;
	void			(*pblockdrawer)(void);
	texture_t		*mt;

// calculate the lightings
	R_BuildColoredLightMap ();
	
	surfrowbytes = r_drawsurf.rowbytes;

	mt = r_drawsurf.texture;
	
	r_source = (byte *)mt + mt->offsets[r_drawsurf.surfmip];
	
// the fractional light values should range from 0 to (VID_GRADES - 1) << 16
// from a source range of 0 - 255
	
	texwidth = mt->width >> r_drawsurf.surfmip;

	blocksize = 16 >> r_drawsurf.surfmip;
	blockdivshift = 4 - r_drawsurf.surfmip;
	blockdivmask = (1 << blockdivshift) - 1;
	
	r_lightwidth = ((r_drawsurf.surf->extents[0]>>4)+1)*3;

	r_numhblocks = (r_drawsurf.surfwidth >> blockdivshift)*3;
	r_numvblocks = r_drawsurf.surfheight >> blockdivshift;

//==============================

	if (r_pixbytes == 1)
	{
		pblockdrawer = surfmiptablecolored[r_drawsurf.surfmip];
	// TODO: only needs to be set when there is a display settings change
		horzblockstep = blocksize;
	}
	else
	{
		pblockdrawer = R_DrawSurfaceBlock16;
	// TODO: only needs to be set when there is a display settings change
		horzblockstep = blocksize << 1;
	}

	smax = mt->width >> r_drawsurf.surfmip;
	twidth = texwidth;
	tmax = mt->height >> r_drawsurf.surfmip;
	sourcetstep = texwidth;
	r_stepback = tmax * twidth;

	r_sourcemax = r_source + (tmax * smax);

	soffset = r_drawsurf.surf->texturemins[0];
	basetoffset = r_drawsurf.surf->texturemins[1];

// << 16 components are to guarantee positive values for %
	soffset = ((soffset >> r_drawsurf.surfmip) + (smax << 16)) % smax;
	basetptr = &r_source[((((basetoffset >> r_drawsurf.surfmip)
		+ (tmax << 16)) % tmax) * twidth)];

	pcolumndest = r_drawsurf.surfdat;

	pbasepal = host_basepal.data();

	for (u=0 ; u<r_numhblocks; u+=3)
	{
		r_lightptr = blocklights + u;

		prowdestbase = pcolumndest;

		pbasesource = basetptr + soffset;

		(*pblockdrawer)();

		soffset = soffset + blocksize;
		if (soffset >= smax)
			soffset = 0;

		pcolumndest += horzblockstep;
	}
}


//=============================================================================

#if	!id386

/*
================
R_DrawSurfaceBlock8_mip0
================
*/
void R_DrawSurfaceBlock8_mip0 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = (unsigned char*)prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 4;
		lightrightstep = (r_lightptr[1] - lightright) >> 4;

		for (i=0 ; i<16 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 4;

			light = lightright;

			for (b=15; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & VID_CMASK) + pix];
				light += lightstep;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip1
================
*/
void R_DrawSurfaceBlock8_mip1 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = (unsigned char*)prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 3;
		lightrightstep = (r_lightptr[1] - lightright) >> 3;

		for (i=0 ; i<8 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 3;

			light = lightright;

			for (b=7; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & VID_CMASK) + pix];
				light += lightstep;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip2
================
*/
void R_DrawSurfaceBlock8_mip2 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = (unsigned char*)prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 2;
		lightrightstep = (r_lightptr[1] - lightright) >> 2;

		for (i=0 ; i<4 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 2;

			light = lightright;

			for (b=3; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & VID_CMASK) + pix];
				light += lightstep;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip3
================
*/
void R_DrawSurfaceBlock8_mip3 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = (unsigned char*)prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 1;
		lightrightstep = (r_lightptr[1] - lightright) >> 1;

		for (i=0 ; i<2 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 1;

			light = lightright;

			for (b=1; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & VID_CMASK) + pix];
				light += lightstep;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_coloredmip0
================
*/
void R_DrawSurfaceBlock8_coloredmip0 (void)
{
	int				v, i, b;
	int				lightstep_r, lightstep_g, lightstep_b;
	int				lighttemp_r, lighttemp_g, lighttemp_b;
	int				light_r, light_g, light_b;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = (unsigned char*)prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: use delta rather than both right and left, like ASM?
		int lightleft_r = r_lightptr[0];
		int lightleft_g = r_lightptr[1];
		int lightleft_b = r_lightptr[2];
		int lightright_r = r_lightptr[3];
		int lightright_g = r_lightptr[4];
		int lightright_b = r_lightptr[5];
		r_lightptr += r_lightwidth;
		int lightleftstep_r = ((int)r_lightptr[0] - lightleft_r) / 16;
		int lightleftstep_g = ((int)r_lightptr[1] - lightleft_g) / 16;
		int lightleftstep_b = ((int)r_lightptr[2] - lightleft_b) / 16;
		int lightrightstep_r = ((int)r_lightptr[3] - lightright_r) / 16;
		int lightrightstep_g = ((int)r_lightptr[4] - lightright_g) / 16;
		int lightrightstep_b = ((int)r_lightptr[5] - lightright_b) / 16;

		for (i=0 ; i<16 ; i++)
		{
			lighttemp_r = lightleft_r - lightright_r;
			lighttemp_g = lightleft_g - lightright_g;
			lighttemp_b = lightleft_b - lightright_b;
			lightstep_r = lighttemp_r / 16;
			lightstep_g = lighttemp_g / 16;
			lightstep_b = lighttemp_b / 16;

			light_r = lightright_r;
			light_g = lightright_g;
			light_b = lightright_b;

			for (b=15; b>=0; b--)
			{
				pix = psource[b];
				if (pix < 224)
				{
					auto pal = pbasepal + pix*3;
					auto rcomp = ((light_r * (*pal++)) >> 15) & 511;
					auto gcomp = ((light_g * (*pal++)) >> 15) & 511;
					auto bcomp = ((light_b * (*pal)) >> 15) & 511;
					if (rcomp > 255) rcomp = 255;
					if (gcomp > 255) gcomp = 255;
					if (bcomp > 255) bcomp = 255;
					prowdest[b] = r_24to8tableptr[(rcomp << 16) | (gcomp << 8) | bcomp];
				}
				else
				{
					prowdest[b] = pix;
				}
				light_r += lightstep_r;
				light_g += lightstep_g;
				light_b += lightstep_b;
			}
	
			psource += sourcetstep;
			lightright_r += lightrightstep_r;
			lightright_g += lightrightstep_g;
			lightright_b += lightrightstep_b;
			lightleft_r += lightleftstep_r;
			lightleft_g += lightleftstep_g;
			lightleft_b += lightleftstep_b;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_coloredmip1
================
*/
void R_DrawSurfaceBlock8_coloredmip1 (void)
{
	int				v, i, b;
	int				lightstep_r, lightstep_g, lightstep_b;
	int				lighttemp_r, lighttemp_g, lighttemp_b;
	int				light_r, light_g, light_b;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = (unsigned char*)prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: use delta rather than both right and left, like ASM?
		int lightleft_r = r_lightptr[0];
		int lightleft_g = r_lightptr[1];
		int lightleft_b = r_lightptr[2];
		int lightright_r = r_lightptr[3];
		int lightright_g = r_lightptr[4];
		int lightright_b = r_lightptr[5];
		r_lightptr += r_lightwidth;
		int lightleftstep_r = ((int)r_lightptr[0] - lightleft_r) / 8;
		int lightleftstep_g = ((int)r_lightptr[1] - lightleft_g) / 8;
		int lightleftstep_b = ((int)r_lightptr[2] - lightleft_b) / 8;
		int lightrightstep_r = ((int)r_lightptr[3] - lightright_r) / 8;
		int lightrightstep_g = ((int)r_lightptr[4] - lightright_g) / 8;
		int lightrightstep_b = ((int)r_lightptr[5] - lightright_b) / 8;

		for (i=0 ; i<8 ; i++)
		{
			lighttemp_r = lightleft_r - lightright_r;
			lighttemp_g = lightleft_g - lightright_g;
			lighttemp_b = lightleft_b - lightright_b;
			lightstep_r = lighttemp_r / 8;
			lightstep_g = lighttemp_g / 8;
			lightstep_b = lighttemp_b / 8;

			light_r = lightright_r;
			light_g = lightright_g;
			light_b = lightright_b;

			for (b=7; b>=0; b--)
			{
				pix = psource[b];
				if (pix < 224)
				{
					auto pal = pbasepal + pix*3;
					auto rcomp = ((light_r * (*pal++)) >> 15) & 511;
					auto gcomp = ((light_g * (*pal++)) >> 15) & 511;
					auto bcomp = ((light_b * (*pal)) >> 15) & 511;
					if (rcomp > 255) rcomp = 255;
					if (gcomp > 255) gcomp = 255;
					if (bcomp > 255) bcomp = 255;
					prowdest[b] = r_24to8tableptr[(rcomp << 16) | (gcomp << 8) | bcomp];
				}
				else
				{
					prowdest[b] = pix;
				}
				light_r += lightstep_r;
				light_g += lightstep_g;
				light_b += lightstep_b;
			}
	
			psource += sourcetstep;
			lightright_r += lightrightstep_r;
			lightright_g += lightrightstep_g;
			lightright_b += lightrightstep_b;
			lightleft_r += lightleftstep_r;
			lightleft_g += lightleftstep_g;
			lightleft_b += lightleftstep_b;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_coloredmip2
================
*/
void R_DrawSurfaceBlock8_coloredmip2 (void)
{
	int				v, i, b;
	int				lightstep_r, lightstep_g, lightstep_b;
	int				lighttemp_r, lighttemp_g, lighttemp_b;
	int				light_r, light_g, light_b;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = (unsigned char*)prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: use delta rather than both right and left, like ASM?
		int lightleft_r = r_lightptr[0];
		int lightleft_g = r_lightptr[1];
		int lightleft_b = r_lightptr[2];
		int lightright_r = r_lightptr[3];
		int lightright_g = r_lightptr[4];
		int lightright_b = r_lightptr[5];
		r_lightptr += r_lightwidth;
		int lightleftstep_r = ((int)r_lightptr[0] - lightleft_r) / 4;
		int lightleftstep_g = ((int)r_lightptr[1] - lightleft_g) / 4;
		int lightleftstep_b = ((int)r_lightptr[2] - lightleft_b) / 4;
		int lightrightstep_r = ((int)r_lightptr[3] - lightright_r) / 4;
		int lightrightstep_g = ((int)r_lightptr[4] - lightright_g) / 4;
		int lightrightstep_b = ((int)r_lightptr[5] - lightright_b) / 4;

		for (i=0 ; i<4 ; i++)
		{
			lighttemp_r = lightleft_r - lightright_r;
			lighttemp_g = lightleft_g - lightright_g;
			lighttemp_b = lightleft_b - lightright_b;
			lightstep_r = lighttemp_r / 4;
			lightstep_g = lighttemp_g / 4;
			lightstep_b = lighttemp_b / 4;

			light_r = lightright_r;
			light_g = lightright_g;
			light_b = lightright_b;

			for (b=3; b>=0; b--)
			{
				pix = psource[b];
				if (pix < 224)
				{
					auto pal = pbasepal + pix*3;
					auto rcomp = ((light_r * (*pal++)) >> 15) & 511;
					auto gcomp = ((light_g * (*pal++)) >> 15) & 511;
					auto bcomp = ((light_b * (*pal)) >> 15) & 511;
					if (rcomp > 255) rcomp = 255;
					if (gcomp > 255) gcomp = 255;
					if (bcomp > 255) bcomp = 255;
					prowdest[b] = r_24to8tableptr[(rcomp << 16) | (gcomp << 8) | bcomp];
				}
				else
				{
					prowdest[b] = pix;
				}
				light_r += lightstep_r;
				light_g += lightstep_g;
				light_b += lightstep_b;
			}
	
			psource += sourcetstep;
			lightright_r += lightrightstep_r;
			lightright_g += lightrightstep_g;
			lightright_b += lightrightstep_b;
			lightleft_r += lightleftstep_r;
			lightleft_g += lightleftstep_g;
			lightleft_b += lightleftstep_b;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_coloredmip3
================
*/
void R_DrawSurfaceBlock8_coloredmip3 (void)
{
	int				v, i, b;
	int				lightstep_r, lightstep_g, lightstep_b;
	int				lighttemp_r, lighttemp_g, lighttemp_b;
	int				light_r, light_g, light_b;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = (unsigned char*)prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: use delta rather than both right and left, like ASM?
		int lightleft_r = r_lightptr[0];
		int lightleft_g = r_lightptr[1];
		int lightleft_b = r_lightptr[2];
		int lightright_r = r_lightptr[3];
		int lightright_g = r_lightptr[4];
		int lightright_b = r_lightptr[5];
		r_lightptr += r_lightwidth;
		int lightleftstep_r = ((int)r_lightptr[0] - lightleft_r) / 2;
		int lightleftstep_g = ((int)r_lightptr[1] - lightleft_g) / 2;
		int lightleftstep_b = ((int)r_lightptr[2] - lightleft_b) / 2;
		int lightrightstep_r = ((int)r_lightptr[3] - lightright_r) / 2;
		int lightrightstep_g = ((int)r_lightptr[4] - lightright_g) / 2;
		int lightrightstep_b = ((int)r_lightptr[5] - lightright_b) / 2;

		for (i=0 ; i<2 ; i++)
		{
			lighttemp_r = lightleft_r - lightright_r;
			lighttemp_g = lightleft_g - lightright_g;
			lighttemp_b = lightleft_b - lightright_b;
			lightstep_r = lighttemp_r / 2;
			lightstep_g = lighttemp_g / 2;
			lightstep_b = lighttemp_b / 2;

			light_r = lightright_r;
			light_g = lightright_g;
			light_b = lightright_b;

			for (b=1; b>=0; b--)
			{
				pix = psource[b];
				if (pix < 224)
				{
					auto pal = pbasepal + pix*3;
					auto rcomp = ((light_r * (*pal++)) >> 15) & 511;
					auto gcomp = ((light_g * (*pal++)) >> 15) & 511;
					auto bcomp = ((light_b * (*pal)) >> 15) & 511;
					if (rcomp > 255) rcomp = 255;
					if (gcomp > 255) gcomp = 255;
					if (bcomp > 255) bcomp = 255;
					prowdest[b] = r_24to8tableptr[(rcomp << 16) | (gcomp << 8) | bcomp];
				}
				else
				{
					prowdest[b] = pix;
				}
				light_r += lightstep_r;
				light_g += lightstep_g;
				light_b += lightstep_b;
			}
	
			psource += sourcetstep;
			lightright_r += lightrightstep_r;
			lightright_g += lightrightstep_g;
			lightright_b += lightrightstep_b;
			lightleft_r += lightleftstep_r;
			lightleft_g += lightleftstep_g;
			lightleft_b += lightleftstep_b;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock16

FIXME: make this work
================
*/
void R_DrawSurfaceBlock16 (void)
{
	int				k;
	unsigned char	*psource;
	int				lighttemp, lightstep, light;
	unsigned short	*prowdest;

	prowdest = (unsigned short *)prowdestbase;

	for (k=0 ; k<blocksize ; k++)
	{
		unsigned short	*pdest;
		unsigned char	pix;
		int				b;

		psource = pbasesource;
		lighttemp = lightright - lightleft;
		lightstep = lighttemp >> blockdivshift;

		light = lightleft;
		pdest = prowdest;

		for (b=0; b<blocksize; b++)
		{
			pix = *psource;
			*pdest = vid.colormap16[(light & 0xFF00) + pix];
			psource += sourcesstep;
			pdest++;
			light += lightstep;
		}

		pbasesource += sourcetstep;
		lightright += lightrightstep;
		lightleft += lightleftstep;
		prowdest = (unsigned short *)((size_t)prowdest + surfrowbytes);
	}

	prowdestbase = prowdest;
}

#endif


//============================================================================

/*
================
R_GenTurbTile
================
*/
void R_GenTurbTile (const pixel_t *pbasetex, void *pdest)
{
	int		*turb;
	int		i, j, s, t;
	byte	*pd;
	
	turb = sintable.data() + ((int)(cl.time*SPEED)&(CYCLE-1));
	pd = (byte *)pdest;

	for (i=0 ; i<TILE_SIZE ; i++)
	{
		for (j=0 ; j<TILE_SIZE ; j++)
		{	
			s = (((j << 16) + turb[i & (CYCLE-1)]) >> 16) & 63;
			t = (((i << 16) + turb[j & (CYCLE-1)]) >> 16) & 63;
			*pd++ = *(pbasetex + (t<<6) + s);
		}
	}
}


/*
================
R_GenTurbTile16
================
*/
void R_GenTurbTile16 (const pixel_t *pbasetex, void *pdest)
{
	int				*turb;
	int				i, j, s, t;
	unsigned short	*pd;

	turb = sintable.data() + ((int)(cl.time*SPEED)&(CYCLE-1));
	pd = (unsigned short *)pdest;

	for (i=0 ; i<TILE_SIZE ; i++)
	{
		for (j=0 ; j<TILE_SIZE ; j++)
		{	
			s = (((j << 16) + turb[i & (CYCLE-1)]) >> 16) & 63;
			t = (((i << 16) + turb[j & (CYCLE-1)]) >> 16) & 63;
			*pd++ = d_8to16table[*(pbasetex + (t<<6) + s)];
		}
	}
}


/*
================
R_GenTile
================
*/
void R_GenTile (msurface_t *psurf, void *pdest)
{
	if (psurf->flags & SURF_DRAWTURB)
	{
		if (r_pixbytes == 1)
		{
			R_GenTurbTile ((pixel_t *)
				((byte *)psurf->texinfo->texture + psurf->texinfo->texture->offsets[0]), pdest);
		}
		else
		{
			R_GenTurbTile16 ((pixel_t *)
				((byte *)psurf->texinfo->texture + psurf->texinfo->texture->offsets[0]), pdest);
		}
	}
	else if (psurf->flags & SURF_DRAWSKY)
	{
		if (r_pixbytes == 1)
		{
			R_GenSkyTile (pdest);
		}
		else
		{
			R_GenSkyTile16 (pdest);
		}
	}
	else
	{
		Sys_Error ("Unknown tile type");
	}
}

