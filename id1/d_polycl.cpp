// d_polycl.c: routines for drawing sets of polygons with colored lighting sharing the same
// texture (used for Alias models)

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct {
	void			*pdest;
	short			*pz;
	int				count;
	byte			*ptex;
    int				sfrac, tfrac, lightr, lightg, lightb;
    long long       zi;
} coloredspanpackage_t;

typedef struct {
	int		isflattop;
	int		numleftedges;
	int		*pleftedgevert0;
	int		*pleftedgevert1;
	int		*pleftedgevert2;
	int		numrightedges;
	int		*prightedgevert0;
	int		*prightedgevert1;
	int		*prightedgevert2;
} colorededgetable;

int	r_p0colored[8], r_p1colored[8], r_p2colored[8];

extern int	d_xdenom;

colorededgetable	*pcolorededgetable;

colorededgetable	colorededgetables[12] = {
	{0, 1, r_p0colored, r_p2colored, NULL, 2, r_p0colored, r_p1colored, r_p2colored },
	{0, 2, r_p1colored, r_p0colored, r_p2colored,   1, r_p1colored, r_p2colored, NULL},
	{1, 1, r_p0colored, r_p2colored, NULL, 1, r_p1colored, r_p2colored, NULL},
	{0, 1, r_p1colored, r_p0colored, NULL, 2, r_p1colored, r_p2colored, r_p0colored },
	{0, 2, r_p0colored, r_p2colored, r_p1colored,   1, r_p0colored, r_p1colored, NULL},
	{0, 1, r_p2colored, r_p1colored, NULL, 1, r_p2colored, r_p0colored, NULL},
	{0, 1, r_p2colored, r_p1colored, NULL, 2, r_p2colored, r_p0colored, r_p1colored },
	{0, 2, r_p2colored, r_p1colored, r_p0colored,   1, r_p2colored, r_p0colored, NULL},
	{0, 1, r_p1colored, r_p0colored, NULL, 1, r_p1colored, r_p2colored, NULL},
	{1, 1, r_p2colored, r_p1colored, NULL, 1, r_p0colored, r_p1colored, NULL},
	{1, 1, r_p1colored, r_p0colored, NULL, 1, r_p2colored, r_p0colored, NULL},
	{0, 1, r_p0colored, r_p2colored, NULL, 1, r_p0colored, r_p1colored, NULL},
};

// FIXME: some of these can become statics
extern int		a_sstepxfrac, a_tstepxfrac;
int 			r_lrstepx, r_lgstepx, r_lbstepx;
extern int		a_ststepxwhole;
extern int		r_sstepx, r_tstepx;
int				r_lrstepy, r_lgstepy, r_lbstepy;
extern int		r_sstepy, r_tstepy;
extern int		r_zistepx, r_zistepy;
extern int		d_aspancount, d_countextrastep;

coloredspanpackage_t	*a_coloredspans;
coloredspanpackage_t	*d_pedgecoloredspanpackage;
static int				ystart;
extern byte				*d_pdest, *d_ptex;
extern short			*d_pz;
extern int				d_sfrac, d_tfrac;
int						d_lightr, d_lightg, d_lightb;
extern long long 		d_zi;
extern int				d_ptexextrastep, d_sfracextrastep;
extern int				d_tfracextrastep;
int						d_lightrextrastep, d_lightgextrastep, d_lightbextrastep;
extern int				d_pdestextrastep;
int						d_lightrbasestep, d_lightgbasestep, d_lightbbasestep;
extern int				d_pdestbasestep, d_ptexbasestep;
extern int				d_sfracbasestep, d_tfracbasestep;
extern int				d_ziextrastep, d_zibasestep;
extern int				d_pzextrastep, d_pzbasestep;

typedef struct {
	int		quotient;
	int		remainder;
} adivtab_t;

static adivtab_t	adivtab[32*32] = {
#include "adivtab.h"
};

extern byte	**skintable;
extern int		skinwidth;

void D_PolysetDrawColoredSpans8 (coloredspanpackage_t *pspanpackage);
void D_PolysetColoredCalcGradients (int skinwidth);
void D_DrawColoredSubdiv (void);
void D_DrawColoredNonSubdiv (void);
void D_PolysetRecursiveColoredTriangle (int *p1, int *p2, int *p3);
void D_PolysetColoredSetEdgeTable (void);
void D_RasterizeAliasColoredPolySmooth (void);
void D_PolysetColoredScanLeftEdge (int height);

#if	!id386

std::vector<coloredspanpackage_t> d_polyset_colored_spans;
/*
================
D_PolysetDrawColored
================
*/
void D_PolysetDrawColored (void)
{
    d_polyset_colored_spans.resize(vid.height + 2 + ((CACHE_SIZE - 1) / sizeof(coloredspanpackage_t)) + 1);
						// one extra because of cache line pretouching

	a_coloredspans = (coloredspanpackage_t *)
			(((size_t)&d_polyset_colored_spans[0] + CACHE_SIZE - 1) & ~(CACHE_SIZE - 1));

	if (r_affinetridesc.drawtype)
	{
		D_DrawColoredSubdiv ();
	}
	else
	{
		D_DrawColoredNonSubdiv ();
	}
}


