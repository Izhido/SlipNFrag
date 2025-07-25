// r_aliasc.c: routines for setting up to draw alias models with colored lighting

#include "quakedef.h"
#include "d_lists.h"
#include "r_local.h"
#include "d_local.h"	// FIXME: shouldn't be needed (is needed for patch
						// right now, but that should move)

#define LIGHT_MIN	5		// lowest light value we'll allow, to avoid the
							//  need for inner-loop light clamping

byte*				abasepal;

extern trivertx_t	*r_apverts;

// TODO: these probably will go away with optimized rasterization
extern vec3_t		r_plightvec;
argbcolor_t			r_ambientcoloredlight;
argbcolorf_t		r_shadecoloredlight;
finalcoloredvert_t	*pfinalcoloredverts;
static float		ziscale;
static model_t		*pmodel;

static vec3_t		alias_forward, alias_right, alias_up;

static maliasskindesc_t	*pskindesc;

extern int			r_anumverts;

extern float	aliastransform[3][4];
extern void R_AliasTransformVector (const vec3_t in, vec3_t out);

typedef struct {
	int	index0;
	int	index1;
} aedge_t;

static aedge_t	aedges[12] = {
{0, 1}, {1, 2}, {2, 3}, {3, 0},
{4, 5}, {5, 6}, {6, 7}, {7, 4},
{0, 5}, {1, 4}, {2, 7}, {3, 6}
};

#define NUMVERTEXNORMALS	162

extern float	r_avertexnormals[NUMVERTEXNORMALS][3];

qboolean R_AliasColoredTransformAndProjectFinalVerts (finalcoloredvert_t *fv,
	stvert_t *pstverts);
void R_AliasColoredSetUpTransform (int trivial_accept);
void R_AliasColoredTransformFinalVert (finalcoloredvert_t *fv, auxvert_t *av,
	trivertx_t *pverts, stvert_t *pstverts);
void R_AliasColoredProjectFinalVert (finalcoloredvert_t *fv, auxvert_t *av);


