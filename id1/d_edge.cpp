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
// d_edge.c

#include "quakedef.h"
#include "d_local.h"
#include "d_lists.h"

static int	miplevel;

float		scale_for_mip;
int			ubasestep, errorterm, erroradjustup, erroradjustdown;

extern int screenwidth;

// FIXME: should go away
extern void			R_RotateBmodel (void);
extern void			R_TransformFrustum (void);

vec3_t		transformed_modelorg;

extern qboolean	r_skyinitialized;
extern qboolean	r_skyRGBAinitialized;
extern qboolean	r_skyboxinitialized;
extern mtexinfo_t	r_skytexinfo[6];

/*
==============
D_DrawPoly

==============
*/
void D_DrawPoly (void)
{
// this driver takes spans, not polygons
}


/*
=============
D_MipLevelForScale
=============
*/
int D_MipLevelForScale (float scale)
{
	int		lmiplevel;

	if (scale >= d_scalemip[0] )
		lmiplevel = 0;
	else if (scale >= d_scalemip[1] )
		lmiplevel = 1;
	else if (scale >= d_scalemip[2] )
		lmiplevel = 2;
	else
		lmiplevel = 3;

	if (lmiplevel < d_minmip)
		lmiplevel = d_minmip;

	return lmiplevel;
}


/*
==============
D_DrawSolidSurface
==============
*/

// FIXME: clean this up

void D_DrawSolidSurface (surf_t *surf, int color)
{
	espan_t	*span;
	byte	*pdest;
	int		u, u2, pix;
	
	pix = (color<<24) | (color<<16) | (color<<8) | color;
	for (span=surf->spans ; span ; span=span->pnext)
	{
		pdest = (byte *)d_viewbuffer + screenwidth*span->v;
		u = span->u;
		u2 = span->u + span->count - 1;
		((byte *)pdest)[u] = pix;

		if (u2 - u < 8)
		{
			for (u++ ; u <= u2 ; u++)
				((byte *)pdest)[u] = pix;
		}
		else
		{
			for (u++ ; u & 3 ; u++)
				((byte *)pdest)[u] = pix;

			u2 -= 4;
			for ( ; u <= u2 ; u+=4)
				*(int *)((byte *)pdest + u) = pix;
			u2 += 4;
			for ( ; u <= u2 ; u++)
				((byte *)pdest)[u] = pix;
		}
	}
}


/*
==============
D_CalcGradients
==============
*/
void D_CalcGradients (msurface_t *pface)
{
	mplane_t	*pplane;
	float		mipscale;
	vec3_t		p_temp1;
	vec3_t		p_saxis, p_taxis;
	float		t;

	pplane = pface->plane;

	mipscale = 1.0 / (float)(1 << miplevel);

	TransformVector (pface->texinfo->vecs[0], p_saxis);
	TransformVector (pface->texinfo->vecs[1], p_taxis);

	t = xscaleinv * mipscale;
	d_sdivzstepu = p_saxis[0] * t;
	d_tdivzstepu = p_taxis[0] * t;

	t = yscaleinv * mipscale;
	d_sdivzstepv = -p_saxis[1] * t;
	d_tdivzstepv = -p_taxis[1] * t;

	d_sdivzorigin = p_saxis[2] * mipscale - xcenter * d_sdivzstepu -
			ycenter * d_sdivzstepv;
	d_tdivzorigin = p_taxis[2] * mipscale - xcenter * d_tdivzstepu -
			ycenter * d_tdivzstepv;

	VectorScale (transformed_modelorg, mipscale, p_temp1);

	t = 0x10000*mipscale;
	sadjust = ((fixed16_t)(DotProduct (p_temp1, p_saxis) * 0x10000 + 0.5)) -
			((pface->texturemins[0] << 16) >> miplevel)
			+ pface->texinfo->vecs[0][3]*t;
	tadjust = ((fixed16_t)(DotProduct (p_temp1, p_taxis) * 0x10000 + 0.5)) -
			((pface->texturemins[1] << 16) >> miplevel)
			+ pface->texinfo->vecs[1][3]*t;

//
// -1 (-epsilon) so we never wander off the edge of the texture
//
	bbextents = ((pface->extents[0] << 16) >> miplevel) - 1;
	bbextentt = ((pface->extents[1] << 16) >> miplevel) - 1;
}