/*
================
D_PolysetDrawFinalColoredVerts
================
*/
void D_PolysetDrawFinalColoredVerts (finalcoloredvert_t *fv, int numverts)
{
	int		i, z;
	short	*zbuf;

	if (r_holey)
	{
		for (i=0 ; i<numverts ; i++, fv++)
		{
		// valid triangle coordinates for filling can include the bottom and
		// right clip edges, due to the fill rule; these shouldn't be drawn
			if ((fv->v[0] < r_refdef.vrectright) &&
				(fv->v[1] < r_refdef.vrectbottom))
			{
				int pix = skintable[fv->v[3]>>16][fv->v[2]>>16];
				if (pix < 255)
				{
					z = fv->v[7]>>16;
					zbuf = zspantable[fv->v[1]] + fv->v[0];
					if (z >= *zbuf)
					{
						*zbuf = z;
						if (pix < 224)
						{
							auto pal = abasepal + pix*3;
							auto r = VID_CMAX - fv->v[4]; if (r < 0) r = 0;
							auto g = VID_CMAX - fv->v[5]; if (g < 0) g = 0;
							auto b = VID_CMAX - fv->v[6]; if (b < 0) b = 0;
							auto rcomp = (((unsigned)r * (*pal++)) >> 13) & 511;
							auto gcomp = (((unsigned)g * (*pal++)) >> 13) & 511;
							auto bcomp = (((unsigned)b * (*pal)) >> 13) & 511;
							if (rcomp > 255) rcomp = 255;
							if (gcomp > 255) gcomp = 255;
							if (bcomp > 255) bcomp = 255;
							pix = r_24to8tableptr[(rcomp << 16) | (gcomp << 8) | bcomp];
						}
						d_viewbuffer[d_scantable[fv->v[1]] + fv->v[0]] = pix;
					}
				}
			}
		}
		return;
	}

	for (i=0 ; i<numverts ; i++, fv++)
	{
	// valid triangle coordinates for filling can include the bottom and
	// right clip edges, due to the fill rule; these shouldn't be drawn
		if ((fv->v[0] < r_refdef.vrectright) &&
			(fv->v[1] < r_refdef.vrectbottom))
		{
			z = fv->v[7]>>16;
			zbuf = zspantable[fv->v[1]] + fv->v[0];
			if (z >= *zbuf)
			{
				int		pix;
				
				*zbuf = z;
				pix = skintable[fv->v[3]>>16][fv->v[2]>>16];
				if (pix < 224)
				{
					auto pal = abasepal + pix*3;
					auto r = VID_CMAX - fv->v[4]; if (r < 0) r = 0;
					auto g = VID_CMAX - fv->v[5]; if (g < 0) g = 0;
					auto b = VID_CMAX - fv->v[6]; if (b < 0) b = 0;
					auto rcomp = (((unsigned)r * (*pal++)) >> 13) & 511;
					auto gcomp = (((unsigned)g * (*pal++)) >> 13) & 511;
					auto bcomp = (((unsigned)b * (*pal)) >> 13) & 511;
					if (rcomp > 255) rcomp = 255;
					if (gcomp > 255) gcomp = 255;
					if (bcomp > 255) bcomp = 255;
					pix = r_24to8tableptr[(rcomp << 16) | (gcomp << 8) | bcomp];
				}
				d_viewbuffer[d_scantable[fv->v[1]] + fv->v[0]] = pix;
			}
		}
	}
}


/*
================
D_DrawColoredSubdiv
================
*/
void D_DrawColoredSubdiv (void)
{
	mtriangle_t		*ptri;
	finalcoloredvert_t	*pfv, *index0, *index1, *index2;
	int				i;
	int				lnumtriangles;

	pfv = r_affinetridesc.pfinalcoloredverts;
	ptri = r_affinetridesc.ptriangles;
	lnumtriangles = r_affinetridesc.numtriangles;

	for (i=0 ; i<lnumtriangles ; i++)
	{
		index0 = pfv + ptri[i].vertindex[0];
		index1 = pfv + ptri[i].vertindex[1];
		index2 = pfv + ptri[i].vertindex[2];

		if (((index0->v[1]-index1->v[1]) *
			 (index0->v[0]-index2->v[0]) -
			 (index0->v[0]-index1->v[0]) * 
			 (index0->v[1]-index2->v[1])) >= 0)
		{
			continue;
		}

		if (ptri[i].facesfront)
		{
			D_PolysetRecursiveColoredTriangle(index0->v, index1->v, index2->v);
		}
		else
		{
			int		s0, s1, s2;

			s0 = index0->v[2];
			s1 = index1->v[2];
			s2 = index2->v[2];

			if (index0->flags & ALIAS_ONSEAM)
				index0->v[2] += r_affinetridesc.seamfixupX16;
			if (index1->flags & ALIAS_ONSEAM)
				index1->v[2] += r_affinetridesc.seamfixupX16;
			if (index2->flags & ALIAS_ONSEAM)
				index2->v[2] += r_affinetridesc.seamfixupX16;

			D_PolysetRecursiveColoredTriangle(index0->v, index1->v, index2->v);

			index0->v[2] = s0;
			index1->v[2] = s1;
			index2->v[2] = s2;
		}
	}
}


