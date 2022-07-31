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
// d_surf.c: rasterization driver surface heap manager

#include "quakedef.h"
#include "d_local.h"
#include "r_local.h"

float           surfscale;
qboolean        r_cache_thrash;         // set if surface cache is thrashing

int                                     sc_size;
surfcache_t                     *sc_rover, *sc_base;

#define GUARDSIZE       4


int     D_SurfaceCacheForRes (int width, int height)
{
	int             size, pix;

	if (COM_CheckParm ("-surfcachesize"))
	{
		size = Q_atoi(com_argv[COM_CheckParm("-surfcachesize")+1]) * 1024;
		return size;
	}
	
	size = SURFCACHE_SIZE_AT_320X200;

	pix = width*height;
	if (pix > 64000)
		size += (pix-64000)*3;
		

	return size;
}

void D_CheckCacheGuard (void)
{
	byte    *s;
	int             i;

	s = (byte *)sc_base + sc_size;
	for (i=0 ; i<GUARDSIZE ; i++)
		if (s[i] != (byte)i)
			Sys_Error ("D_CheckCacheGuard: failed");
}

void D_ClearCacheGuard (void)
{
	byte    *s;
	int             i;
	
	s = (byte *)sc_base + sc_size;
	for (i=0 ; i<GUARDSIZE ; i++)
		s[i] = (byte)i;
}


/*
================
D_InitCaches

================
*/
void D_InitCaches (void *buffer, int size)
{

	if (!msg_suppress_1)
		Con_Printf ("%ik surface cache\n", size/1024);

	sc_size = size - GUARDSIZE;
	sc_base = (surfcache_t *)buffer;
	sc_rover = sc_base;
	
	sc_base->next = NULL;
	sc_base->owner = NULL;
	sc_base->size = sc_size;
	
	D_ClearCacheGuard ();
	Mod_ClearCacheSurfaces();
}


/*
==================
D_FlushCaches
==================
*/
void D_FlushCaches (void)
{
	surfcache_t     *c;
	
	if (!sc_base)
		return;

	for (c = sc_base ; c ; c = c->next)
	{
		if (c->owner)
			*c->owner = NULL;
	}
	
	sc_rover = sc_base;
	sc_base->next = NULL;
	sc_base->owner = NULL;
	sc_base->size = sc_size;
}

/*
=================
D_SCAlloc
=================
*/
surfcache_t     *D_SCAlloc (int width, int size)
{
	surfcache_t             *new_surf;
	qboolean                wrapped_this_time;

	if (width < 0)
	{
		Con_Printf ("D_SCAlloc: bad cache width %d\n", width);
		return nullptr;
	}

	if (size <= 0)
	{
		Con_Printf ("D_SCAlloc: bad cache size %d\n", size);
		return nullptr;
	}
	
    auto size_ptr = (size_t)&((surfcache_t *)0)->data[size];
    size = (int)size_ptr;
	size = (size + 7) & ~7;
	if (size > sc_size)
	{
		Con_Printf ("D_SCAlloc: %i > cache size\n",size);
		return nullptr;
	}

// if there is not size bytes after the rover, reset to the start
	wrapped_this_time = false;

	if ( !sc_rover || (byte *)sc_rover - (byte *)sc_base > sc_size - size)
	{
		if (sc_rover)
		{
			wrapped_this_time = true;
		}
		sc_rover = sc_base;
	}
		
// colect and free surfcache_t blocks until the rover block is large enough
	new_surf = sc_rover;
	if (sc_rover->owner)
		*sc_rover->owner = NULL;
	
	while (new_surf->size < size)
	{
		if (sc_rover->next == nullptr)
		{
			Con_Printf ("D_SCAlloc: hit the end of memory\n");

			if (d_roverwrapped)
			{
				if (wrapped_this_time || (sc_rover >= d_initial_rover))
					r_cache_thrash = true;
			}
			else if (wrapped_this_time)
			{
				d_roverwrapped = true;
			}

			return nullptr;
		}
	// free another
		sc_rover = sc_rover->next;
		if (sc_rover->owner)
			*sc_rover->owner = NULL;
			
		new_surf->size += sc_rover->size;
		new_surf->next = sc_rover->next;
	}

// create a fragment out of any leftovers
	if (new_surf->size - size > 256)
	{
		sc_rover = (surfcache_t *)( (byte *)new_surf + size);
		sc_rover->size = new_surf->size - size;
		sc_rover->next = new_surf->next;
		sc_rover->width = 0;
		sc_rover->owner = NULL;
		new_surf->next = sc_rover;
		new_surf->size = size;
	}
	else
		sc_rover = new_surf->next;
	
	new_surf->width = width;
// DEBUG
	if (width > 0)
		new_surf->height = (size - sizeof(*new_surf) + sizeof(new_surf->data)) / width;

	new_surf->owner = NULL;              // should be set properly after return

	if (d_roverwrapped)
	{
		if (wrapped_this_time || (sc_rover >= d_initial_rover))
			r_cache_thrash = true;
	}
	else if (wrapped_this_time)
	{       
		d_roverwrapped = true;
	}

D_CheckCacheGuard ();   // DEBUG
	return new_surf;
}


