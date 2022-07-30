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
// r_sky.c

#include "quakedef.h"
#include "r_local.h"
#include "d_local.h"


int		iskyspeed = 8;
int		iskyspeed2 = 2;
float	skyspeed, skyspeed2;

float		skytime;

byte		*r_skysource;
unsigned	*r_skysourceRGBA;
int			r_skyRGBAwidth;
int			r_skyRGBAheight;

int r_skymade;
int r_skydirect;		// not used?

qboolean r_skyinitialized;
qboolean r_skyRGBAinitialized;
qboolean r_skyboxinitialized;
std::string r_skyboxprefix;

// TODO: clean up these routines

byte	bottomsky[128*131];
byte	bottommask[128*131];
byte	newsky[128*256];	// newsky and topsky both pack in here, 128 bytes
							//  of newsky on the left of each scan, 128 bytes
							//  of topsky on the right, because the low-level
							//  drawers need 256-byte scan widths

std::vector<unsigned> newskyRGBA;

msurface_t		*r_skyfaces;
mplane_t		r_skyplanes[6];
mtexinfo_t		r_skytexinfo[6];
mvertex_t		*r_skyverts;
medge_t			*r_skyedges;
int				*r_skysurfedges;
float			r_skyrotate;
vec3_t			r_skyaxis;
int				r_skyframe;

int skybox_planes[12] = {2,-128, 0,-128, 2,128, 1,128, 0,128, 1,-128};

int box_surfedges[24] = { 1,2,3,4,  -1,5,6,7,  8,9,-6,10,  -2,-7,-9,11,
						  12,-3,-11,-8,  -12,-10,-5,-4};
int box_edges[24] = { 1,2, 2,3, 3,4, 4,1, 1,5, 5,6, 6,2, 7,8, 8,6, 5,7, 8,3, 7,4};

int	box_faces[6] = {0,0,2,2,2,0};

vec3_t	box_vecs[6][2] = {
		{	{0,-1,0}, {-1,0,0} },
		{ {0,1,0}, {0,0,-1} },
		{	{0,-1,0}, {1,0,0} },
		{ {1,0,0}, {0,0,-1} },
		{ {0,-1,0}, {0,0,-1} },
		{ {-1,0,0}, {0,0,-1} }
};

float	box_verts[8][3] = {
		{-1,-1,-1},
		{-1,1,-1},
		{1,1,-1},
		{1,-1,-1},
		{-1,-1,1},
		{-1,1,1},
		{1,-1,1},
		{1,1,1}
};



/*
=============
R_InitSky

A sky texture is 256*128, with the right side being a masked overlay
==============
*/
void R_InitSky (texture_t *mt)
{
	int			i, j;
	byte		*src;

    r_skyinitialized = true;
    
    if (mt->width != 256 || mt->height != 128)
    {
        memset(newsky, 0, 128*256);
        memset(bottommask, 0, 128*131);
        memset(bottomsky, 0, 128*131);
		r_skysource = newsky;
        Con_Printf ("R_InitSky: %ix%i instead of 256x128\n", mt->width, mt->height);
        return;
    }

    src = (byte *)mt + mt->offsets[0];

	for (i=0 ; i<128 ; i++)
	{
		for (j=0 ; j<128 ; j++)
		{
			newsky[(i*256) + j + 128] = src[i*256 + j + 128];
		}
	}

	for (i=0 ; i<128 ; i++)
	{
		for (j=0 ; j<131 ; j++)
		{
			if (src[i*256 + (j & 0x7F)])
			{
				bottomsky[(i*131) + j] = src[i*256 + (j & 0x7F)];
				bottommask[(i*131) + j] = 0;
			}
			else
			{
				bottomsky[(i*131) + j] = 0;
				bottommask[(i*131) + j] = 0xff;
			}
		}
	}
	
	r_skysource = newsky;
}


void R_InitSkyRGBA (miptex_t *mt)
{
	int			i, j;
	unsigned	*src;

	r_skyRGBAinitialized = true;

	auto width = mt->width;
	auto height = mt->height;
	auto halfWidth = width / 2;

	newskyRGBA.resize(width * height);
	memcpy(newskyRGBA.data(), (byte *)mt + mt->offsets[0], newskyRGBA.size() * sizeof(unsigned));

	r_skysourceRGBA = newskyRGBA.data();
	r_skyRGBAwidth = halfWidth;
	r_skyRGBAheight = height;
}