/*
==============
D_TurbCalcGradients
==============
*/
// Special version for turbulent surfaces - assumes miplevel = 0, texturemins = -8192 and extents = 16384
void D_TurbCalcGradients (msurface_t *pface)
{
	vec3_t		p_saxis, p_taxis;

	TransformVector (pface->texinfo->vecs[0], p_saxis);
	TransformVector (pface->texinfo->vecs[1], p_taxis);

	d_sdivzstepu = p_saxis[0] * xscaleinv;
	d_tdivzstepu = p_taxis[0] * xscaleinv;

	d_sdivzstepv = -p_saxis[1] * yscaleinv;
	d_tdivzstepv = -p_taxis[1] * yscaleinv;

	d_sdivzorigin = p_saxis[2] - xcenter * d_sdivzstepu -
			ycenter * d_sdivzstepv;
	d_tdivzorigin = p_taxis[2] - xcenter * d_tdivzstepu -
			ycenter * d_tdivzstepv;

	sadjust = ((fixed16_t)(DotProduct (transformed_modelorg, p_saxis) * 0x10000 + 0.5)) -
			(-8192 * 0x10000)
			+ pface->texinfo->vecs[0][3]*0x10000;
	tadjust = ((fixed16_t)(DotProduct (transformed_modelorg, p_taxis) * 0x10000 + 0.5)) -
			(-8192 * 0x10000)
			+ pface->texinfo->vecs[1][3]*0x10000;

	r_turb_lm_sadjust = ((fixed16_t)(DotProduct (transformed_modelorg, p_saxis) * 0x10000 + 0.5)) -
			(pface->texturemins[0] * 0x10000)
			+ pface->texinfo->vecs[0][3]*0x10000;
	r_turb_lm_tadjust = ((fixed16_t)(DotProduct (transformed_modelorg, p_taxis) * 0x10000 + 0.5)) -
			(pface->texturemins[1] * 0x10000)
			+ pface->texinfo->vecs[1][3]*0x10000;

//
// -1 (-epsilon) so we never wander off the edge of the texture
//
	bbextents = (16384 << 16) - 1;
	bbextentt = (16384 << 16) - 1;

	r_turb_lm_bbextents = (pface->extents[0] << 16) - 1;
	r_turb_lm_bbextentt = (pface->extents[1] << 16) - 1;
	
	r_turb_extents0 = pface->extents[0];
	r_turb_extents1 = pface->extents[1];
}