/*
================
R_AliasColoredCheckBBox
================
*/
qboolean R_AliasColoredCheckBBox (void)
{
	int					i, flags, frame, numv;
	aliashdr_t			*pahdr;
	float				zi, basepts[8][3], v0, v1, frac;
	finalcoloredvert_t	*pv0, *pv1, viewpts[16];
	auxvert_t			*pa0, *pa1, viewaux[16];
	maliasframedesc_t	*pframedesc;
	qboolean			zclipped, zfullyclipped;
	unsigned			anyclip, allclip;
	int					minz;
	
// expand, rotate, and translate points into worldspace

	currententity->trivial_accept = 0;
	pmodel = currententity->model;
	pahdr = (aliashdr_t*)Mod_Extradata (pmodel);
	pmdl = (mdl_t *)((byte *)pahdr + pahdr->model);

	R_AliasColoredSetUpTransform (0);

// construct the base bounding box for this frame
	frame = currententity->frame;
// TODO: don't repeat this check when drawing?
	if ((frame >= pmdl->numframes) || (frame < 0))
	{
		Con_DPrintf ("No such frame %d %s\n", frame,
				pmodel->name.c_str());
		frame = 0;
	}

	pframedesc = &pahdr->frames[frame];

// x worldspace coordinates
	basepts[0][0] = basepts[1][0] = basepts[2][0] = basepts[3][0] =
			(float)pframedesc->bboxmin.v[0];
	basepts[4][0] = basepts[5][0] = basepts[6][0] = basepts[7][0] =
			(float)pframedesc->bboxmax.v[0];

// y worldspace coordinates
	basepts[0][1] = basepts[3][1] = basepts[5][1] = basepts[6][1] =
			(float)pframedesc->bboxmin.v[1];
	basepts[1][1] = basepts[2][1] = basepts[4][1] = basepts[7][1] =
			(float)pframedesc->bboxmax.v[1];

// z worldspace coordinates
	basepts[0][2] = basepts[1][2] = basepts[4][2] = basepts[5][2] =
			(float)pframedesc->bboxmin.v[2];
	basepts[2][2] = basepts[3][2] = basepts[6][2] = basepts[7][2] =
			(float)pframedesc->bboxmax.v[2];

	zclipped = false;
	zfullyclipped = true;

	minz = 9999;
	for (i=0; i<8 ; i++)
	{
		R_AliasTransformVector  (&basepts[i][0], &viewaux[i].fv[0]);

		if (viewaux[i].fv[2] < ALIAS_Z_CLIP_PLANE)
		{
		// we must clip points that are closer than the near clip plane
			viewpts[i].flags = ALIAS_Z_CLIP;
			zclipped = true;
		}
		else
		{
			if (viewaux[i].fv[2] < minz)
				minz = viewaux[i].fv[2];
			viewpts[i].flags = 0;
			zfullyclipped = false;
		}
	}

	
	if (zfullyclipped)
	{
		return false;	// everything was near-z-clipped
	}

	numv = 8;

	if (zclipped)
	{
	// organize points by edges, use edges to get new points (possible trivial
	// reject)
		for (i=0 ; i<12 ; i++)
		{
		// edge endpoints
			pv0 = &viewpts[aedges[i].index0];
			pv1 = &viewpts[aedges[i].index1];
			pa0 = &viewaux[aedges[i].index0];
			pa1 = &viewaux[aedges[i].index1];

		// if one end is clipped and the other isn't, make a new point
			if (pv0->flags ^ pv1->flags)
			{
				frac = (ALIAS_Z_CLIP_PLANE - pa0->fv[2]) /
					   (pa1->fv[2] - pa0->fv[2]);
				viewaux[numv].fv[0] = pa0->fv[0] +
						(pa1->fv[0] - pa0->fv[0]) * frac;
				viewaux[numv].fv[1] = pa0->fv[1] +
						(pa1->fv[1] - pa0->fv[1]) * frac;
				viewaux[numv].fv[2] = ALIAS_Z_CLIP_PLANE;
				viewpts[numv].flags = 0;
				numv++;
			}
		}
	}

// project the vertices that remain after clipping
	anyclip = 0;
	allclip = ALIAS_XY_CLIP_MASK;

// TODO: probably should do this loop in ASM, especially if we use floats
	for (i=0 ; i<numv ; i++)
	{
	// we don't need to bother with vertices that were z-clipped
		if (viewpts[i].flags & ALIAS_Z_CLIP)
			continue;

		zi = 1.0 / viewaux[i].fv[2];

	// FIXME: do with chop mode in ASM, or convert to float
		v0 = (viewaux[i].fv[0] * xscale * zi) + xcenter;
		v1 = (viewaux[i].fv[1] * yscale * zi) + ycenter;

		flags = 0;

		if (v0 < r_refdef.fvrectx)
			flags |= ALIAS_LEFT_CLIP;
		if (v1 < r_refdef.fvrecty)
			flags |= ALIAS_TOP_CLIP;
		if (v0 > r_refdef.fvrectright)
			flags |= ALIAS_RIGHT_CLIP;
		if (v1 > r_refdef.fvrectbottom)
			flags |= ALIAS_BOTTOM_CLIP;

		anyclip |= flags;
		allclip &= flags;
	}

	if (allclip)
		return false;	// trivial reject off one side

	currententity->trivial_accept = !anyclip & !zclipped;

	if (currententity->trivial_accept)
	{
		if (minz > (r_aliastransition + (pmdl->size * r_resfudge)))
		{
			currententity->trivial_accept |= 2;
		}
	}

	return true;
}