/*
================
D_DrawColoredNonSubdiv
================
*/
void D_DrawColoredNonSubdiv (void)
{
	mtriangle_t		*ptri;
	finalcoloredvert_t	*pfv, *index0, *index1, *index2;
	int				i;
	int				lnumtriangles;

	pfv = r_affinetridesc.pfinalcoloredverts;
	ptri = r_affinetridesc.ptriangles;
	lnumtriangles = r_affinetridesc.numtriangles;

	for (i=0 ; i<lnumtriangles ; i++, ptri++)
	{
		index0 = pfv + ptri->vertindex[0];
		index1 = pfv + ptri->vertindex[1];
		index2 = pfv + ptri->vertindex[2];

		d_xdenom = (index0->v[1]-index1->v[1]) *
				(index0->v[0]-index2->v[0]) -
				(index0->v[0]-index1->v[0])*(index0->v[1]-index2->v[1]);

		if (d_xdenom >= 0)
		{
			continue;
		}

		r_p0colored[0] = index0->v[0];		// u
		r_p0colored[1] = index0->v[1];		// v
		r_p0colored[2] = index0->v[2];		// s
		r_p0colored[3] = index0->v[3];		// t
		r_p0colored[4] = index0->v[4];		// light r
		r_p0colored[5] = index0->v[5];		// light g
		r_p0colored[6] = index0->v[6];		// light b
		r_p0colored[7] = index0->v[7];		// iz

		r_p1colored[0] = index1->v[0];
		r_p1colored[1] = index1->v[1];
		r_p1colored[2] = index1->v[2];
		r_p1colored[3] = index1->v[3];
		r_p1colored[4] = index1->v[4];
		r_p1colored[5] = index1->v[5];
		r_p1colored[6] = index1->v[6];
		r_p1colored[7] = index1->v[7];

		r_p2colored[0] = index2->v[0];
		r_p2colored[1] = index2->v[1];
		r_p2colored[2] = index2->v[2];
		r_p2colored[3] = index2->v[3];
		r_p2colored[4] = index2->v[4];
		r_p2colored[5] = index2->v[5];
		r_p2colored[6] = index2->v[6];
		r_p2colored[7] = index2->v[7];

		if (!ptri->facesfront)
		{
			if (index0->flags & ALIAS_ONSEAM)
				r_p0colored[2] += r_affinetridesc.seamfixupX16;
			if (index1->flags & ALIAS_ONSEAM)
				r_p1colored[2] += r_affinetridesc.seamfixupX16;
			if (index2->flags & ALIAS_ONSEAM)
				r_p2colored[2] += r_affinetridesc.seamfixupX16;
		}

		D_PolysetColoredSetEdgeTable ();
		D_RasterizeAliasColoredPolySmooth ();
	}
}


/*
================
D_PolysetRecursiveColoredTriangle
================
*/
void D_PolysetRecursiveColoredTriangle (int *lp1, int *lp2, int *lp3)
{
	int		*temp;
	int		d;
	int		new_points[8];
	int		z;
	short	*zbuf;

	d = lp2[0] - lp1[0];
	if (d < -1 || d > 1)
		goto split;
	d = lp2[1] - lp1[1];
	if (d < -1 || d > 1)
		goto split;

	d = lp3[0] - lp2[0];
	if (d < -1 || d > 1)
		goto split2;
	d = lp3[1] - lp2[1];
	if (d < -1 || d > 1)
		goto split2;

	d = lp1[0] - lp3[0];
	if (d < -1 || d > 1)
		goto split3;
	d = lp1[1] - lp3[1];
	if (d < -1 || d > 1)
	{
split3:
		temp = lp1;
		lp1 = lp3;
		lp3 = lp2;
		lp2 = temp;

		goto split;
	}

	return;			// entire tri is filled

split2:
	temp = lp1;
	lp1 = lp2;
	lp2 = lp3;
	lp3 = temp;

split:
// split this edge
	new_points[0] = (lp1[0] + lp2[0]) >> 1;
	new_points[1] = (lp1[1] + lp2[1]) >> 1;
	new_points[2] = (lp1[2] + lp2[2]) >> 1;
	new_points[3] = (lp1[3] + lp2[3]) >> 1;
	new_points[4] = (lp1[4] + lp2[4]) >> 1;
	new_points[5] = (lp1[5] + lp2[5]) >> 1;
	new_points[6] = (lp1[6] + lp2[6]) >> 1;
	new_points[7] = (lp1[7] + lp2[7]) >> 1;

// draw the point if splitting a leading edge
	if (lp2[1] > lp1[1])
		goto nodraw;
	if ((lp2[1] == lp1[1]) && (lp2[0] < lp1[0]))
		goto nodraw;


	if (r_holey)
	{
		int pix = skintable[new_points[3]>>16][new_points[2]>>16];
		if (pix < 255)
		{
			z = new_points[7]>>16;
			zbuf = zspantable[new_points[1]] + new_points[0];
			if (z >= *zbuf)
			{
				*zbuf = z;
				if (pix < 224)
				{
					auto pal = abasepal + pix*3;
					auto r = VID_CMAX - new_points[4]; if (r < 0) r = 0;
					auto g = VID_CMAX - new_points[5]; if (g < 0) g = 0;
					auto b = VID_CMAX - new_points[6]; if (b < 0) b = 0;
					auto rcomp = (((unsigned)r * (*pal++)) >> 13) & 511;
					auto gcomp = (((unsigned)g * (*pal++)) >> 13) & 511;
					auto bcomp = (((unsigned)b * (*pal)) >> 13) & 511;
					if (rcomp > 255) rcomp = 255;
					if (gcomp > 255) gcomp = 255;
					if (bcomp > 255) bcomp = 255;
					pix = r_24to8tableptr[(rcomp << 16) | (gcomp << 8) | bcomp];
				}
				d_viewbuffer[d_scantable[new_points[1]] + new_points[0]] = pix;
			}
		}
	}
	else
	{
		z = new_points[7]>>16;
		zbuf = zspantable[new_points[1]] + new_points[0];
		if (z >= *zbuf)
		{
			int		pix;
			
			*zbuf = z;
			pix = skintable[new_points[3]>>16][new_points[2]>>16];
			if (pix < 224)
			{
				auto pal = abasepal + pix*3;
				auto r = VID_CMAX - new_points[4]; if (r < 0) r = 0;
				auto g = VID_CMAX - new_points[5]; if (g < 0) g = 0;
				auto b = VID_CMAX - new_points[6]; if (b < 0) b = 0;
				auto rcomp = (((unsigned)r * (*pal++)) >> 13) & 511;
				auto gcomp = (((unsigned)g * (*pal++)) >> 13) & 511;
				auto bcomp = (((unsigned)b * (*pal)) >> 13) & 511;
				if (rcomp > 255) rcomp = 255;
				if (gcomp > 255) gcomp = 255;
				if (bcomp > 255) bcomp = 255;
				pix = r_24to8tableptr[(rcomp << 16) | (gcomp << 8) | bcomp];
			}
			d_viewbuffer[d_scantable[new_points[1]] + new_points[0]] = pix;
		}
	}

nodraw:
// recursively continue
	D_PolysetRecursiveColoredTriangle (lp3, lp1, new_points);
	D_PolysetRecursiveColoredTriangle (lp3, new_points, lp2);
}