/*
================
R_InitSkyBox

================
*/
void R_InitSkyBox (void)
{
	int		i;
	model_t* loadmodel = cl.worldmodel;

    // Using the extra space previously allocated in the various Mod_Load****() calls:
    r_skyfaces = loadmodel->surfaces + loadmodel->numsurfaces;
	loadmodel->numsurfaces += 6;
	r_skyverts = loadmodel->vertexes + loadmodel->numvertexes;
	loadmodel->numvertexes += 8;
	r_skyedges = loadmodel->edges + loadmodel->numedges;
	loadmodel->numedges += 12;
	r_skysurfedges = loadmodel->surfedges + loadmodel->numsurfedges;
	loadmodel->numsurfedges += 24;

	for (i=0 ; i<6 ; i++)
	{
		r_skyplanes[i].normal[skybox_planes[i*2]] = 1;
		r_skyplanes[i].dist = skybox_planes[i*2+1];

		VectorCopy (box_vecs[i][0], r_skytexinfo[i].vecs[0]);
		VectorCopy (box_vecs[i][1], r_skytexinfo[i].vecs[1]);

		r_skyfaces[i] = { };
		r_skyfaces[i].plane = &r_skyplanes[i];
		r_skyfaces[i].numedges = 4;
		r_skyfaces[i].flags = box_faces[i] | SURF_DRAWSKYBOX;
		r_skyfaces[i].firstedge = loadmodel->numsurfedges-24+i*4;
		r_skyfaces[i].texinfo = &r_skytexinfo[i];
	}

	for (i=0 ; i<24 ; i++)
		if (box_surfedges[i] > 0)
			r_skysurfedges[i] = loadmodel->numedges-13 + box_surfedges[i];
		else
			r_skysurfedges[i] = - (loadmodel->numedges-13 + -box_surfedges[i]);

	for(i=0 ; i<12 ; i++)
	{
		r_skyedges[i].v[0] = loadmodel->numvertexes-9+box_edges[i*2+0];
		r_skyedges[i].v[1] = loadmodel->numvertexes-9+box_edges[i*2+1];
		r_skyedges[i].cachededgeoffset = 0;
	}
}


/*
=================
R_MakeSky
=================
*/
void R_MakeSky (void)
{
	int			x, y;
	int			ofs, baseofs;
	int			xshift, yshift;
	unsigned	*pnewsky;
	static int	xlast = -1, ylast = -1;

	xshift = skytime*skyspeed;
	yshift = skytime*skyspeed;

	if ((xshift == xlast) && (yshift == ylast))
		return;

	xlast = xshift;
	ylast = yshift;
	
	pnewsky = (unsigned *)&newsky[0];

	for (y=0 ; y<SKYSIZE ; y++)
	{
		baseofs = ((y+yshift) & SKYMASK) * 131;

// FIXME: clean this up
#if UNALIGNED_OK

		for (x=0 ; x<SKYSIZE ; x += 4)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

		// PORT: unaligned dword access to bottommask and bottomsky

			*pnewsky = (*(pnewsky + (128 / sizeof (unsigned))) &
						*(unsigned *)&bottommask[ofs]) |
						*(unsigned *)&bottomsky[ofs];
			pnewsky++;
		}

#else

		for (x=0 ; x<SKYSIZE ; x++)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

			*(byte *)pnewsky = (*((byte *)pnewsky + 128) &
						*(byte *)&bottommask[ofs]) |
						*(byte *)&bottomsky[ofs];
			pnewsky = (unsigned *)((byte *)pnewsky + 1);
		}

#endif

		pnewsky += 128 / sizeof (unsigned);
	}

	r_skymade = 1;
}


/*
=================
R_GenSkyTile
=================
*/
void R_GenSkyTile (void *pdest)
{
	int			x, y;
	int			ofs, baseofs;
	int			xshift, yshift;
	unsigned	*pnewsky;
	unsigned	*pd;

	xshift = skytime*skyspeed;
	yshift = skytime*skyspeed;

	pnewsky = (unsigned *)&newsky[0];
	pd = (unsigned *)pdest;

	for (y=0 ; y<SKYSIZE ; y++)
	{
		baseofs = ((y+yshift) & SKYMASK) * 131;

// FIXME: clean this up
#if UNALIGNED_OK

		for (x=0 ; x<SKYSIZE ; x += 4)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

		// PORT: unaligned dword access to bottommask and bottomsky

			*pd = (*(pnewsky + (128 / sizeof (unsigned))) &
				   *(unsigned *)&bottommask[ofs]) |
				   *(unsigned *)&bottomsky[ofs];
			pnewsky++;
			pd++;
		}

#else

		for (x=0 ; x<SKYSIZE ; x++)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

			*(byte *)pd = (*((byte *)pnewsky + 128) &
						*(byte *)&bottommask[ofs]) |
						*(byte *)&bottomsky[ofs];
			pnewsky = (unsigned *)((byte *)pnewsky + 1);
			pd = (unsigned *)((byte *)pd + 1);
		}