/*
================
R_AliasColoredPreparePoints

General clipped case
================
*/
void R_AliasColoredPreparePoints (void)
{
	int			i;
	stvert_t	*pstverts;
	finalcoloredvert_t	*fv;
	auxvert_t	*av;
	mtriangle_t	*ptri;
	finalcoloredvert_t	*pfv[3];

	pstverts = (stvert_t *)((byte *)paliashdr + paliashdr->stverts);
	r_anumverts = pmdl->numverts;
 	fv = pfinalcoloredverts;
	av = pauxverts;

	for (i=0 ; i<r_anumverts ; i++, fv++, av++, r_apverts++, pstverts++)
	{
		R_AliasColoredTransformFinalVert (fv, av, r_apverts, pstverts);
		if (av->fv[2] < ALIAS_Z_CLIP_PLANE)
			fv->flags |= ALIAS_Z_CLIP;
		else
		{
			 R_AliasColoredProjectFinalVert (fv, av);

			if (fv->v[0] < r_refdef.aliasvrect.x)
				fv->flags |= ALIAS_LEFT_CLIP;
			if (fv->v[1] < r_refdef.aliasvrect.y)
				fv->flags |= ALIAS_TOP_CLIP;
			if (fv->v[0] > r_refdef.aliasvrectright)
				fv->flags |= ALIAS_RIGHT_CLIP;
			if (fv->v[1] > r_refdef.aliasvrectbottom)
				fv->flags |= ALIAS_BOTTOM_CLIP;	
		}
	}

//
// clip and draw all triangles
//
	r_affinetridesc.numtriangles = 1;

	ptri = (mtriangle_t *)((byte *)paliashdr + paliashdr->triangles);
	for (i=0 ; i<pmdl->numtris ; i++, ptri++)
	{
		pfv[0] = &pfinalcoloredverts[ptri->vertindex[0]];
		pfv[1] = &pfinalcoloredverts[ptri->vertindex[1]];
		pfv[2] = &pfinalcoloredverts[ptri->vertindex[2]];

		if ( pfv[0]->flags & pfv[1]->flags & pfv[2]->flags & (ALIAS_XY_CLIP_MASK | ALIAS_Z_CLIP) )
			continue;		// completely clipped
		
		if ( ! ( (pfv[0]->flags | pfv[1]->flags | pfv[2]->flags) &
			(ALIAS_XY_CLIP_MASK | ALIAS_Z_CLIP) ) )
		{	// totally unclipped
			r_affinetridesc.pfinalverts = NULL;
			r_affinetridesc.pfinalcoloredverts = pfinalcoloredverts;
			r_affinetridesc.ptriangles = ptri;
			D_PolysetDraw ();
		}
		else		
		{	// partially clipped
			R_AliasColoredClipTriangle (ptri);
		}
	}
}


/*
================
R_AliasColoredSetUpTransform
================
*/
void R_AliasColoredSetUpTransform (int trivial_accept)
{
	int				i;
	float			rotationmatrix[3][4], t2matrix[3][4];
	static float	tmatrix[3][4];
	static float	viewmatrix[3][4];
	vec3_t			angles;

// TODO: should really be stored with the entity instead of being reconstructed
// TODO: should use a look-up table
// TODO: could cache lazily, stored in the entity

	angles[ROLL] = currententity->angles[ROLL];
	angles[PITCH] = -currententity->angles[PITCH];
	angles[YAW] = currententity->angles[YAW];
	AngleVectors (angles, alias_forward, alias_right, alias_up);

	tmatrix[0][0] = pmdl->scale[0];
	tmatrix[1][1] = pmdl->scale[1];
	tmatrix[2][2] = pmdl->scale[2];

	tmatrix[0][3] = pmdl->scale_origin[0];
	tmatrix[1][3] = pmdl->scale_origin[1];
	tmatrix[2][3] = pmdl->scale_origin[2];

// TODO: can do this with simple matrix rearrangement

	for (i=0 ; i<3 ; i++)
	{
		t2matrix[i][0] = alias_forward[i];
		t2matrix[i][1] = -alias_right[i];
		t2matrix[i][2] = alias_up[i];
	}

	t2matrix[0][3] = -modelorg[0];
	t2matrix[1][3] = -modelorg[1];
	t2matrix[2][3] = -modelorg[2];

// FIXME: can do more efficiently than full concatenation
	R_ConcatTransforms (t2matrix, tmatrix, rotationmatrix);

// TODO: should be global, set when vright, etc., set
	VectorCopy (vright, viewmatrix[0]);
	VectorCopy (vup, viewmatrix[1]);
	VectorInverse (viewmatrix[1]);
	VectorCopy (vpn, viewmatrix[2]);

//	viewmatrix[0][3] = 0;
//	viewmatrix[1][3] = 0;
//	viewmatrix[2][3] = 0;

	R_ConcatTransforms (viewmatrix, rotationmatrix, aliastransform);

// do the scaling up of x and y to screen coordinates as part of the transform
// for the unclipped case (it would mess up clipping in the clipped case).
// Also scale down z, so 1/z is scaled 31 bits for free, and scale down x and y
// correspondingly so the projected x and y come out right
// FIXME: make this work for clipped case too?
	if (trivial_accept)
	{
		for (i=0 ; i<4 ; i++)
		{
			aliastransform[0][i] *= aliasxscale *
					(1.0 / ((float)0x8000 * 0x10000));
			aliastransform[1][i] *= aliasyscale *
					(1.0 / ((float)0x8000 * 0x10000));
			aliastransform[2][i] *= 1.0 / ((float)0x8000 * 0x10000);

		}
	}
}