/*
==============
D_DrawSurfaces
==============
*/
void D_DrawSurfaces (void)
{
	surf_t			*s;
	msurface_t		*pface;
	surfcache_t		*pcurrentcache;
	vec3_t			world_transformed_modelorg;
	vec3_t			local_modelorg;

	currententity = &cl_entities[0];
	TransformVector (modelorg, transformed_modelorg);
	VectorCopy (transformed_modelorg, world_transformed_modelorg);

// TODO: could preset a lot of this at mode set time
	if (r_drawflat.value)
	{
		for (s = &surfaces[1] ; s<surface_p ; s++)
		{
			if (!s->spans)
				continue;

			d_zistepu = s->d_zistepu;
			d_zistepv = s->d_zistepv;
			d_ziorigin = s->d_ziorigin;

			D_DrawSolidSurface (s, (int)((size_t)s->data) & 0xFF);
			(*d_drawzspans) (s->spans);
		}
	}
	else
	{
		// First, draw all non-holey spans:
		for (s = &surfaces[1] ; s<surface_p ; s++)
		{
			if (!s->spans)
				continue;

			d_zistepu = s->d_zistepu;
			d_zistepv = s->d_zistepv;
			d_ziorigin = s->d_ziorigin;

			if (s->flags & SURF_DRAWSKY)
			{
				r_drawnpolycount++;

				if (r_skyinitialized)
				{
					if (!r_skymade)
					{
						R_MakeSky ();
					}

					D_DrawSkyScans8 (s->spans);
					(*d_drawzspans) (s->spans);
				}
			}
			else if (s->flags & SURF_DRAWSKYBOX)
			{
				r_drawnpolycount++;

				pface = (msurface_t*)s->data;
				miplevel = 0;
				if (!pface->texinfo->texture)
					return;
				cacheblock = (pixel_t *)
						((byte *)pface->texinfo->texture +
						 pface->texinfo->texture->offsets[0]);
				cachewidth = pface->texinfo->texture->width;

				d_zistepu = s->d_zistepu;
				d_zistepv = s->d_zistepv;
				d_ziorigin = s->d_ziorigin;

				D_CalcGradients (pface);

				(*d_drawspans) (s->spans);

				// set up a gradient for the background surface that places it
				// effectively at infinity distance from the viewpoint
				d_zistepu = 0;
				d_zistepv = 0;
				d_ziorigin = -0.9;

				(*d_drawzspans) (s->spans);
			}
			else if (s->flags & SURF_DRAWBACKGROUND)
			{
				r_drawnpolycount++;

			// set up a gradient for the background surface that places it
			// effectively at infinity distance from the viewpoint
				d_zistepu = 0;
				d_zistepv = 0;
				d_ziorigin = -0.9;

				D_DrawSolidSurface (s, (int)r_clearcolor.value & 0xFF);
				(*d_drawzspans) (s->spans);
			}
			else if (s->flags & SURF_DRAWTURB)
			{
				if (!s->isfence && s->alpha == 0)
				{
					r_drawnpolycount++;

					pface = (msurface_t*)s->data;
					miplevel = 0;
					cacheblock = (pixel_t *)
							((byte *)pface->texinfo->texture +
							 pface->texinfo->texture->offsets[0]);
					cachewidth = pface->texinfo->texture->width;
					r_turb_cacheheight = pface->texinfo->texture->height;

					if (s->insubmodel)
					{
					// FIXME: we don't want to do all this for every polygon!
					// TODO: store once at start of frame
						currententity = s->entity;	//FIXME: make this passed in to
													// R_RotateBmodel ()
						VectorSubtract (r_origin, currententity->origin,
								local_modelorg);
						TransformVector (local_modelorg, transformed_modelorg);

						R_RotateBmodel ();	// FIXME: don't mess with the frustum,
											// make entity passed in
					}

					D_TurbCalcGradients (pface);
					if (pface->samples != nullptr)
					{
						auto texture = R_TextureAnimation (pface->texinfo->texture);
						pcurrentcache = D_CacheLightmap (pface, texture);
						if (pcurrentcache == nullptr)
						{
							Turbulent8Non64 (s->spans);
						}
						else
						{
							r_turb_lightmapblock = (unsigned*)&(pcurrentcache->data[0]);
							r_turb_lightmapwidthminusone = (pcurrentcache->width / sizeof(unsigned)) - 1;
							r_turb_lightmapheightminusone = pcurrentcache->height - 1;
							r_turb_lightmapwidthminusone16 = r_turb_lightmapwidthminusone << 16;
							r_turb_lightmapheightminusone16 = r_turb_lightmapheightminusone << 16;
							TurbulentLit8 (s->spans);
						}
					}
					else if (cachewidth == 64 && r_turb_cacheheight == 64)
					{
						Turbulent8 (s->spans);
					}
					else
					{
						Turbulent8Non64 (s->spans);
					}
					(*d_drawzspans) (s->spans);

					if (s->insubmodel)
					{
					//
					// restore the old drawing state
					// FIXME: we don't want to do this every time!
					// TODO: speed up
					//
						currententity = &cl_entities[0];
						VectorCopy (world_transformed_modelorg,
									transformed_modelorg);
						VectorCopy (base_vpn, vpn);
						VectorCopy (base_vup, vup);
						VectorCopy (base_vright, vright);
						VectorCopy (base_modelorg, modelorg);
						R_TransformFrustum ();
					}
				}
			}
			else if (!s->isfence && s->alpha == 0)
			{
				r_drawnpolycount++;

				if (s->insubmodel)
				{
				// FIXME: we don't want to do all this for every polygon!
				// TODO: store once at start of frame
					currententity = s->entity;	//FIXME: make this passed in to
												// R_RotateBmodel ()
					VectorSubtract (r_origin, currententity->origin, local_modelorg);
					TransformVector (local_modelorg, transformed_modelorg);

					R_RotateBmodel ();	// FIXME: don't mess with the frustum,
										// make entity passed in
				}

				pface = (msurface_t*)s->data;
				miplevel = D_MipLevelForScale (s->nearzi * scale_for_mip
				* pface->texinfo->mipadjust);

			// FIXME: make this passed in to D_CacheSurface
				pcurrentcache = D_CacheSurface (pface, miplevel);

				if (pcurrentcache != nullptr)
				{
					cacheblock = (pixel_t *)pcurrentcache->data;
					cachewidth = pcurrentcache->width;

					D_CalcGradients (pface);

					(*d_drawspans) (s->spans);

					(*d_drawzspans) (s->spans);
				}

				if (s->insubmodel)
				{
				//
				// restore the old drawing state
				// FIXME: we don't want to do this every time!
				// TODO: speed up
				//
					currententity = &cl_entities[0];
					VectorCopy (world_transformed_modelorg,
								transformed_modelorg);
					VectorCopy (base_vpn, vpn);
					VectorCopy (base_vup, vup);
					VectorCopy (base_vright, vright);
					VectorCopy (base_modelorg, modelorg);
					R_TransformFrustum ();
				}
			}
		}
		// Then, draw holey spans (fences, dithered alpha turbulent or regular surfaces):
		for (auto holey = r_holeysurfs ; holey < r_holeysurf_p ; holey++)
		{
			s = &surfaces[*holey];

			if (!s->spans)
				continue;

			if (s->flags & SURF_DRAWTURB)
			{
				r_drawnpolycount++;

				d_zistepu = s->d_zistepu;
				d_zistepv = s->d_zistepv;
				d_ziorigin = s->d_ziorigin;

				pface = (msurface_t*)s->data;
				miplevel = 0;
				cacheblock = (pixel_t *)
						((byte *)pface->texinfo->texture +
						 pface->texinfo->texture->offsets[0]);
				cachewidth = pface->texinfo->texture->width;
				r_turb_cacheheight = pface->texinfo->texture->height;

				if (s->insubmodel)
				{
				// FIXME: we don't want to do all this for every polygon!
				// TODO: store once at start of frame
					currententity = s->entity;	//FIXME: make this passed in to
												// R_RotateBmodel ()
					VectorSubtract (r_origin, currententity->origin,
							local_modelorg);
					TransformVector (local_modelorg, transformed_modelorg);

					R_RotateBmodel ();	// FIXME: don't mess with the frustum,
										// make entity passed in
				}

				D_TurbCalcGradients (pface);
				if (pface->samples != nullptr)
				{
					auto texture = R_TextureAnimation (pface->texinfo->texture);
					pcurrentcache = D_CacheLightmap (pface, texture);
					if (pcurrentcache == nullptr)
					{
						TurbulentAlpha8Non64 (s->spans, s->alpha);
					}
					else
					{
						r_turb_lightmapblock = (unsigned*)&(pcurrentcache->data[0]);
						r_turb_lightmapwidthminusone = (pcurrentcache->width / sizeof(unsigned)) - 1;
						r_turb_lightmapheightminusone = pcurrentcache->height - 1;
						r_turb_lightmapwidthminusone16 = r_turb_lightmapwidthminusone << 16;
						r_turb_lightmapheightminusone16 = r_turb_lightmapheightminusone << 16;
						TurbulentLitAlpha8 (s->spans, s->alpha);
					}
				}
                else if (cachewidth == 64 && r_turb_cacheheight == 64)
                {
                    TurbulentAlpha8 (s->spans, s->alpha);
                }
                else
                {
					TurbulentAlpha8Non64 (s->spans, s->alpha);
                }

				if (s->insubmodel)
				{
				//
				// restore the old drawing state
				// FIXME: we don't want to do this every time!
				// TODO: speed up
				//
					currententity = &cl_entities[0];
					VectorCopy (world_transformed_modelorg,
								transformed_modelorg);
					VectorCopy (base_vpn, vpn);
					VectorCopy (base_vup, vup);
					VectorCopy (base_vright, vright);
					VectorCopy (base_modelorg, modelorg);
					R_TransformFrustum ();
				}
			}
			else
			{
				r_drawnpolycount++;

				d_zistepu = s->d_zistepu;
				d_zistepv = s->d_zistepv;
				d_ziorigin = s->d_ziorigin;

				// Fence textures are assumed to be referenced only by regular surfaces:
				if (s->insubmodel)
				{
				// FIXME: we don't want to do all this for every polygon!
				// TODO: store once at start of frame
					currententity = s->entity;	//FIXME: make this passed in to
												// R_RotateBmodel ()
					VectorSubtract (r_origin, currententity->origin, local_modelorg);
					TransformVector (local_modelorg, transformed_modelorg);

					R_RotateBmodel ();	// FIXME: don't mess with the frustum,
										// make entity passed in
				}

				pface = (msurface_t*)s->data;
				miplevel = D_MipLevelForScale (s->nearzi * scale_for_mip
				* pface->texinfo->mipadjust);

			// FIXME: make this passed in to D_CacheSurface
				pcurrentcache = D_CacheSurface (pface, miplevel);

				if (pcurrentcache != nullptr)
				{
					cacheblock = (pixel_t *)pcurrentcache->data;
					cachewidth = pcurrentcache->width;

					D_CalcGradients (pface);

					if (s->isfence && s->alpha != 0)
						D_DrawFenceAlphaSpans8 (s->spans, s->alpha);
					else if (s->isfence)
						D_DrawFenceSpans8 (s->spans);
					else
						D_DrawAlphaSpans8 (s->spans, s->alpha);
				}

				if (s->insubmodel)
				{
				//
				// restore the old drawing state
				// FIXME: we don't want to do this every time!
				// TODO: speed up
				//
					currententity = &cl_entities[0];
					VectorCopy (world_transformed_modelorg,
								transformed_modelorg);
					VectorCopy (base_vpn, vpn);
					VectorCopy (base_vup, vup);
					VectorCopy (base_vright, vright);
					VectorCopy (base_modelorg, modelorg);
					R_TransformFrustum ();
				}
			}
		}
	}
}