#endif	// !id386


#if	!id386

/*
===================
 D_PolysetColoredScanLeftEdge
====================
*/
void D_PolysetColoredScanLeftEdge (int height)
{

	do
	{
		d_pedgecoloredspanpackage->pdest = d_pdest;
		d_pedgecoloredspanpackage->pz = d_pz;
		d_pedgecoloredspanpackage->count = d_aspancount;
		d_pedgecoloredspanpackage->ptex = d_ptex;

		d_pedgecoloredspanpackage->sfrac = d_sfrac;
		d_pedgecoloredspanpackage->tfrac = d_tfrac;

	// FIXME: need to clamp l, s, t, at both ends?
		d_pedgecoloredspanpackage->lightr = d_lightr;
		d_pedgecoloredspanpackage->lightg = d_lightg;
		d_pedgecoloredspanpackage->lightb = d_lightb;
		d_pedgecoloredspanpackage->zi = d_zi;

		d_pedgecoloredspanpackage++;

		errorterm += erroradjustup;
		if (errorterm >= 0)
		{
			d_pdest += d_pdestextrastep;
			d_pz += d_pzextrastep;
			d_aspancount += d_countextrastep;
			d_ptex += d_ptexextrastep;
			d_sfrac += d_sfracextrastep;
			d_ptex += d_sfrac >> 16;

			d_sfrac &= 0xFFFF;
			d_tfrac += d_tfracextrastep;
			if (d_tfrac & 0x10000)
			{
				d_ptex += r_affinetridesc.skinwidth;
				d_tfrac &= 0xFFFF;
			}
			d_lightr += d_lightrextrastep;
			d_lightg += d_lightgextrastep;
			d_lightb += d_lightbextrastep;
			d_zi += d_ziextrastep;
			errorterm -= erroradjustdown;
		}
		else
		{
			d_pdest += d_pdestbasestep;
			d_pz += d_pzbasestep;
			d_aspancount += ubasestep;
			d_ptex += d_ptexbasestep;
			d_sfrac += d_sfracbasestep;
			d_ptex += d_sfrac >> 16;
			d_sfrac &= 0xFFFF;
			d_tfrac += d_tfracbasestep;
			if (d_tfrac & 0x10000)
			{
				d_ptex += r_affinetridesc.skinwidth;
				d_tfrac &= 0xFFFF;
			}
			d_lightr += d_lightrbasestep;
			d_lightg += d_lightgbasestep;
			d_lightb += d_lightbbasestep;
			d_zi += d_zibasestep;
		}
	} while (--height);
}

#endif	// !id386


/*
===================
D_PolysetColoredSetUpForLineScan
====================
*/
void D_PolysetColoredSetUpForLineScan(fixed8_t startvertu, fixed8_t startvertv,
		fixed8_t endvertu, fixed8_t endvertv)
{
	double		dm, dn;
	int			tm, tn;
	adivtab_t	*ptemp;

// TODO: implement x86 version

	errorterm = -1;

	tm = endvertu - startvertu;
	tn = endvertv - startvertv;

	if (((tm <= 16) && (tm >= -15)) &&
		((tn <= 16) && (tn >= -15)))
	{
		ptemp = &adivtab[((tm+15) << 5) + (tn+15)];
		ubasestep = ptemp->quotient;
		erroradjustup = ptemp->remainder;
		erroradjustdown = tn;
	}
	else
	{
		dm = (double)tm;
		dn = (double)tn;

		FloorDivMod (dm, dn, &ubasestep, &erroradjustup);

		erroradjustdown = dn;
	}
}


#if	!id386