/*
=================
D_SCDump
=================
*/
void D_SCDump (void)
{
	surfcache_t             *test;

	for (test = sc_base ; test ; test = test->next)
	{
		if (test == sc_rover)
			Sys_Printf ("ROVER:\n");
		printf ("%p : %i bytes     %i width\n",test, test->size, test->width);
	}
}

//=============================================================================

// if the num is not a power of 2, assume it will not repeat

int     MaskForNum (int num)
{
	if (num==128)
		return 127;
	if (num==64)
		return 63;
	if (num==32)
		return 31;
	if (num==16)
		return 15;
	return 255;
}

int D_log2 (int num)
{
	int     c;
	
	c = 0;
	
	while (num>>=1)
		c++;
	return c;
}

//=============================================================================

/*
================
D_CacheSurface
================
*/
surfcache_t *D_CacheSurface (msurface_t *surface, int miplevel)
{
	surfcache_t     *cache;

//
// if the surface is animating or flashing, flush the cache
//
	r_drawsurf.texture = R_TextureAnimation (surface->texinfo->texture);
	r_drawsurf.lightadj[0] = d_lightstylevalue[surface->styles[0]];
	r_drawsurf.lightadj[1] = d_lightstylevalue[surface->styles[1]];
	r_drawsurf.lightadj[2] = d_lightstylevalue[surface->styles[2]];
	r_drawsurf.lightadj[3] = d_lightstylevalue[surface->styles[3]];
	
//
// see if the cache holds apropriate data
//
	cache = surface->cachespots[miplevel];

	if (cache && !cache->dlight && surface->dlightframe != r_framecount
			&& cache->texture == r_drawsurf.texture
			&& cache->lightadj[0] == r_drawsurf.lightadj[0]
			&& cache->lightadj[1] == r_drawsurf.lightadj[1]
			&& cache->lightadj[2] == r_drawsurf.lightadj[2]
			&& cache->lightadj[3] == r_drawsurf.lightadj[3] )
		return cache;

//
// determine shape of surface
//
	surfscale = 1.0 / (1<<miplevel);
	r_drawsurf.surfmip = miplevel;
	r_drawsurf.surfwidth = surface->extents[0] >> miplevel;
	r_drawsurf.rowbytes = r_drawsurf.surfwidth;
	r_drawsurf.surfheight = surface->extents[1] >> miplevel;
	
//
// allocate memory if needed
//
	if (!cache)     // if a texture just animated, don't reallocate it
	{
		cache = D_SCAlloc (r_drawsurf.surfwidth,
						   r_drawsurf.surfwidth * r_drawsurf.surfheight);
		surface->cachespots[miplevel] = cache;
		if (cache == nullptr)
		{
			return cache;
		}
		cache->owner = &surface->cachespots[miplevel];
		cache->mipscale = surfscale;
	}
	
	if (surface->dlightframe == r_framecount)
		cache->dlight = 1;
	else
		cache->dlight = 0;

	r_drawsurf.surfdat = (pixel_t *)cache->data;
	
	cache->texture = r_drawsurf.texture;
	cache->lightadj[0] = r_drawsurf.lightadj[0];
	cache->lightadj[1] = r_drawsurf.lightadj[1];
	cache->lightadj[2] = r_drawsurf.lightadj[2];
	cache->lightadj[3] = r_drawsurf.lightadj[3];

	cache->created = r_framecount;

//
// draw and light the surface texture
//
	r_drawsurf.surf = surface;

	r_blocklights_smax = (surface->extents[0]>>4)+1;
	r_blocklights_tmax = (surface->extents[1]>>4)+1;
	r_blocklights_size = r_blocklights_smax*r_blocklights_tmax;
	if (r_blocklights_base.size() < r_blocklights_size + 2*2)
	{
		r_blocklights_base.resize(r_blocklights_size + 2*2);
	}
	blocklights = r_blocklights_base.data();

	c_surf++;
	R_DrawSurface ();

	return surface->cachespots[miplevel];
}