#endif

		pnewsky += 128 / sizeof (unsigned);
	}
}


/*
=================
R_GenSkyTile16
=================
*/
void R_GenSkyTile16 (void *pdest)
{
	int				x, y;
	int				ofs, baseofs;
	int				xshift, yshift;
	byte			*pnewsky;
	unsigned short	*pd;

	xshift = skytime * skyspeed;
	yshift = skytime * skyspeed;

	pnewsky = (byte *)&newsky[0];
	pd = (unsigned short *)pdest;

	for (y=0 ; y<SKYSIZE ; y++)
	{
		baseofs = ((y+yshift) & SKYMASK) * 131;

// FIXME: clean this up
// FIXME: do faster unaligned version?
		for (x=0 ; x<SKYSIZE ; x++)
		{
			ofs = baseofs + ((x+xshift) & SKYMASK);

			*pd = d_8to16table[(*(pnewsky + 128) &
					*(byte *)&bottommask[ofs]) |
					*(byte *)&bottomsky[ofs]];
			pnewsky++;
			pd++;
		}

		pnewsky += TILE_SIZE;
	}
}


/*
=============
R_SetSkyFrame
==============
*/
void R_SetSkyFrame (void)
{
	int		g, s1, s2;
	float	temp;

	skyspeed = iskyspeed;
	skyspeed2 = iskyspeed2;

	g = GreatestCommonDivisor (iskyspeed, iskyspeed2);
	s1 = iskyspeed / g;
	s2 = iskyspeed2 / g;
	temp = SKYSIZE * s1 * s2;

	skytime = cl.time - ((int)(cl.time / temp) * temp);
	

	r_skymade = 0;
}


qboolean R_LoadSkyImage(std::string& path, const std::string& prefix, texture_t*& texture)
{
	static byte* pic = nullptr;
	static int piclen = 0;
	int width;
	int height;

	if (texture != nullptr)
	{
		delete[] texture;
		texture = nullptr;
	}
	if (r_load_as_rgba)
	{
		pic = nullptr;
		piclen = 0;
		R_LoadTGA(path.c_str(), sizeof(texture_t), true, 0, true, &pic, &piclen, &width, &height);
	}
	else
	{
		R_LoadTGA(path.c_str(), 0, false, 0, true, &pic, &piclen, &width, &height);
	}
	if (pic != nullptr)
	{
		Draw_BeginDisc ();
        auto pixels = width * height;
        if (r_load_as_rgba)
        {
			texture = (texture_t*)pic;
        }
		else
		{
			texture = (texture_t*)new byte[sizeof(texture_t) + pixels];
		}
		memset(texture, 0, sizeof(texture_t));
        strcpy(texture->name, prefix.c_str());
		texture->width = width;
		texture->height = height;
		texture->offsets[0] = sizeof(texture_t);
		auto start = Sys_FloatTime ();
        static std::vector<float> halfSquaredDistances;
        if (halfSquaredDistances.empty())
        {
            auto first = host_basepal.data();
            for (auto i = 0; i < 256; i++)
            {
                auto r1 = *first++;
                auto g1 = *first++;
                auto b1 = *first++;
                auto second = host_basepal.data();
                auto nearestDistanceSquared = INT_MAX;
                for (auto j = 0; j < 256; j++)
                {
                    auto r2 = *second++;
                    auto g2 = *second++;
                    auto b2 = *second++;
                    auto dr = r2 - r1;
                    auto dg = g2 - g1;
                    auto db = b2 - b1;
                    if (dr == 0 && dg == 0 && db == 0)
                    {
                        continue;
                    }
                    auto distanceSquared = dr * dr + dg * dg + db * db;
                    if (nearestDistanceSquared > distanceSquared)
                    {
                        nearestDistanceSquared = distanceSquared;
                    }
                }
                auto halfDistance = sqrt((float)nearestDistanceSquared) / 2;
                halfSquaredDistances.push_back(halfDistance * halfDistance);
            }
        }
		auto source = pic;
		if (r_load_as_rgba)
		{
			source += sizeof(texture_t) + pixels;
		}
		auto target = (byte*)texture + sizeof(texture_t);
        auto rprev = -1;
        auto gprev = -1;
        auto bprev = -1;
        auto iprev = -1;
		for (auto count = width * height; count > 0; count--)
		{
			auto r1 = *source++;
			auto g1 = *source++;
			auto b1 = *source++;
			source++;
            if (rprev == r1 && gprev == g1 && bprev == b1)
            {
                *target++ = iprev;
                continue;
            }
            if (iprev >= 0)
            {
                auto previous = host_basepal.data() + iprev * 3;
                auto drprev = *previous++ - r1;
                auto dgprev = *previous++ - g1;
                auto dbprev = *previous - b1;
                auto squaredDistanceToPrevious = drprev * drprev + dgprev * dgprev + dbprev * dbprev;
                if (squaredDistanceToPrevious < halfSquaredDistances[iprev])
                {
                    *target++ = iprev;
                    continue;
                }
            }
            auto nearestDistanceSquared = INT_MAX;
            auto nearestIndex = 0;
            auto entry = host_basepal.data();
            for (auto i = 0; i < 256; i++)
            {
                auto r2 = *entry++;
                auto g2 = *entry++;
                auto b2 = *entry++;
                auto dr = r2 - r1;
                auto dg = g2 - g1;
                auto db = b2 - b1;
                auto distanceSquared = dr * dr + dg * dg + db * db;
                if (nearestDistanceSquared > distanceSquared)
                {
                    nearestDistanceSquared = distanceSquared;
                    nearestIndex = i;
                }
            }
            *target++ = nearestIndex;
            rprev = r1;
            gprev = g1;
            bprev = b1;
            iprev = nearestIndex;
		}
		auto stop = Sys_FloatTime();
		Sys_Printf("%s: %.3f seconds\n", path.c_str(), stop - start);
		Draw_EndDisc ();
		return true;
	}
	return false;
}

