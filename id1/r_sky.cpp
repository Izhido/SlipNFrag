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

int r_skymade;
int r_skydirect;		// not used?

qboolean r_skyinitialized;
qboolean r_skyboxinitialized;
std::string r_skyboxprefix;

// TODO: clean up these routines

byte	bottomsky[128*131];
byte	bottommask[128*131];
byte	newsky[128*256];	// newsky and topsky both pack in here, 128 bytes
							//  of newsky on the left of each scan, 128 bytes
							//  of topsky on the right, because the low-level
							//  drawers need 256-byte scan widths

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
	r_skyinitialized = true;
}


/*
================
R_InitSkyBox

================
*/
void R_InitSkyBox (void)
{
	int		i;
	extern model_t *loadmodel;

    // Using the extra space previously allocated in the various Mod_Load****() calls:
    r_skyfaces = loadmodel->surfaces + loadmodel->numsurfaces;
	loadmodel->numsurfaces += 6;
	r_skyverts = loadmodel->vertexes + loadmodel->numvertexes;
	loadmodel->numvertexes += 8;
	r_skyedges = loadmodel->edges + loadmodel->numedges;
	loadmodel->numedges += 12;
	r_skysurfedges = loadmodel->surfedges + loadmodel->numsurfedges;
	loadmodel->numsurfedges += 24;

	memset (r_skyfaces, 0, 6*sizeof(*r_skyfaces));
	for (i=0 ; i<6 ; i++)
	{
		r_skyplanes[i].normal[skybox_planes[i*2]] = 1;
		r_skyplanes[i].dist = skybox_planes[i*2+1];

		VectorCopy (box_vecs[i][0], r_skytexinfo[i].vecs[0]);
		VectorCopy (box_vecs[i][1], r_skytexinfo[i].vecs[1]);

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

	r_skyboxinitialized = true;
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

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
=============
LoadTGA
=============
*/
void R_LoadTGA (const char *name, byte **pic, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	int		length;
	TargaHeader		targa_header;
	byte			*targa_rgba;
	byte tmp[2];

	*pic = NULL;

	//
	// load the file
	//
	static std::vector<byte> buffer;
	int handle = -1;
	length = COM_OpenFile(name, &handle);
	if (handle >= 0 && length > 0)
	{
		buffer.resize(length);
		if (Sys_FileRead(handle, buffer.data(), length) != length)
		{
			buffer.clear();
		}
		COM_CloseFile(handle);
	}
	if (buffer.size() == 0)
	{
		Con_Printf("Bad tga file %s\n", name);
		return;
	}

	buf_p = buffer.data();

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_index = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_length = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.y_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.width = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.height = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type!=2
		&& targa_header.image_type!=10)
		Con_Printf ("R_LoadTGA: Only type 2 and 10 targa RGB images supported\n");

	if (targa_header.colormap_type !=0
		|| (targa_header.pixel_size!=32 && targa_header.pixel_size!=24))
		Con_Printf ("R_LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = new byte[numPixels*4];
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment

	if (targa_header.image_type==2) {  // Uncompressed, RGB images
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; column++) {
				unsigned char red,green,blue,alphabyte;
				switch (targa_header.pixel_size) {
					case 24:

						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = 255;
						break;
					case 32:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = *buf_p++;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						break;
				}
			}
		}
	}
	else if (targa_header.image_type==10) {   // Runlength encoded RGB images
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; ) {
				packetHeader= *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) {        // run-length packet
					switch (targa_header.pixel_size) {
						case 24:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = 255;
							break;
						case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							break;
					}

					for(j=0;j<packetSize;j++) {
						*pixbuf++=red;
						*pixbuf++=green;
						*pixbuf++=blue;
						*pixbuf++=alphabyte;
						column++;
						if (column==columns) { // run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
				else {                            // non run-length packet
					for(j=0;j<packetSize;j++) {
						switch (targa_header.pixel_size) {
							case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = 255;
								break;
							case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
						}
						column++;
						if (column==columns) { // pixel packet run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
			}
			breakOut:;
		}
	}
}


void R_LoadSkyImage(std::string& path, texture_t*& texture)
{
	if (texture != nullptr)
	{
		delete[] texture;
		texture = nullptr;
	}
	byte* pic;
	int width;
	int height;
	R_LoadTGA(path.c_str(), &pic, &width, &height);
	if (pic != nullptr)
	{
		texture = (texture_t*)new byte[sizeof(texture_t) + width * height];
		memset(texture, 0, sizeof(texture_t));
		texture->width = width;
		texture->height = height;
		texture->offsets[0] = sizeof(texture_t);
		auto source = (byte*)pic;
		auto target = (byte*)texture + sizeof(texture_t);
		for (auto j = 0; j < height; j++)
		{
			for (auto i = 0; i < width; i++)
			{
				auto nearestDistanceSquared = INT_MAX;
				int nearest;
				auto r1 = *source++;
				auto g1 = *source++;
				auto b1 = *source++;
				for (auto p = 0; p < 768; )
				{
					auto r2 = host_basepal[p++];
					auto g2 = host_basepal[p++];
					auto b2 = host_basepal[p++];
					auto dr = r2 - r1;
					auto dg = g2 - g1;
					auto db = b2 - b1;
					auto distanceSquared = dr * dr + dg * dg + db * db;
					if (nearestDistanceSquared > distanceSquared)
					{
						nearestDistanceSquared = distanceSquared;
						nearest = p - 3;
					}
				}
				source++;
				*target++ = nearest / 3;
			}
		}
		delete[] pic;
	}
}

/*
============
R_SetSkyBox
============
*/
char	*suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
int	r_skysideimage[6] = {5, 2, 4, 1, 0, 3};

void R_SetSkyBox (float rotate, vec3_t axis)
{
	r_skyrotate = rotate;
	VectorCopy (axis, r_skyaxis);

	for (auto i=0 ; i<6 ; i++)
	{
		std::string path = "gfx/env/" + r_skyboxprefix + suf[r_skysideimage[i]] + ".tga";
		R_LoadSkyImage(path, r_skytexinfo[i].texture);
        r_skyfaces[i].texturemins[0] = -r_skytexinfo[i].texture->width / 2;
        r_skyfaces[i].texturemins[1] = -r_skytexinfo[i].texture->height / 2;
        r_skyfaces[i].extents[0] = r_skytexinfo[i].texture->width;
        r_skyfaces[i].extents[1] = r_skytexinfo[i].texture->height;
	}
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