/*
================
R_AliasColoredTransformFinalVert
================
*/
void R_AliasColoredTransformFinalVert (finalcoloredvert_t *fv, auxvert_t *av,
	trivertx_t *pverts, stvert_t *pstverts)
{
	argbcolor_t	temp;
	float	lightcos, *plightnormal;

	av->fv[0] = DotProduct(pverts->v, aliastransform[0]) +
			aliastransform[0][3];
	av->fv[1] = DotProduct(pverts->v, aliastransform[1]) +
			aliastransform[1][3];
	av->fv[2] = DotProduct(pverts->v, aliastransform[2]) +
			aliastransform[2][3];

	fv->v[2] = pstverts->s;
	fv->v[3] = pstverts->t;

	fv->flags = pstverts->onseam;

// lighting
	plightnormal = r_avertexnormals[pverts->lightnormalindex];
	lightcos = DotProduct (plightnormal, r_plightvec);
	temp = r_ambientcoloredlight;

	if (lightcos < 0)
	{
		temp.color[0] += (int)(r_shadecoloredlight.color[0] * lightcos);
		temp.color[1] += (int)(r_shadecoloredlight.color[1] * lightcos);
		temp.color[2] += (int)(r_shadecoloredlight.color[2] * lightcos);

	// clamp; because we limited the minimum ambient and shading light, we
	// don't have to clamp low light, just bright
		if (temp.color[0] < 0)
			temp.color[0] = 0;
		if (temp.color[1] < 0)
			temp.color[1] = 0;
		if (temp.color[2] < 0)
			temp.color[2] = 0;
	}

	fv->v[4] = temp.color[0];
	fv->v[5] = temp.color[1];
	fv->v[6] = temp.color[2];
}


#if	!id386

/*
================
R_AliasColoredTransformAndProjectFinalVerts
================
*/
qboolean R_AliasColoredTransformAndProjectFinalVerts (finalcoloredvert_t *fv, stvert_t *pstverts)
{
	int			i;
	argbcolor_t temp;
	float		lightcos, *plightnormal, zi;
	trivertx_t	*pverts;

	pverts = r_apverts;

	for (i=0 ; i<r_anumverts ; i++, fv++, pverts++, pstverts++)
	{
	// transform and project
		zi = 1.0 / (DotProduct(pverts->v, aliastransform[2]) +
				aliastransform[2][3]);

	// x, y, and z are scaled down by 1/2**31 in the transform, so 1/z is
	// scaled up by 1/2**31, and the scaling cancels out for x and y in the
	// projection
		fv->v[7] = zi;

		fv->v[0] = ((DotProduct(pverts->v, aliastransform[0]) +
				aliastransform[0][3]) * zi) + aliasxcenter;
		fv->v[1] = ((DotProduct(pverts->v, aliastransform[1]) +
				aliastransform[1][3]) * zi) + aliasycenter;

		if (fv->v[0] < r_refdef.aliasvrect.x || fv->v[0] >= r_refdef.aliasvrectright ||
			fv->v[1] < r_refdef.aliasvrect.y || fv->v[1] >= r_refdef.aliasvrectbottom)
		{
			return false;
		}

		fv->v[2] = pstverts->s;
		fv->v[3] = pstverts->t;
		fv->flags = pstverts->onseam;

	// lighting
		plightnormal = r_avertexnormals[pverts->lightnormalindex];
		lightcos = DotProduct (plightnormal, r_plightvec);
		temp = r_ambientcoloredlight;

		if (lightcos < 0)
		{
			temp.color[0] += (int)(r_shadecoloredlight.color[0] * lightcos);
			temp.color[1] += (int)(r_shadecoloredlight.color[1] * lightcos);
			temp.color[2] += (int)(r_shadecoloredlight.color[2] * lightcos);

		// clamp; because we limited the minimum ambient and shading light, we
		// don't have to clamp low light, just bright
			if (temp.color[0] < 0)
				temp.color[0] = 0;
			if (temp.color[1] < 0)
				temp.color[1] = 0;
			if (temp.color[2] < 0)
				temp.color[2] = 0;
		}

		fv->v[4] = temp.color[0];
		fv->v[5] = temp.color[1];
		fv->v[6] = temp.color[2];
	}

	return true;
}