/*
============
R_SetSkyBox
============
*/
const char	*suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
int	r_skysideimage[6] = {5, 2, 4, 1, 0, 3};

qboolean R_SetSkyBox (float rotate, const vec3_t axis)
{
	r_skyrotate = rotate;
	VectorCopy (axis, r_skyaxis);

	for (auto i=0 ; i<6 ; i++)
	{
		std::string prefix = suf[r_skysideimage[i]];
		std::string path = "gfx/env/" + r_skyboxprefix + prefix + ".tga";
		if (R_LoadSkyImage(path, prefix, r_skytexinfo[i].texture))
		{
			r_skyfaces[i].texturemins[0] = -(int)r_skytexinfo[i].texture->width / 2;
			r_skyfaces[i].texturemins[1] = -(int)r_skytexinfo[i].texture->height / 2;
			r_skyfaces[i].extents[0] = r_skytexinfo[i].texture->width;
			r_skyfaces[i].extents[1] = r_skytexinfo[i].texture->height;
		}
		else
		{
			return false;
		}
	}
	return true;
}

/*
================
R_EmitSkyBox
================
*/
void R_EmitSkyBox (void)
{
	int		i, j;
	int		oldkey;

	if (insubmodel)
		return;		// submodels should never have skies
	if (r_skyframe == r_framecount)
		return;		// already set this frame

	r_skyframe = r_framecount;

	// set the eight fake vertexes
	for (i=0 ; i<8 ; i++)
		for (j=0 ; j<3 ; j++)
			r_skyverts[i].position[j] = r_origin[j] + box_verts[i][j]*128;

	// set the six fake planes
	for (i=0 ; i<6 ; i++)
        if (r_skyfaces[i].texinfo->texture != nullptr)
            if (skybox_planes[i*2+1] > 0)
                r_skyplanes[i].dist = r_origin[skybox_planes[i*2]]+r_skyfaces[i].texinfo->texture->width / 2;
            else
                r_skyplanes[i].dist = r_origin[skybox_planes[i*2]]-r_skyfaces[i].texinfo->texture->height / 2;

	// fix texture offseets
	for (i=0 ; i<6 ; i++)
	{
		r_skytexinfo[i].vecs[0][3] = -DotProduct (r_origin, r_skytexinfo[i].vecs[0]);
		r_skytexinfo[i].vecs[1][3] = -DotProduct (r_origin, r_skytexinfo[i].vecs[1]);
	}

	// emit the six faces
	oldkey = r_currentkey;
	r_currentkey = 0x7ffffff0;
	for (i=0 ; i<6 ; i++)
	{
        auto face = r_skyfaces + i;
        if (face->texinfo->texture == nullptr)
        {
            continue;
        }
		R_RenderFace (face, 15);
	}
	r_currentkey = oldkey;		// bsp sorting order
}