//=============================================================================

/*
================
D_CacheLightmap
================
*/
surfcache_t* D_CacheLightmap (msurface_t *surface)
{
	surfcache_t     *cache;

//
// if the surface is animating or flashing, flush the cache
//
	r_drawsurf.texture = R_TextureAnimation (surface->texinfo->texture);
	r_drawsurf.lightadj[0] = d_lightstylevalue[surface->styles[0]];
	r_drawsurf.lightadj[1] = d_lightstylevalue[surface->styles[1]];
	r_drawsurf.lightadj[2] = d_lightstylevalue[surface->styles[2]];
	r_drawsurf.lightadj[3] = d_lightstylevalue[surface->styles[3]];

//
// determine shape of surface
//
	r_blocklights_smax = (surface->extents[0]>>4)+1;
	r_blocklights_tmax = (surface->extents[1]>>4)+1;
	r_blocklights_size = r_blocklights_smax*r_blocklights_tmax;

//
// see if the cache holds apropriate data
//
	cache = surface->cachespots[0];

	if (cache && !cache->dlight && surface->dlightframe != r_framecount
			&& cache->texture == r_drawsurf.texture
			&& cache->lightadj[0] == r_drawsurf.lightadj[0]
			&& cache->lightadj[1] == r_drawsurf.lightadj[1]
			&& cache->lightadj[2] == r_drawsurf.lightadj[2]
			&& cache->lightadj[3] == r_drawsurf.lightadj[3]
			&& cache->size >= r_blocklights_size * sizeof(unsigned) )
		return cache;

//
// allocate memory if needed
//
	if (!cache)     // if a texture just animated, don't reallocate it
	{
		cache = D_SCAlloc (r_blocklights_smax * sizeof(unsigned),
						   r_blocklights_size * sizeof(unsigned));
		surface->cachespots[0] = cache;
		if (cache == nullptr)
		{
			return cache;
		}
		cache->owner = &surface->cachespots[0];
		cache->mipscale = 1;
	}

	if (surface->dlightframe == r_framecount)
		cache->dlight = 1;
	else
		cache->dlight = 0;

	r_drawsurf.surfdat = (pixel_t *)cache->data;

	cache->texture = r_drawsurf.texture;
	cache->lightadj[0] = r_drawsurf.lightadj[0];
	cache->lightadj[1] = r_drawsurf.lightadj[1];
	cache->lightadj[2] = r_drawsurf.lightadj[2];
	cache->lightadj[3] = r_drawsurf.lightadj[3];

	cache->created = r_framecount;

//
// draw and light the surface texture
//
	r_drawsurf.surf = surface;

	blocklights = (unsigned*)&(cache->data[0]);

	c_surf++;
	R_BuildLightMap ();

	return surface->cachespots[0];
}