#endif


/*
================
 R_AliasColoredProjectFinalVert
================
*/
void R_AliasColoredProjectFinalVert (finalcoloredvert_t *fv, auxvert_t *av)
{
	float	zi;

// project points
	zi = 1.0 / av->fv[2];

	fv->v[7] = zi * ziscale;

	fv->v[0] = (av->fv[0] * aliasxscale * zi) + aliasxcenter;
	fv->v[1] = (av->fv[1] * aliasyscale * zi) + aliasycenter;
}


/*
================
R_AliasColoredPrepareUnclippedPoints
================
*/
void R_AliasColoredPrepareUnclippedPoints (void)
{
	stvert_t	*pstverts;
	finalcoloredvert_t	*fv;

	pstverts = (stvert_t *)((byte *)paliashdr + paliashdr->stverts);
	r_anumverts = pmdl->numverts;
// FIXME: just use pfinalverts directly?
	fv = pfinalcoloredverts;

	if (!R_AliasColoredTransformAndProjectFinalVerts (fv, pstverts))
		return;

	if (r_affinetridesc.drawtype)
		D_PolysetDrawFinalColoredVerts (fv, r_anumverts);

	r_affinetridesc.pfinalverts = NULL;
	r_affinetridesc.pfinalcoloredverts = pfinalcoloredverts;
	r_affinetridesc.ptriangles = (mtriangle_t *)
			((byte *)paliashdr + paliashdr->triangles);
	r_affinetridesc.numtriangles = pmdl->numtris;

	D_PolysetDraw ();
}

/*
===============
R_AliasColoredSetupSkin
===============
*/
void R_AliasColoredSetupSkin (void)
{
	int					skinnum;
	int					i, numskins;
	maliasskingroup_t	*paliasskingroup;
	float				*pskinintervals, fullskininterval;
	float				skintargettime, skintime;

	skinnum = currententity->skinnum;
	if ((skinnum >= pmdl->numskins) || (skinnum < 0))
	{
		Con_DPrintf ("R_AliasColoredSetupSkin: no such skin # %d\n", skinnum);
		skinnum = 0;
	}

	pskindesc = ((maliasskindesc_t *)
			((byte *)paliashdr + paliashdr->skindesc)) + skinnum;
	a_skinwidth = pmdl->skinwidth;

	if (pskindesc->type == ALIAS_SKIN_GROUP)
	{
		paliasskingroup = (maliasskingroup_t *)((byte *)paliashdr +
				pskindesc->skin);
		pskinintervals = (float *)
				((byte *)paliashdr + paliasskingroup->intervals);
		numskins = paliasskingroup->numskins;
		fullskininterval = pskinintervals[numskins-1];
	
		skintime = cl.time + currententity->syncbase;
	
	// when loading in Mod_LoadAliasSkinGroup, we guaranteed all interval
	// values are positive, so we don't have to worry about division by 0
		skintargettime = skintime -
				((int)(skintime / fullskininterval)) * fullskininterval;
	
		for (i=0 ; i<(numskins-1) ; i++)
		{
			if (pskinintervals[i] > skintargettime)
				break;
		}
	
		pskindesc = &paliasskingroup->skindescs[i];
	}

	r_affinetridesc.pskindesc = pskindesc;
	r_affinetridesc.pskin = (void *)((byte *)paliashdr + pskindesc->skin);
	r_affinetridesc.skinwidth = a_skinwidth;
	r_affinetridesc.seamfixupX16 =  (a_skinwidth >> 1) << 16;
	r_affinetridesc.skinheight = pmdl->skinheight;
}