/*
==============
D_DrawTurbulentToLists
==============
*/
void D_DrawTurbulentToLists (msurface_t* pface)
{
	auto texture = R_TextureAnimation (pface->texinfo->texture);
	auto alpha = R_AlphaForSurface(pface, currententity);
	if (pface->samplesRGB != nullptr)
	{
		auto pcurrentcache = D_CacheColoredLightmap (pface, texture);
		if (pcurrentcache != nullptr)
		{
			if (currententity->origin[0] == 0 &&
				currententity->origin[1] == 0 &&
				currententity->origin[2] == 0 &&
				currententity->angles[YAW] == 0 &&
				currententity->angles[PITCH] == 0 &&
				currententity->angles[ROLL] == 0 &&
				alpha == 0)
			{
				if (texture->external_color != nullptr)
					D_AddTurbulentRGBAColoredLightsToLists (pface, pcurrentcache, currententity);
				else
					D_AddTurbulentColoredLightsToLists (pface, pcurrentcache, currententity);
			}
			else if (texture->external_color != nullptr)
				D_AddTurbulentRotatedRGBAColoredLightsToLists (pface, pcurrentcache, currententity, alpha);
			else
				D_AddTurbulentRotatedColoredLightsToLists (pface, pcurrentcache, currententity, alpha);
		}
	}
	else if (pface->samples != nullptr)
	{
		auto pcurrentcache = D_CacheLightmap (pface, texture);
		if (pcurrentcache != nullptr)
		{
			if (currententity->origin[0] == 0 &&
				currententity->origin[1] == 0 &&
				currententity->origin[2] == 0 &&
				currententity->angles[YAW] == 0 &&
				currententity->angles[PITCH] == 0 &&
				currententity->angles[ROLL] == 0 &&
				alpha == 0)
			{
				if (texture->external_color != nullptr)
					D_AddTurbulentRGBALitToLists (pface, pcurrentcache, currententity);
				else
					D_AddTurbulentLitToLists (pface, pcurrentcache, currententity);
			}
			else if (texture->external_color != nullptr)
				D_AddTurbulentRotatedRGBALitToLists (pface, pcurrentcache, currententity, alpha);
			else
				D_AddTurbulentRotatedLitToLists (pface, pcurrentcache, currententity, alpha);
		}
	}
	else if (currententity->origin[0] == 0 &&
	         currententity->origin[1] == 0 &&
	         currententity->origin[2] == 0 &&
	         currententity->angles[YAW] == 0 &&
	         currententity->angles[PITCH] == 0 &&
	         currententity->angles[ROLL] == 0 &&
			 alpha == 0)
	{
		if (texture->external_color != nullptr)
			D_AddTurbulentRGBAToLists (pface, currententity);
		else
			D_AddTurbulentToLists (pface, currententity);
	}
	else if (texture->external_color != nullptr)
		D_AddTurbulentRotatedRGBAToLists (pface, currententity, alpha);
	else
		D_AddTurbulentRotatedToLists (pface, currententity, alpha);
}