/*
================
 D_PolysetColoredCalcGradients
================
*/
void D_PolysetColoredCalcGradients (int skinwidth)
{
	float	xstepdenominv, ystepdenominv, t0, t1;
	float	p01_minus_p21, p11_minus_p21, p00_minus_p20, p10_minus_p20;

	p00_minus_p20 = r_p0colored[0] - r_p2colored[0];
	p01_minus_p21 = r_p0colored[1] - r_p2colored[1];
	p10_minus_p20 = r_p1colored[0] - r_p2colored[0];
	p11_minus_p21 = r_p1colored[1] - r_p2colored[1];

	xstepdenominv = 1.0 / (float)d_xdenom;

	ystepdenominv = -xstepdenominv;

// ceil () for light so positive steps are exaggerated, negative steps
// diminished,  pushing us away from underflow toward overflow. Underflow is
// very visible, overflow is very unlikely, because of ambient lighting
	t0 = r_p0colored[4] - r_p2colored[4];
	t1 = r_p1colored[4] - r_p2colored[4];
	r_lrstepx = (int)
			ceil((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
	r_lrstepy = (int)
			ceil((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);

	t0 = r_p0colored[5] - r_p2colored[5];
	t1 = r_p1colored[5] - r_p2colored[5];
	r_lgstepx = (int)
			ceil((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
	r_lgstepy = (int)
			ceil((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);

	t0 = r_p0colored[6] - r_p2colored[6];
	t1 = r_p1colored[6] - r_p2colored[6];
	r_lbstepx = (int)
			ceil((t1 * p01_minus_p21 - t0 * p11_minus_p21) * xstepdenominv);
	r_lbstepy = (int)
			ceil((t1 * p00_minus_p20 - t0 * p10_minus_p20) * ystepdenominv);

	t0 = r_p0colored[2] - r_p2colored[2];
	t1 = r_p1colored[2] - r_p2colored[2];
	r_sstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
			xstepdenominv);
	r_sstepy = (int)((t1 * p00_minus_p20 - t0* p10_minus_p20) *
			ystepdenominv);

	t0 = r_p0colored[3] - r_p2colored[3];
	t1 = r_p1colored[3] - r_p2colored[3];
	r_tstepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
			xstepdenominv);
	r_tstepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
			ystepdenominv);

	t0 = r_p0colored[7] - r_p2colored[7];
	t1 = r_p1colored[7] - r_p2colored[7];
	r_zistepx = (int)((t1 * p01_minus_p21 - t0 * p11_minus_p21) *
			xstepdenominv);
	r_zistepy = (int)((t1 * p00_minus_p20 - t0 * p10_minus_p20) *
			ystepdenominv);

#if	id386
	a_sstepxfrac = r_sstepx << 16;
	a_tstepxfrac = r_tstepx << 16;
#else
	a_sstepxfrac = r_sstepx & 0xFFFF;
	a_tstepxfrac = r_tstepx & 0xFFFF;
#endif

	a_ststepxwhole = skinwidth * (r_tstepx >> 16) + (r_sstepx >> 16);
}

#endif	// !id386


#if	!id386

/*
================
D_PolysetDrawColoredSpans8
================
*/
void D_PolysetDrawColoredSpans8 (coloredspanpackage_t *pspanpackage)
{
	int		lcount;
	byte	*lpdest;
	byte	*lptex;
	int		lsfrac, ltfrac;
	int		llightr, llightg, llightb;
	long long		lzi;
	short	*lpz;

	if (r_holey)
	{
		do
		{
			lcount = d_aspancount - pspanpackage->count;

			errorterm += erroradjustup;
			if (errorterm >= 0)
			{
				d_aspancount += d_countextrastep;
				errorterm -= erroradjustdown;
			}
			else
			{
				d_aspancount += ubasestep;
			}

			if (lcount)
			{
				lpdest = (byte*)pspanpackage->pdest;
				lptex = pspanpackage->ptex;
				lpz = pspanpackage->pz;
				lsfrac = pspanpackage->sfrac;
				ltfrac = pspanpackage->tfrac;
				llightr = pspanpackage->lightr;
				llightg = pspanpackage->lightg;
				llightb = pspanpackage->lightb;
				lzi = pspanpackage->zi;

				do
				{
					auto pix = *lptex;
					if (pix < 255)
					{
						if ((lzi >> 16) >= *lpz)
						{
							if (pix < 224)
							{
								auto pal = abasepal + pix*3;
								auto r = VID_CMAX - llightr; if (r < 0) r = 0;
								auto g = VID_CMAX - llightg; if (g < 0) g = 0;
								auto b = VID_CMAX - llightb; if (b < 0) b = 0;
								auto rcomp = (((unsigned)r * (*pal++)) >> 13) & 511;
								auto gcomp = (((unsigned)g * (*pal++)) >> 13) & 511;
								auto bcomp = (((unsigned)b * (*pal)) >> 13) & 511;
								if (rcomp > 255) rcomp = 255;
								if (gcomp > 255) gcomp = 255;
								if (bcomp > 255) bcomp = 255;
								pix = r_24to8tableptr[(rcomp << 16) | (gcomp << 8) | bcomp];
							}
							*lpdest = pix;
		// gel mapping					*lpdest = gelmap[*lpdest];
							*lpz = lzi >> 16;
						}
					}
					lpdest++;
					lzi += r_zistepx;
					lpz++;
					llightr += r_lrstepx;
					llightg += r_lgstepx;
					llightb += r_lbstepx;
					lptex += a_ststepxwhole;
					lsfrac += a_sstepxfrac;
					lptex += lsfrac >> 16;
					lsfrac &= 0xFFFF;
					ltfrac += a_tstepxfrac;
					if (ltfrac & 0x10000)
					{
						lptex += r_affinetridesc.skinwidth;
						ltfrac &= 0xFFFF;
					}
				} while (--lcount);
			}

			pspanpackage++;
		} while (pspanpackage->count != -999999);
		return;
	}

	do
	{
		lcount = d_aspancount - pspanpackage->count;

		errorterm += erroradjustup;
		if (errorterm >= 0)
		{
			d_aspancount += d_countextrastep;
			errorterm -= erroradjustdown;
		}
		else
		{
			d_aspancount += ubasestep;
		}

		if (lcount)
		{
			lpdest = (byte*)pspanpackage->pdest;
			lptex = pspanpackage->ptex;
			lpz = pspanpackage->pz;
			lsfrac = pspanpackage->sfrac;
			ltfrac = pspanpackage->tfrac;
			llightr = pspanpackage->lightr;
			llightg = pspanpackage->lightg;
			llightb = pspanpackage->lightb;
			lzi = pspanpackage->zi;

			do
			{
				if ((lzi >> 16) >= *lpz)
				{
					auto pix = *lptex;
					if (pix < 224)
					{
						auto pal = abasepal + pix*3;
						auto r = VID_CMAX - llightr; if (r < 0) r = 0;
						auto g = VID_CMAX - llightg; if (g < 0) g = 0;
						auto b = VID_CMAX - llightb; if (b < 0) b = 0;
						auto rcomp = (((unsigned)r * (*pal++)) >> 13) & 511;
						auto gcomp = (((unsigned)g * (*pal++)) >> 13) & 511;
						auto bcomp = (((unsigned)b * (*pal)) >> 13) & 511;
						if (rcomp > 255) rcomp = 255;
						if (gcomp > 255) gcomp = 255;
						if (bcomp > 255) bcomp = 255;
						pix = r_24to8tableptr[(rcomp << 16) | (gcomp << 8) | bcomp];
					}
					*lpdest = pix;
// gel mapping					*lpdest = gelmap[*lpdest];
					*lpz = lzi >> 16;
				}
				lpdest++;
				lzi += r_zistepx;
				lpz++;
				llightr += r_lrstepx;
				llightg += r_lgstepx;
				llightb += r_lbstepx;
				lptex += a_ststepxwhole;
				lsfrac += a_sstepxfrac;
				lptex += lsfrac >> 16;
				lsfrac &= 0xFFFF;
				ltfrac += a_tstepxfrac;
				if (ltfrac & 0x10000)
				{
					lptex += r_affinetridesc.skinwidth;
					ltfrac &= 0xFFFF;
				}
			} while (--lcount);
		}

		pspanpackage++;
	} while (pspanpackage->count != -999999);
}
#endif	// !id386

/*
================
 D_RasterizeAliasColoredPolySmooth
================
*/
void D_RasterizeAliasColoredPolySmooth (void)
{
	int				initialleftheight, initialrightheight;
	int				*plefttop, *prighttop, *pleftbottom, *prightbottom;
	int				working_lrstepx, working_lgstepx, working_lbstepx, originalcount;

	plefttop = pcolorededgetable->pleftedgevert0;
	prighttop = pcolorededgetable->prightedgevert0;

	pleftbottom = pcolorededgetable->pleftedgevert1;
	prightbottom = pcolorededgetable->prightedgevert1;

	initialleftheight = pleftbottom[1] - plefttop[1];
	initialrightheight = prightbottom[1] - prighttop[1];

//
// set the s, t, and light gradients, which are consistent across the triangle
// because being a triangle, things are affine
//
	D_PolysetColoredCalcGradients (r_affinetridesc.skinwidth);

//
// rasterize the polygon
//

//
// scan out the top (and possibly only) part of the left edge
//
	d_pedgecoloredspanpackage = a_coloredspans;

	ystart = plefttop[1];
	d_aspancount = plefttop[0] - prighttop[0];

	d_ptex = (byte *)r_affinetridesc.pskin + (plefttop[2] >> 16) +
			(plefttop[3] >> 16) * r_affinetridesc.skinwidth;
#if	id386
	d_sfrac = (plefttop[2] & 0xFFFF) << 16;
	d_tfrac = (plefttop[3] & 0xFFFF) << 16;
#else
	d_sfrac = plefttop[2] & 0xFFFF;
	d_tfrac = plefttop[3] & 0xFFFF;
#endif
	d_lightr = plefttop[4];
	d_lightg = plefttop[5];
	d_lightb = plefttop[6];
	d_zi = plefttop[7];

	d_pdest = (byte *)d_viewbuffer +
			ystart * screenwidth + plefttop[0];
	d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

	if (initialleftheight == 1)
	{
		d_pedgecoloredspanpackage->pdest = d_pdest;
		d_pedgecoloredspanpackage->pz = d_pz;
		d_pedgecoloredspanpackage->count = d_aspancount;
		d_pedgecoloredspanpackage->ptex = d_ptex;

		d_pedgecoloredspanpackage->sfrac = d_sfrac;
		d_pedgecoloredspanpackage->tfrac = d_tfrac;

	// FIXME: need to clamp lr, lg, lb, s, t, at both ends?
		d_pedgecoloredspanpackage->lightr = d_lightr;
		d_pedgecoloredspanpackage->lightg = d_lightg;
		d_pedgecoloredspanpackage->lightb = d_lightb;
		d_pedgecoloredspanpackage->zi = d_zi;

		d_pedgecoloredspanpackage++;
	}
	else
	{
		D_PolysetColoredSetUpForLineScan(plefttop[0], plefttop[1],
							  pleftbottom[0], pleftbottom[1]);

	#if	id386
		d_pzbasestep = (d_zwidth + ubasestep) << 1;
		d_pzextrastep = d_pzbasestep + 2;
	#else
		d_pzbasestep = d_zwidth + ubasestep;
		d_pzextrastep = d_pzbasestep + 1;
	#endif

		d_pdestbasestep = screenwidth + ubasestep;
		d_pdestextrastep = d_pdestbasestep + 1;

	// TODO: can reuse partial expressions here

	// for negative steps in x along left edge, bias toward overflow rather than
	// underflow (sort of turning the floor () we did in the gradient calcs into
	// ceil (), but plus a little bit)
		if (ubasestep < 0)
		{
			working_lrstepx = r_lrstepx - 1;
			working_lgstepx = r_lgstepx - 1;
			working_lbstepx = r_lbstepx - 1;
		}
		else
		{
			working_lrstepx = r_lrstepx;
			working_lgstepx = r_lgstepx;
			working_lbstepx = r_lbstepx;
		}

		d_countextrastep = ubasestep + 1;
		d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
				((r_tstepy + r_tstepx * ubasestep) >> 16) *
				r_affinetridesc.skinwidth;
	#if	id386
		d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) << 16;
		d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) << 16;
	#else
		d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
		d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
	#endif
		d_lightrbasestep = r_lrstepy + working_lrstepx * ubasestep;
		d_lightgbasestep = r_lgstepy + working_lgstepx * ubasestep;
		d_lightbbasestep = r_lbstepy + working_lbstepx * ubasestep;
		d_zibasestep = r_zistepy + r_zistepx * ubasestep;

		d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
				((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
				r_affinetridesc.skinwidth;
	#if	id386
		d_sfracextrastep = (r_sstepy + r_sstepx*d_countextrastep) << 16;
		d_tfracextrastep = (r_tstepy + r_tstepx*d_countextrastep) << 16;
	#else
		d_sfracextrastep = (r_sstepy + r_sstepx*d_countextrastep) & 0xFFFF;
		d_tfracextrastep = (r_tstepy + r_tstepx*d_countextrastep) & 0xFFFF;
	#endif
		d_lightrextrastep = d_lightrbasestep + working_lrstepx;
		d_lightgextrastep = d_lightgbasestep + working_lgstepx;
		d_lightbextrastep = d_lightbbasestep + working_lbstepx;
		d_ziextrastep = d_zibasestep + r_zistepx;

		D_PolysetColoredScanLeftEdge (initialleftheight);
	}

//
// scan out the bottom part of the left edge, if it exists
//
	if (pcolorededgetable->numleftedges == 2)
	{
		int		height;

		plefttop = pleftbottom;
		pleftbottom = pcolorededgetable->pleftedgevert2;

		height = pleftbottom[1] - plefttop[1];

// TODO: make this a function; modularize this function in general

		ystart = plefttop[1];
		d_aspancount = plefttop[0] - prighttop[0];
		d_ptex = (byte *)r_affinetridesc.pskin + (plefttop[2] >> 16) +
				(plefttop[3] >> 16) * r_affinetridesc.skinwidth;
		d_sfrac = 0;
		d_tfrac = 0;
		d_lightr = plefttop[4];
		d_lightg = plefttop[5];
		d_lightb = plefttop[6];
		d_zi = plefttop[7];

		d_pdest = (byte *)d_viewbuffer + ystart * screenwidth + plefttop[0];
		d_pz = d_pzbuffer + ystart * d_zwidth + plefttop[0];

		if (height == 1)
		{
			d_pedgecoloredspanpackage->pdest = d_pdest;
			d_pedgecoloredspanpackage->pz = d_pz;
			d_pedgecoloredspanpackage->count = d_aspancount;
			d_pedgecoloredspanpackage->ptex = d_ptex;

			d_pedgecoloredspanpackage->sfrac = d_sfrac;
			d_pedgecoloredspanpackage->tfrac = d_tfrac;

		// FIXME: need to clamp l, s, t, at both ends?
			d_pedgecoloredspanpackage->lightr = d_lightr;
			d_pedgecoloredspanpackage->lightg = d_lightg;
			d_pedgecoloredspanpackage->lightb = d_lightb;
			d_pedgecoloredspanpackage->zi = d_zi;

			d_pedgecoloredspanpackage++;
		}
		else
		{
			D_PolysetColoredSetUpForLineScan(plefttop[0], plefttop[1],
								  pleftbottom[0], pleftbottom[1]);

			d_pdestbasestep = screenwidth + ubasestep;
			d_pdestextrastep = d_pdestbasestep + 1;

	#if	id386
			d_pzbasestep = (d_zwidth + ubasestep) << 1;
			d_pzextrastep = d_pzbasestep + 2;
	#else
			d_pzbasestep = d_zwidth + ubasestep;
			d_pzextrastep = d_pzbasestep + 1;
	#endif

			if (ubasestep < 0)
			{
				working_lrstepx = r_lrstepx - 1;
				working_lgstepx = r_lgstepx - 1;
				working_lbstepx = r_lbstepx - 1;
			}
			else
			{
				working_lrstepx = r_lrstepx;
				working_lgstepx = r_lgstepx;
				working_lbstepx = r_lbstepx;
			}

			d_countextrastep = ubasestep + 1;
			d_ptexbasestep = ((r_sstepy + r_sstepx * ubasestep) >> 16) +
					((r_tstepy + r_tstepx * ubasestep) >> 16) *
					r_affinetridesc.skinwidth;
	#if	id386
			d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) << 16;
			d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) << 16;
	#else
			d_sfracbasestep = (r_sstepy + r_sstepx * ubasestep) & 0xFFFF;
			d_tfracbasestep = (r_tstepy + r_tstepx * ubasestep) & 0xFFFF;
	#endif
			d_lightrbasestep = r_lrstepy + working_lrstepx * ubasestep;
			d_lightgbasestep = r_lgstepy + working_lgstepx * ubasestep;
			d_lightbbasestep = r_lbstepy + working_lbstepx * ubasestep;
			d_zibasestep = r_zistepy + r_zistepx * ubasestep;

			d_ptexextrastep = ((r_sstepy + r_sstepx * d_countextrastep) >> 16) +
					((r_tstepy + r_tstepx * d_countextrastep) >> 16) *
					r_affinetridesc.skinwidth;
	#if	id386
			d_sfracextrastep = ((r_sstepy+r_sstepx*d_countextrastep) & 0xFFFF)<<16;
			d_tfracextrastep = ((r_tstepy+r_tstepx*d_countextrastep) & 0xFFFF)<<16;
	#else
			d_sfracextrastep = (r_sstepy+r_sstepx*d_countextrastep) & 0xFFFF;
			d_tfracextrastep = (r_tstepy+r_tstepx*d_countextrastep) & 0xFFFF;
	#endif
			d_lightrextrastep = d_lightrbasestep + working_lrstepx;
			d_lightgextrastep = d_lightgbasestep + working_lgstepx;
			d_lightbextrastep = d_lightbbasestep + working_lbstepx;
			d_ziextrastep = d_zibasestep + r_zistepx;

			D_PolysetColoredScanLeftEdge (height);
		}
	}

// scan out the top (and possibly only) part of the right edge, updating the
// count field
	d_pedgecoloredspanpackage = a_coloredspans;

	D_PolysetColoredSetUpForLineScan(prighttop[0], prighttop[1],
						  prightbottom[0], prightbottom[1]);
	d_aspancount = 0;
	d_countextrastep = ubasestep + 1;
	originalcount = a_coloredspans[initialrightheight].count;
	a_coloredspans[initialrightheight].count = -999999; // mark end of the spanpackages
	D_PolysetDrawColoredSpans8 (a_coloredspans);

// scan out the bottom part of the right edge, if it exists
	if (pcolorededgetable->numrightedges == 2)
	{
		int				height;
		coloredspanpackage_t	*pstart;

		pstart = a_coloredspans + initialrightheight;
		pstart->count = originalcount;

		d_aspancount = prightbottom[0] - prighttop[0];

		prighttop = prightbottom;
		prightbottom = pcolorededgetable->prightedgevert2;

		height = prightbottom[1] - prighttop[1];

		D_PolysetColoredSetUpForLineScan(prighttop[0], prighttop[1],
							  prightbottom[0], prightbottom[1]);

		d_countextrastep = ubasestep + 1;
		a_coloredspans[initialrightheight + height].count = -999999;
											// mark end of the spanpackages
		D_PolysetDrawColoredSpans8 (pstart);
	}
}


/*
================
 D_PolysetColoredSetEdgeTable
================
*/
void D_PolysetColoredSetEdgeTable (void)
{
	int			edgetableindex;

	edgetableindex = 0;	// assume the vertices are already in
						//  top to bottom order

//
// determine which edges are right & left, and the order in which
// to rasterize them
//
	if (r_p0colored[1] >= r_p1colored[1])
	{
		if (r_p0colored[1] == r_p1colored[1])
		{
			if (r_p0colored[1] < r_p2colored[1])
				pcolorededgetable = &colorededgetables[2];
			else
				pcolorededgetable = &colorededgetables[5];

			return;
		}
		else
		{
			edgetableindex = 1;
		}
	}

	if (r_p0colored[1] == r_p2colored[1])
	{
		if (edgetableindex)
			pcolorededgetable = &colorededgetables[8];
		else
			pcolorededgetable = &colorededgetables[9];

		return;
	}
	else if (r_p1colored[1] == r_p2colored[1])
	{
		if (edgetableindex)
			pcolorededgetable = &colorededgetables[10];
		else
			pcolorededgetable = &colorededgetables[11];

		return;
	}

	if (r_p0colored[1] > r_p2colored[1])
		edgetableindex += 2;

	if (r_p1colored[1] > r_p2colored[1])
		edgetableindex += 4;

	pcolorededgetable = &colorededgetables[edgetableindex];
}