/*
================
R_AliasColoredSetupLighting
================
*/
void R_AliasColoredSetupLighting (acoloredlight_t *plighting)
{

// guarantee that no vertex will ever be lit below LIGHT_MIN, so we don't have
// to clamp off the bottom
	r_ambientcoloredlight = plighting->ambientlight;

	if (r_ambientcoloredlight.color[0] < LIGHT_MIN)
		r_ambientcoloredlight.color[0] = LIGHT_MIN;
	if (r_ambientcoloredlight.color[1] < LIGHT_MIN)
		r_ambientcoloredlight.color[1] = LIGHT_MIN;
	if (r_ambientcoloredlight.color[2] < LIGHT_MIN)
		r_ambientcoloredlight.color[2] = LIGHT_MIN;

	r_ambientcoloredlight.color[0] = (255 - r_ambientcoloredlight.color[0]) << VID_CBITS;
	r_ambientcoloredlight.color[1] = (255 - r_ambientcoloredlight.color[1]) << VID_CBITS;
	r_ambientcoloredlight.color[2] = (255 - r_ambientcoloredlight.color[2]) << VID_CBITS;

	if (r_ambientcoloredlight.color[0] < LIGHT_MIN)
		r_ambientcoloredlight.color[0] = LIGHT_MIN;
	if (r_ambientcoloredlight.color[1] < LIGHT_MIN)
		r_ambientcoloredlight.color[1] = LIGHT_MIN;
	if (r_ambientcoloredlight.color[2] < LIGHT_MIN)
		r_ambientcoloredlight.color[2] = LIGHT_MIN;

	r_shadecoloredlight.color[0] = plighting->shadelight.color[0];
	r_shadecoloredlight.color[1] = plighting->shadelight.color[1];
	r_shadecoloredlight.color[2] = plighting->shadelight.color[2];

	if (r_shadecoloredlight.color[0] < 0)
		r_shadecoloredlight.color[0] = 0;
	if (r_shadecoloredlight.color[1] < 0)
		r_shadecoloredlight.color[1] = 0;
	if (r_shadecoloredlight.color[2] < 0)
		r_shadecoloredlight.color[2] = 0;

	r_shadecoloredlight.color[0] *= VID_GRADES;
	r_shadecoloredlight.color[1] *= VID_GRADES;
	r_shadecoloredlight.color[2] *= VID_GRADES;

// rotate the lighting vector into the model's frame of reference
	r_plightvec[0] = DotProduct (plighting->plightvec, alias_forward);
	r_plightvec[1] = -DotProduct (plighting->plightvec, alias_right);
	r_plightvec[2] = DotProduct (plighting->plightvec, alias_up);
}