/*
==============
D_DrawSurfaceToLists
==============
*/
void D_DrawSurfaceToLists (msurface_t* pface)
{
	auto texture = R_TextureAnimation (pface->texinfo->texture);
	auto alpha = R_AlphaForSurface(pface, currententity);
	if (pface->samplesRGB != NULL)
	{
		auto pcurrentcache = D_CacheColoredLightmap (pface, texture);
		if (pcurrentcache != nullptr)
		{
			if (currententity->origin[0] == 0 &&
				currententity->origin[1] == 0 &&
				currententity->origin[2] == 0 &&
				currententity->angles[YAW] == 0 &&
				currententity->angles[PITCH] == 0 &&
				currententity->angles[ROLL] == 0 &&
				alpha == 0)
			{
				if (pface->flags & SURF_DRAWFENCE)
				{
					if (texture->external_color != nullptr && texture->external_glow != nullptr)
						D_AddFenceRGBAColoredLightsToLists (pface, pcurrentcache, currententity);
					else if (texture->external_color != nullptr)
						D_AddFenceRGBANoGlowColoredLightsToLists (pface, pcurrentcache, currententity);
					else
						D_AddFenceColoredLightsToLists (pface, pcurrentcache, currententity);
				}
				else if (texture->external_color != nullptr && texture->external_glow != nullptr)
					D_AddSurfaceRGBAColoredLightsToLists (pface, pcurrentcache, currententity);
				else if (texture->external_color != nullptr)
					D_AddSurfaceRGBANoGlowColoredLightsToLists (pface, pcurrentcache, currententity);
				else
					D_AddSurfaceColoredLightsToLists (pface, pcurrentcache, currententity);
			}
			else if (pface->flags & SURF_DRAWFENCE)
			{
				if (texture->external_color != nullptr && texture->external_glow != nullptr)
					D_AddFenceRotatedRGBAColoredLightsToLists (pface, pcurrentcache, currententity, alpha);
				else if (texture->external_color != nullptr)
					D_AddFenceRotatedRGBANoGlowColoredLightsToLists (pface, pcurrentcache, currententity, alpha);
				else
					D_AddFenceRotatedColoredLightsToLists (pface, pcurrentcache, currententity, alpha);
			}
			else if (texture->external_color != nullptr && texture->external_glow != nullptr)
				D_AddSurfaceRotatedRGBAColoredLightsToLists (pface, pcurrentcache, currententity, alpha);
			else if (texture->external_color != nullptr)
				D_AddSurfaceRotatedRGBANoGlowColoredLightsToLists (pface, pcurrentcache, currententity, alpha);
			else
				D_AddSurfaceRotatedColoredLightsToLists (pface, pcurrentcache, currententity, alpha);
		}
	}
	else
	{
		auto pcurrentcache = D_CacheLightmap (pface, texture);
		if (pcurrentcache != nullptr)
		{
			if (currententity->origin[0] == 0 &&
				currententity->origin[1] == 0 &&
				currententity->origin[2] == 0 &&
				currententity->angles[YAW] == 0 &&
				currententity->angles[PITCH] == 0 &&
				currententity->angles[ROLL] == 0 &&
				alpha == 0)
			{
				if (pface->flags & SURF_DRAWFENCE)
				{
					if (texture->external_color != nullptr && texture->external_glow != nullptr)
						D_AddFenceRGBAToLists (pface, pcurrentcache, currententity);
					else if (texture->external_color != nullptr)
						D_AddFenceRGBANoGlowToLists (pface, pcurrentcache, currententity);
					else
						D_AddFenceToLists (pface, pcurrentcache, currententity);
				}
				else if (texture->external_color != nullptr && texture->external_glow != nullptr)
					D_AddSurfaceRGBAToLists (pface, pcurrentcache, currententity);
				else if (texture->external_color != nullptr)
					D_AddSurfaceRGBANoGlowToLists (pface, pcurrentcache, currententity);
				else
					D_AddSurfaceToLists (pface, pcurrentcache, currententity);
			}
			else if (pface->flags & SURF_DRAWFENCE)
			{
				if (texture->external_color != nullptr && texture->external_glow != nullptr)
					D_AddFenceRotatedRGBAToLists (pface, pcurrentcache, currententity, alpha);
				else if (texture->external_color != nullptr)
					D_AddFenceRotatedRGBANoGlowToLists (pface, pcurrentcache, currententity, alpha);
				else
					D_AddFenceRotatedToLists (pface, pcurrentcache, currententity, alpha);
			}
			else if (texture->external_color != nullptr && texture->external_glow != nullptr)
				D_AddSurfaceRotatedRGBAToLists (pface, pcurrentcache, currententity, alpha);
			else if (texture->external_color != nullptr)
				D_AddSurfaceRotatedRGBANoGlowToLists (pface, pcurrentcache, currententity, alpha);
			else
				D_AddSurfaceRotatedToLists (pface, pcurrentcache, currententity, alpha);
		}
	}
}