/*
=================
R_AliasColoredSetupFrame

set r_apverts
=================
*/
void R_AliasColoredSetupFrame (void)
{
	int				frame;
	int				i, numframes;
	maliasgroup_t	*paliasgroup;
	float			*pintervals, fullinterval, targettime, time;

	frame = currententity->frame;
	if ((frame >= pmdl->numframes) || (frame < 0))
	{
		Con_DPrintf ("R_AliasColoredSetupFrame: no such frame %d\n", frame);
		frame = 0;
	}

	if (paliashdr->frames[frame].type == ALIAS_SINGLE)
	{
		r_apverts = (trivertx_t *)
				((byte *)paliashdr + paliashdr->frames[frame].frame);
		return;
	}
	
	paliasgroup = (maliasgroup_t *)
				((byte *)paliashdr + paliashdr->frames[frame].frame);
	pintervals = (float *)((byte *)paliashdr + paliasgroup->intervals);
	numframes = paliasgroup->numframes;
	fullinterval = pintervals[numframes-1];

	time = cl.time + currententity->syncbase;

//
// when loading in Mod_LoadAliasGroup, we guaranteed all interval values
// are positive, so we don't have to worry about division by 0
//
	targettime = time - ((int)(time / fullinterval)) * fullinterval;

	for (i=0 ; i<(numframes-1) ; i++)
	{
		if (pintervals[i] > targettime)
			break;
	}

	r_apverts = (trivertx_t *)
				((byte *)paliashdr + paliasgroup->frames[i].frame);
}


/*
================
R_AliasColoredDrawModel
================
*/
std::vector<finalcoloredvert_t> r_aliasfinalcoloredverts;
std::vector<auxvert_t> r_aliascoloredauxverts;

void R_AliasColoredDrawModel (acoloredlight_t *plighting)
{
	r_amodels_drawn++;

// cache align
    paliashdr = (aliashdr_t *)Mod_Extradata (currententity->model);
    pmdl = (mdl_t *)((byte *)paliashdr + paliashdr->model);
	r_holey = (currententity->model->flags & MF_HOLEY);
	r_aliasalpha = currententity->alpha;

    if (pmdl->numverts + ((CACHE_SIZE - 1) / sizeof(finalcoloredvert_t)) + 1 > r_aliasfinalcoloredverts.size())
    {
        r_aliasfinalcoloredverts.resize(pmdl->numverts + ((CACHE_SIZE - 1) / sizeof(finalcoloredvert_t)) + 1);
    }
    if (pmdl->numverts > r_aliascoloredauxverts.size())
    {
        r_aliascoloredauxverts.resize(pmdl->numverts);
    }
	pfinalcoloredverts = (finalcoloredvert_t *)
			(((size_t)&r_aliasfinalcoloredverts[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));
	pauxverts = &r_aliascoloredauxverts[0];

	R_AliasColoredSetupSkin ();
	R_AliasColoredSetUpTransform (currententity->trivial_accept);
	R_AliasColoredSetupLighting (plighting);
	R_AliasColoredSetupFrame ();

	if (d_uselists)
	{
		if (currententity == &cl.viewent)
		{
			if (r_holey)
				D_AddViewmodelHoleyColoredLightsToLists (paliashdr, pskindesc, r_apverts, currententity);
			else
				D_AddViewmodelColoredLightsToLists (paliashdr, pskindesc, r_apverts, currententity);
		}
		else if (currententity->alpha > 0)
		{
			if (r_holey)
				D_AddAliasHoleyAlphaColoredLightsToLists (paliashdr, pskindesc, r_apverts, currententity);
			else
				D_AddAliasAlphaColoredLightsToLists (paliashdr, pskindesc, r_apverts, currententity);
		}
		else
		{
			if (r_holey)
				D_AddAliasHoleyColoredLightsToLists (paliashdr, pskindesc, r_apverts, currententity);
			else
				D_AddAliasColoredLightsToLists (paliashdr, pskindesc, r_apverts, currententity);
		}
		return;
	}

	if (!currententity->colormap)
		Sys_Error ("R_AliasColoredDrawModel: !currententity->colormap");

	r_affinetridesc.drawtype = (currententity->trivial_accept == 3);

	if (r_affinetridesc.drawtype)
	{
		D_PolysetUpdateTables ();		// FIXME: precalc...
	}
	else
	{
#if	id386
		D_Aff8Patch (currententity->colormap);
#endif
	}

	abasepal = host_basepal.data();

	if (currententity != &cl.viewent)
		ziscale = (float)0x8000 * (float)0x10000;
	else
		ziscale = (float)0x8000 * (float)0x10000 * 3.0;

	if (currententity->trivial_accept)
		R_AliasColoredPrepareUnclippedPoints ();
	else
		R_AliasColoredPreparePoints ();
}

