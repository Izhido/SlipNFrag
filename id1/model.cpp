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
// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"
#include "r_local.h"

model_t	*loadmodel;
char	loadname[32];	// for hunk tags

void Mod_LoadSpriteModel (model_t *mod, void *buffer);
void Mod_LoadBrushModel (model_t *mod, void *buffer);
void Mod_LoadAliasModel (model_t *mod, void *buffer);
model_t *Mod_LoadModel (model_t *mod, qboolean crash);

std::vector<byte> mod_novis(MAX_MAP_LEAFS/8);

std::list<model_t> mod_known;

struct mpool_t
{
	std::vector<std::vector<mvertex_t>> vertexes;
	std::vector<std::vector<medge_t>> edges;
	std::vector<std::vector<int>> surfedges;
	std::vector<std::vector<texture_t*>> texturelists;
	std::vector<std::vector<byte>> textures;
	std::vector<std::vector<byte>> lightdata;
	std::vector<std::vector<byte>> lightRGBdata;
	std::vector<std::vector<mplane_t>> planes;
	std::vector<std::vector<mtexinfo_t>> texinfo;
	std::vector<std::vector<msurface_t>> surfaces;
	std::vector<std::vector<msurface_t*>> marksurfaces;
	std::vector<std::vector<byte>> visdata;
	std::vector<std::vector<mleaf_t>> leafs;
	std::vector<std::vector<mnode_t>> nodes;
	std::vector<std::vector<mclipnode_t>> clipnodes;
	std::vector<std::vector<char>> entities;
	std::vector<std::vector<dmodel_t>> submodels;
	std::vector<std::vector<byte>> sprites;

	void Clear()
	{
		for (auto& list : texturelists)
		{
			for (auto texture : list)
			{
				if (texture != nullptr)
				{
					delete[] (byte*)(texture->external_glow);
					delete[] (byte*)(texture->external_color);
				}
			}
		}
		sprites.clear();
		submodels.clear();
		entities.clear();
		clipnodes.clear();
		nodes.clear();
		leafs.clear();
		visdata.clear();
		marksurfaces.clear();
		surfaces.clear();
		texinfo.clear();
		planes.clear();
		lightRGBdata.clear();
		lightdata.clear();
		textures.clear();
		texturelists.clear();
		surfedges.clear();
		edges.clear();
		vertexes.clear();
	}
};

mpool_t mod_pool;

std::vector<byte> mod_aliaspool;

// values for model_t's needload
#define NL_PRESENT		0
#define NL_NEEDS_LOADED	1
#define NL_UNREFERENCED	2

/*
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	memset (mod_novis.data(), 0xff, mod_novis.size());
}

/*
===============
Mod_Extradata

Caches the data if needed
===============
*/
void *Mod_Extradata (model_t *mod)
{
	return mod->extradata;
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *Mod_PointInLeaf (const vec3_t p, model_t *model)
{
	mnode_t		*node;
	float		d;
	mplane_t	*plane;

	if (!model || !model->nodes)
		Sys_Error ("Mod_PointInLeaf: bad model");

	node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t *)node;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
	
	return NULL;	// never reached
}


/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static std::vector<byte> decompressed(MAX_MAP_LEAFS/8);
	int		c;
	byte	*out;
	int		row;

	row = (model->numleafs+7)>>3;	
	size_t size = (model->numleafs+31)>>3;
	if (decompressed.size() < size)
	{
		decompressed.resize(size);
	}
	auto start = decompressed.data();
	out = start;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return start;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - start < row);
	
	return start;
}

byte *Mod_LeafPVS (mleaf_t *leaf, model_t *model)
{
	if (leaf == model->leafs)
	{
		size_t newsize = (model->numleafs+7)>>3;
		if (mod_novis.size() < newsize)
		{
			auto oldsize = mod_novis.size();
			mod_novis.resize(newsize);
			memset(mod_novis.data() + oldsize, 0xff, newsize - oldsize);
		}
		return mod_novis.data();
	}
	return Mod_DecompressVis (leaf->compressed_vis, model);
}

/*
===================
Mod_ClearAll
===================
*/
void Mod_ClearAll (void)
{
	mod_pool.Clear();
	for (auto& mod : mod_known) {
		mod.needload = NL_UNREFERENCED;
		if (mod.type == mod_sprite) mod.extradata = NULL;
	}
}

/*
===================
Mod_ClearCacheSurfaces
===================
*/
void Mod_ClearCacheSurfaces()
{
	for (auto& list : mod_pool.surfaces)
	{
		for (auto& surface : list)
		{
			std::fill(surface.cachespots, surface.cachespots + MIPLEVELS, nullptr);
		}
	}
}

/*
==================
Mod_FindName

==================
*/
model_t *Mod_FindName (const char *name)
{
	int		i;
	model_t	*mod = nullptr;
	model_t	*avail = NULL;

	if (!name[0])
		Sys_Error ("Mod_ForName: NULL name");
		
//
// search the currently loaded models
//
	i = 0;
	for (auto& entry : mod_known)
	{
		mod = &entry;
		if (!strcmp (pr_strings + mod->name, name) )
			break;
		if (mod->needload == NL_UNREFERENCED)
			if (!avail || mod->type != mod_alias)
				avail = mod;
		i++;
	}
			
	if (i == mod_known.size())
	{
			if (avail)
			{
				mod = avail;
				if (mod->type == mod_alias)
				{
					delete[] mod->extradata;
				}
				mod->extradata = nullptr;
			}
			else
			{
				mod_known.emplace_back();
				mod = &mod_known.back();
			}
		auto name_len = (int)strlen(name);
		mod->name = ED_NewString(name_len + 1);
		Q_strncpy(pr_strings + mod->name, name, name_len);
		mod->needload = NL_NEEDS_LOADED;
	}

	return mod;
}

/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel (char *name)
{
}

/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
model_t *Mod_LoadModel (model_t *mod, qboolean crash)
{
	unsigned *buf;

	if (mod->type == mod_alias)
	{
		if (mod->extradata != nullptr)
		{
			mod->needload = NL_PRESENT;
			return mod;
		}
	}
	else
	{
		if (mod->needload == NL_PRESENT)
			return mod;
	}

//
// because the world is so huge, load it one piece at a time
//
	
//
// load the file
//
	std::vector<byte> contents;
	buf = (unsigned *)COM_LoadFile (pr_strings + mod->name, contents);
	if (!buf)
	{
		if (crash)
			Sys_Error ("Mod_NumForName: %s not found", pr_strings + mod->name);
		return NULL;
	}
	
//
// allocate a new model
//
	COM_FileBase (pr_strings + mod->name, loadname);
	
	loadmodel = mod;

//
// fill it in
//

// call the apropriate loader
	mod->needload = NL_PRESENT;

	switch (LittleLong(*(unsigned *)buf))
	{
	case IDPOLYHEADER:
		Mod_LoadAliasModel (mod, buf);
		break;
		
	case IDSPRITEHEADER:
		Mod_LoadSpriteModel (mod, buf);
		break;
	
	default:
		Mod_LoadBrushModel (mod, buf);
		break;
	}

	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName (const char *name, qboolean crash)
{
	model_t	*mod;

	mod = Mod_FindName (name);

	return Mod_LoadModel (mod, crash);
}


/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

byte	*mod_base;


/*
=================
Mod_AveragePixels
=================
*/
byte Mod_AveragePixels (std::vector<byte>& pixdata)
{
	int		r,g,b;
	int		i;
	int		vis;
	int		pix;
	int		dr, dg, db;
	int		bestdistortion, distortion;
	int		bestcolor;
	byte	*pal;
	int		e;
	
	vis = 0;
	r = g = b = 0;
	for (i=0 ; i<pixdata.size() ; i++)
	{
		pix = pixdata[i];
		if (pix == 255)
		{
			continue;
		}
		else if (pix >= 224)
		{
			return pix;
		}

		r += host_basepal[pix*3];
		g += host_basepal[pix*3+1];
		b += host_basepal[pix*3+2];
		vis++;
	}
	
	if (vis == 0)
		return 255;
		
	r /= vis;
	g /= vis;
	b /= vis;
	
//
// find the best color
//
	bestdistortion = r*r + g*g + b*b;
	bestcolor = 0;
	i = 0;
	e = 224;

	for ( ; i< e ; i++)
	{
		pix = i;	//pixdata[i];

		pal = host_basepal.data() + pix*3;

		dr = r - (int)pal[0];
		dg = g - (int)pal[1];
		db = b - (int)pal[2];

		distortion = dr*dr + dg*dg + db*db;
		if (distortion < bestdistortion)
		{
			if (!distortion)
			{
				return pix;		// perfect match
			}

			bestdistortion = distortion;
			bestcolor = pix;
		}
	}

	return bestcolor;
}

/*
=================
Mod_GenerateMipmaps
=================
*/
void Mod_GenerateMipmaps (byte* data, int w, int h)
{
	auto source = data;
	auto lump_p = data + w * h;
	std::vector<byte> pixdata;
	for (auto miplevel = 1 ; miplevel<MIPLEVELS ; miplevel++)
	{
		auto mipstep = 1<<miplevel;
		pixdata.resize(mipstep * mipstep);
		for (auto y=0 ; y<h ; y+=mipstep)
		{
			for (auto x = 0 ; x<w ; x+= mipstep)
			{
				size_t count = 0;
				for (auto yy=0 ; yy<mipstep ; yy++)
					for (auto xx=0 ; xx<mipstep ; xx++)
					{
						pixdata[count++] = source[ (y+yy)*w + x + xx ];
					}
				*lump_p++ = Mod_AveragePixels (pixdata);
			}
		}
	}
}

/*
=================
Mod_GenerateRGBAMipmaps
=================
*/
void Mod_GenerateRGBAMipmaps (byte* data, int w, int h)
{
	auto source = data;
	auto lump_p = data + w * h * 4;
	for (auto miplevel = 1 ; miplevel<MIPLEVELS ; miplevel++)
	{
		auto mipstep = 1<<miplevel;
		for (auto y=0 ; y<h ; y+=mipstep)
		{
			for (auto x = 0 ; x<w ; x+= mipstep)
			{
				unsigned r = 0;
				unsigned g = 0;
				unsigned b = 0;
				unsigned a = 0;
				unsigned total = mipstep*mipstep;
				for (auto yy=0 ; yy<mipstep ; yy++)
					for (auto xx=0 ; xx<mipstep ; xx++)
					{
						auto pos = 4 * ((y+yy)*w + x + xx);
						r += source[pos++];
						g += source[pos++];
						b += source[pos++];
						a += source[pos];
					}
				*lump_p++ = r / total;
				*lump_p++ = g / total;
				*lump_p++ = b / total;
				*lump_p++ = a / total;
			}
		}
	}
}

/*
=================
Mod_LoadTextures
=================
*/
void Mod_LoadTextures (lump_t *l)
{
	int		i, j, pixels, num, max, altmax;
	miptex_t	*mt;
	texture_t	*tx, *tx2;
	texture_t	*anims[10];
	texture_t	*altanims[10];
	dmiptexlump_t *m;
	std::vector<char> modelname;
	std::string external_filename;
	int external_size;
	int external_width;
	int external_height;
	std::string external_glow_filename;
	int external_glow_width;
	int external_glow_height;

	if (!l->filelen)
	{
		loadmodel->textures = NULL;
		return;
	}
	m = (dmiptexlump_t *)(mod_base + l->fileofs);
	
	m->nummiptex = LittleLong (m->nummiptex);
	
	loadmodel->numtextures = m->nummiptex;
	mod_pool.texturelists.emplace_back(m->nummiptex);
	loadmodel->textures = mod_pool.texturelists.back().data();

	modelname.resize(strlen(pr_strings + loadmodel->name + 5) + 1); // +5 to skip "maps/" during copy...
	COM_StripExtension(pr_strings + loadmodel->name + 5, modelname.data());

	for (i=0 ; i<m->nummiptex ; i++)
	{
		m->dataofs[i] = LittleLong(m->dataofs[i]);
		if (m->dataofs[i] == -1)
			continue;
		mt = (miptex_t *)((byte *)m + m->dataofs[i]);
		mt->width = LittleLong (mt->width);
		mt->height = LittleLong (mt->height);
		for (j=0 ; j<MIPLEVELS ; j++)
			mt->offsets[j] = LittleLong (mt->offsets[j]);
		
		if ( (mt->width & 15) || (mt->height & 15) )
			Sys_Error ("Texture %s is not 16 aligned", mt->name);
		pixels = mt->width*mt->height/64*85;
		mod_pool.textures.emplace_back(sizeof(texture_t) + pixels);
		tx = (texture_t*)mod_pool.textures.back().data();
		loadmodel->textures[i] = tx;

		memcpy (tx->name, mt->name, sizeof(tx->name));
		tx->width = mt->width;
		tx->height = mt->height;
		for (j=0 ; j<MIPLEVELS ; j++)
			tx->offsets[j] = mt->offsets[j] + sizeof(texture_t) - sizeof(miptex_t);
		// the pixels immediately follow the structures
		auto remaining = (mod_base + l->fileofs + l->filelen) - ((byte*)(mt+1));
		if (remaining < pixels)
		{
			Con_Printf("Mod_LoadTextures: %s extends past the end of the lump\n", mt->name);
			if (remaining >= mt->width*mt->height)
			{
				memcpy ( tx+1, mt+1, mt->width*mt->height);
				Mod_GenerateMipmaps ((byte*)(tx+1), mt->width, mt->height);
			}
			else if (remaining > 0)
			{
				memcpy ( tx+1, mt+1, remaining);
				memset ( tx+1+remaining, 0, mt->width*mt->height-remaining);
				Mod_GenerateMipmaps ((byte*)(tx+1), mt->width, mt->height);
			}
			else
			{
				memset ( tx+1, 0, pixels);
			}
		}
		else if (tx->name[0] == '{')
		{
			memcpy ( tx+1, mt+1, mt->width*mt->height);
			Mod_GenerateMipmaps ((byte*)(tx+1), mt->width, mt->height);
		}
		else
		{
			memcpy ( tx+1, mt+1, pixels);
		}
		if (!Q_strncmp(mt->name,"sky",3))
			R_InitSky (tx);

		// load external textures, if present
		if (r_load_as_rgba)
		{
			if (!Q_strncmp(mt->name,"sky",3))
			{
				external_filename = std::string("textures/") + modelname.data() + "/" + tx->name + ".tga";
				external_size = 0;
				auto found = R_LoadTGA(external_filename.c_str(), sizeof(miptex_t), false, MIPLEVELS, false, (byte**)(&tx->external_color), &external_size, &external_width, &external_height);
				if (!found)
				{
					external_filename = std::string("textures/") + tx->name + ".tga";
					found = R_LoadTGA(external_filename.c_str(), sizeof(miptex_t), false, MIPLEVELS, false, (byte**)(&tx->external_color), &external_size, &external_width, &external_height);
				}
				if (found)
				{
					memcpy (tx->external_color->name, mt->name, sizeof(tx->external_color->name));
					tx->external_color->width = external_width;
					tx->external_color->height = external_height;
					tx->external_color->offsets[0] = sizeof(miptex_t);
					tx->external_color->offsets[1] = tx->external_color->offsets[0] + external_width*external_height*4;
					tx->external_color->offsets[2] = tx->external_color->offsets[1] + external_width*external_height;
					tx->external_color->offsets[3] = tx->external_color->offsets[2] + external_width*external_height/4;
					Mod_GenerateRGBAMipmaps((byte*)tx->external_color + tx->external_color->offsets[0], external_width, external_height);
					Con_DPrintf("Mod_LoadTextures: loaded %s (%i x %i) from (%i x %i)\n", external_filename.c_str(), external_width, external_height, tx->width, tx->height);

					R_InitSkyRGBA(tx->external_color);
				}
			}
			else
			{
				auto is_turbulent = (tx->name[0] == '*');
				if (is_turbulent)
				{
					external_filename = std::string("textures/") + modelname.data() + "/#" + (tx->name + 1) + ".tga";
				}
				else
				{
					external_filename = std::string("textures/") + modelname.data() + "/" + tx->name + ".tga";
				}
				external_size = 0;
				auto found = R_LoadTGA(external_filename.c_str(), sizeof(miptex_t), false, MIPLEVELS, false, (byte**)(&tx->external_color), &external_size, &external_width, &external_height);
				if (!found)
				{
					if (is_turbulent)
					{
						external_filename = std::string("textures/#") + (tx->name + 1) + ".tga";
					}
					else
					{
						external_filename = std::string("textures/") + tx->name + ".tga";
					}
					found = R_LoadTGA(external_filename.c_str(), sizeof(miptex_t), false, MIPLEVELS, false, (byte**)(&tx->external_color), &external_size, &external_width, &external_height);
				}
				if (found)
				{
					memcpy (tx->external_color->name, mt->name, sizeof(tx->external_color->name));
					tx->external_color->width = external_width;
					tx->external_color->height = external_height;
					tx->external_color->offsets[0] = sizeof(miptex_t);
					tx->external_color->offsets[1] = tx->external_color->offsets[0] + external_width*external_height*4;
					tx->external_color->offsets[2] = tx->external_color->offsets[1] + external_width*external_height;
					tx->external_color->offsets[3] = tx->external_color->offsets[2] + external_width*external_height/4;
					Mod_GenerateRGBAMipmaps((byte*)tx->external_color + tx->external_color->offsets[0], external_width, external_height);
					Con_DPrintf("Mod_LoadTextures: loaded %s (%i x %i) from (%i x %i)\n", external_filename.c_str(), external_width, external_height, tx->width, tx->height);
					if (!is_turbulent)
					{
						external_glow_filename = std::string("textures/") + modelname.data() + "/" + tx->name + "_glow.tga";
						external_size = 0;
						found = R_LoadTGA(external_glow_filename.c_str(), sizeof(miptex_t), false, MIPLEVELS, false, (byte**)(&tx->external_glow), &external_size, &external_glow_width, &external_glow_height);
						if (!found)
						{
							external_glow_filename = std::string("textures/") + modelname.data() + "/" + tx->name + "_luma.tga";
							found = R_LoadTGA(external_glow_filename.c_str(), sizeof(miptex_t), false, MIPLEVELS, false, (byte**)(&tx->external_glow), &external_size, &external_glow_width, &external_glow_height);
							if (!found)
							{
								external_glow_filename = std::string("textures/") + tx->name + "_glow.tga";
								found = R_LoadTGA(external_glow_filename.c_str(), sizeof(miptex_t), false, MIPLEVELS, false, (byte**)(&tx->external_glow), &external_size, &external_glow_width, &external_glow_height);
								if (!found)
								{
									external_glow_filename = std::string("textures/") + tx->name + "_luma.tga";
									found = R_LoadTGA(external_glow_filename.c_str(), sizeof(miptex_t), false, MIPLEVELS, false, (byte**)(&tx->external_glow), &external_size, &external_glow_width, &external_glow_height);
								}
							}
						}
						if (found)
						{
							memcpy (tx->external_glow->name, mt->name, sizeof(tx->external_glow->name));
							tx->external_glow->width = external_glow_width;
							tx->external_glow->height = external_glow_height;
							tx->external_glow->offsets[0] = sizeof(miptex_t);
							tx->external_glow->offsets[1] = tx->external_glow->offsets[0] + external_glow_width*external_glow_height*4;
							tx->external_glow->offsets[2] = tx->external_glow->offsets[1] + external_glow_width*external_glow_height;
							tx->external_glow->offsets[3] = tx->external_glow->offsets[2] + external_glow_width*external_glow_height/4;
							Mod_GenerateRGBAMipmaps((byte*)tx->external_glow + tx->external_glow->offsets[0], external_glow_width, external_glow_height);
							if (tx->external_color->width != tx->external_glow->width || tx->external_color->height != tx->external_glow->height)
							{
								delete[] (byte*)(tx->external_glow);
								tx->external_glow = nullptr;
								delete[] (byte*)(tx->external_color);
								tx->external_color = nullptr;
								Con_Printf("Mod_LoadTextures: the size of %s (%i x %i) does not match the size of %s (%i x %i)\n", external_filename.c_str(), external_width, external_height, external_glow_filename.c_str(), external_glow_width, external_glow_height);
							}
							else
							{
								Con_DPrintf("Mod_LoadTextures: loaded %s (%i x %i) from (%i x %i)\n", external_filename.c_str(), external_width, external_height, tx->width, tx->height);
							}
						}
					}
				}
			}
		}
	}

//
// sequence the animations
//
	for (i=0 ; i<m->nummiptex ; i++)
	{
		tx = loadmodel->textures[i];
		if (!tx || tx->name[0] != '+')
			continue;
		if (tx->anim_next)
			continue;	// allready sequenced

	// find the number of frames in the animation
		memset (anims, 0, sizeof(anims));
		memset (altanims, 0, sizeof(altanims));

		max = tx->name[1];
		altmax = 0;
		if (max >= 'a' && max <= 'z')
			max -= 'a' - 'A';
		if (max >= '0' && max <= '9')
		{
			max -= '0';
			altmax = 0;
			anims[max] = tx;
			max++;
		}
		else if (max >= 'A' && max <= 'J')
		{
			altmax = max - 'A';
			max = 0;
			altanims[altmax] = tx;
			altmax++;
		}
		else
			Sys_Error ("Bad animating texture %s", tx->name);

		for (j=i+1 ; j<m->nummiptex ; j++)
		{
			tx2 = loadmodel->textures[j];
			if (!tx2 || tx2->name[0] != '+')
				continue;
			if (strcmp (tx2->name+2, tx->name+2))
				continue;

			num = tx2->name[1];
			if (num >= 'a' && num <= 'z')
				num -= 'a' - 'A';
			if (num >= '0' && num <= '9')
			{
				num -= '0';
				anims[num] = tx2;
				if (num+1 > max)
					max = num + 1;
			}
			else if (num >= 'A' && num <= 'J')
			{
				num = num - 'A';
				altanims[num] = tx2;
				if (num+1 > altmax)
					altmax = num+1;
			}
			else
				Sys_Error ("Bad animating texture %s", tx->name);
		}
		
#define	ANIM_CYCLE	2
	// link them all together
		for (j=0 ; j<max ; j++)
		{
			tx2 = anims[j];
			if (!tx2)
				Sys_Error ("Missing frame %i of %s",j, tx->name);
			tx2->anim_total = max * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j+1) * ANIM_CYCLE;
			tx2->anim_next = anims[ (j+1)%max ];
			if (altmax)
				tx2->alternate_anims = altanims[0];
		}
		for (j=0 ; j<altmax ; j++)
		{
			tx2 = altanims[j];
			if (!tx2)
				Sys_Error ("Missing frame %i of %s",j, tx->name);
			tx2->anim_total = altmax * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j+1) * ANIM_CYCLE;
			tx2->anim_next = altanims[ (j+1)%altmax ];
			if (max)
				tx2->alternate_anims = anims[0];
		}
	}
}

/*
=================
Mod_Load24To8Table
=================
*/
void Mod_Load24To8Table (void)
{
	if (r_24to8table.size() == 0)
	{
		Draw_BeginDisc ();
		Con_Printf("Creating color table\n");
		auto start = Sys_FloatTime ();
		R_Load24To8Coverage ();
		auto palette = host_basepal.data();
		r_24to8table.resize(256 * 256 * 256);
		r_24to8tableptr = r_24to8table.data();
		auto target = r_24to8tableptr;
		auto lastCoverageEntry = -1;
		auto lastCoverageR = -1;
		auto lastCoverageG = -1;
		auto lastCoverageB = -1;
		auto lastCoverage = -1;
		for (auto r = 0; r < 256; r++)
		{
			for (auto g = 0; g < 256; g++)
			{
				for (auto b = 0; b < 256; b++)
				{
					qboolean search = true;
					if (lastCoverage > 0)
					{
						auto deltaR = r - lastCoverageR;
						auto deltaG = g - lastCoverageG;
						auto deltaB = b - lastCoverageB;
						auto distance = deltaR * deltaR + deltaG * deltaG + deltaB * deltaB;
						if (distance <= lastCoverage)
						{
							*target++ = lastCoverageEntry;
							search = false;
						}
					}
					if (search)
					{
						auto entry = 0;
						auto sourcePalette = palette;
						auto closest = UINT_MAX;
						for (auto i = 0; i < 224; i++)
						{
							auto deltaR = r - *sourcePalette++;
							auto deltaG = g - *sourcePalette++;
							auto deltaB = b - *sourcePalette++;
							auto distance = deltaR * deltaR + deltaG * deltaG + deltaB * deltaB;
							if (closest > distance)
							{
								closest = distance;
								entry = i;
							}
						}
						*target++ = entry;
						lastCoverageEntry = entry;
						entry *= 4;
						lastCoverageR = host_basepalcoverage[entry++];
						lastCoverageG = host_basepalcoverage[entry++];
						lastCoverageB = host_basepalcoverage[entry++];
						lastCoverage = host_basepalcoverage[entry++];
					}
				}
			}
		}
		auto stop = Sys_FloatTime();
		Sys_Printf("R_Load24To8Table: %.3f seconds\n", stop - start);
		Draw_EndDisc ();
	}
}

/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting (lump_t *l)
{
	loadmodel->lightRGBdata = NULL;

	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		return;
	}
	mod_pool.lightdata.emplace_back(l->filelen);
	loadmodel->lightdata = mod_pool.lightdata.back().data();
	memcpy (loadmodel->lightdata, mod_base + l->fileofs, l->filelen);

	std::vector<char> litfilename(strlen(pr_strings + loadmodel->name) + 5); // Extra space for the new extension just in case...
	COM_StripExtension(pr_strings + loadmodel->name, litfilename.data());
	strcat(litfilename.data(), ".lit");

	std::vector<byte> contents;
	if (COM_LoadFile (litfilename.data(), false, contents) != nullptr)
	{
		auto size = 8+l->filelen*3;
		if (size == contents.size() - 1)
		{
			if (contents[0] == 'Q' && contents[1] == 'L' && contents[2] == 'I' && contents[3] == 'T')
			{
				auto version = LittleLong(*((int*)(contents.data() + 4)));
				if (version == 1)
				{
					Mod_Load24To8Table();
					mod_pool.lightRGBdata.emplace_back(l->filelen*3);
					loadmodel->lightRGBdata = mod_pool.lightRGBdata.back().data();
					memcpy (loadmodel->lightRGBdata, contents.data() + 8, size-8);
					return;
				}
				else
				{
					Con_Printf("Mod_LoadLighting: Invalid .lit file version (expected 1, found %i)\n", version);
				}
			}
			else
			{
				contents[4] = 0;
				Con_Printf("Mod_LoadLighting: Invalid .lit file signature (expected 'QLIT', found %s)\n", contents.data());
			}
		}
		else
		{
			Con_Printf("Mod_LoadLighting: Invalid .lit file size (expected %i, found %i)\n", size, contents.size() - 1);
		}
	}
}


/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility (lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->visdata = NULL;
		return;
	}
	mod_pool.visdata.emplace_back(l->filelen);
	loadmodel->visdata = mod_pool.visdata.back().data();
	memcpy (loadmodel->visdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadEntities
=================
*/
void Mod_LoadEntities (lump_t *l)
{
	if (!l->filelen)
	{
		loadmodel->entities = NULL;
		return;
	}
	mod_pool.entities.emplace_back(l->filelen);
	loadmodel->entities = mod_pool.entities.back().data();
	memcpy (loadmodel->entities, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes (lump_t *l)
{
	dvertex_t	*in;
	mvertex_t	*out;
	int			i, count;

	in = (dvertex_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.vertexes.emplace_back(count + 8); // Extra in case a skybox is loaded
	out = mod_pool.vertexes.back().data();

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}
}

/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels (lump_t *l)
{
	dmodel_t	*in;
	dmodel_t	*out;
	int			i, j, count;

	in = (dmodel_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.submodels.emplace_back(count);
	out = mod_pool.submodels.back().data();

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		for (j=0 ; j<MAX_MAP_HULLS ; j++)
			out->headnode[j] = LittleLong (in->headnode[j]);
		out->visleafs = LittleLong (in->visleafs);
		out->firstface = LittleLong (in->firstface);
		out->numfaces = LittleLong (in->numfaces);
	}
}

/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges (lump_t *l)
{
	dedge_t *in;
	medge_t *out;
	int 	i, count;

	in = (dedge_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.edges.emplace_back(count + 1 + 12); // Extra in case a skybox is loaded
	out = mod_pool.edges.back().data();

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->v[0] = (unsigned short)LittleShort(in->v[0]);
		out->v[1] = (unsigned short)LittleShort(in->v[1]);
	}
}

/*
=================
Mod_LoadBSP2Edges
=================
*/
void Mod_LoadBSP2Edges (lump_t *l)
{
	dbsp2edge_t *in;
	medge_t *out;
	int	 i, count;

	in = (dbsp2edge_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("Mod_LoadBSP2Edges: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.edges.emplace_back(count + 1 + 12); // Extra in case a skybox is loaded
	out = mod_pool.edges.back().data();

	loadmodel->edges = out;
	loadmodel->numedges = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->v[0] = (unsigned int)LittleLong(in->v[0]);
		out->v[1] = (unsigned int)LittleLong(in->v[1]);
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
void Mod_LoadTexinfo (lump_t *l)
{
	texinfo_t *in;
	mtexinfo_t *out;
	int 	i, j, k, count;
	int		miptex;
	float	len1, len2;

	in = (texinfo_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.texinfo.emplace_back(count);
	out = mod_pool.texinfo.back().data();

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<2 ; j++)
			for (k=0 ; k<4 ; k++)
				out->vecs[j][k] = LittleFloat (in->vecs[j][k]);
		len1 = Length (out->vecs[0]);
		len2 = Length (out->vecs[1]);
		len1 = (len1 + len2)/2;
		if (len1 < 0.32)
			out->mipadjust = 4;
		else if (len1 < 0.49)
			out->mipadjust = 3;
		else if (len1 < 0.99)
			out->mipadjust = 2;
		else
			out->mipadjust = 1;
#if 0
		if (len1 + len2 < 0.001)
			out->mipadjust = 1;		// don't crash
		else
			out->mipadjust = 1 / floor( (len1+len2)/2 + 0.1 );
#endif

		miptex = LittleLong (in->miptex);
		out->flags = LittleLong (in->flags);
	
		if (!loadmodel->textures)
		{
			out->texture = r_notexture_mip;	// checkerboard texture
			out->flags = 0;
		}
		else
		{
			if (miptex >= loadmodel->numtextures)
				Sys_Error ("miptex >= loadmodel->numtextures");
			out->texture = loadmodel->textures[miptex];
			if (!out->texture)
			{
				out->texture = r_notexture_mip; // texture not found
				out->flags = 0;
			}
		}
	}
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents (msurface_t *s)
{
	float	mins[2], maxs[2], val;
	int		i,j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;
	
	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];
		
		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->vecs[j][0] + 
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{	
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}


/*
=================
Mod_LoadFaces
=================
*/
void Mod_LoadFaces (lump_t *l)
{
	dface_t		*in;
	msurface_t 	*out;
	int			i, count, surfnum;
	int			planenum, side;

	in = (dface_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.surfaces.emplace_back(count + 6); // Extra in case a skybox is loaded
	out = mod_pool.surfaces.back().data();

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for ( surfnum=0 ; surfnum<count ; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);		
		out->flags = 0;

		planenum = LittleShort(in->planenum);
		side = LittleShort(in->side);
		if (side)
			out->flags |= SURF_PLANEBACK;			

		out->plane = loadmodel->planes + planenum;

		out->texinfo = loadmodel->texinfo + LittleShort (in->texinfo);

		CalcSurfaceExtents (out);
				
	// lighting info

		for (i=0 ; i<MAXLIGHTMAPS ; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);
		if (i == -1)
		{
			out->samples = NULL;
			out->samplesRGB = NULL;
		}
		else
		{
			out->samples = loadmodel->lightdata + i;
			if (loadmodel->lightRGBdata != NULL)
			{
				out->samplesRGB = loadmodel->lightRGBdata + i * 3;
			}
		}

	// set the drawing flags flag
		
		if (!Q_strncmp(out->texinfo->texture->name,"sky",3))	// sky
		{
			out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
			continue;
		}
		
		if (!Q_strncmp(out->texinfo->texture->name,"*",1))		// turbulent
		{
			out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
			if (!Q_strncmp(out->texinfo->texture->name+1,"lava",4))
				out->flags |= SURF_DRAWLAVA;
			else if (!Q_strncmp(out->texinfo->texture->name+1,"tele",4))
				out->flags |= SURF_DRAWTELE;
			else if (!Q_strncmp(out->texinfo->texture->name+1,"slime",5))
				out->flags |= SURF_DRAWSLIME;
			else
				out->flags |= SURF_DRAWWATER;
			continue;
		}

		if (out->texinfo->texture->name[0] == '{')
			out->flags |= SURF_DRAWFENCE;
	}
}


/*
=================
Mod_LoadBSP2Faces
=================
*/
void Mod_LoadBSP2Faces (lump_t *l)
{
	dbsp2face_t	*in;
	msurface_t	 *out;
	int			i, count, surfnum;
	int			planenum, side;

	in = (dbsp2face_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.surfaces.emplace_back(count + 6); // Extra in case a skybox is loaded
	out = mod_pool.surfaces.back().data();

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for ( surfnum=0 ; surfnum<count ; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleLong(in->numedges);
		out->flags = 0;

		planenum = LittleLong(in->planenum);
		side = LittleLong(in->side);
		if (side)
			out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + planenum;

		out->texinfo = loadmodel->texinfo + LittleLong (in->texinfo);

		CalcSurfaceExtents (out);
				
	// lighting info

		for (i=0 ; i<MAXLIGHTMAPS ; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);
		if (i == -1)
		{
			out->samples = NULL;
			out->samplesRGB = NULL;
		}
		else
		{
			out->samples = loadmodel->lightdata + i;
			if (loadmodel->lightRGBdata != NULL)
			{
				out->samplesRGB = loadmodel->lightRGBdata + i * 3;
			}
		}

	// set the drawing flags flag
		
		if (!Q_strncmp(out->texinfo->texture->name,"sky",3))	// sky
		{
			out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
			continue;
		}
		
		if (!Q_strncmp(out->texinfo->texture->name,"*",1))		// turbulent
		{
			out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
			if (!Q_strncmp(out->texinfo->texture->name+1,"lava",4))
				out->flags |= SURF_DRAWLAVA;
			else if (!Q_strncmp(out->texinfo->texture->name+1,"tele",4))
				out->flags |= SURF_DRAWTELE;
			else if (!Q_strncmp(out->texinfo->texture->name+1,"slime",5))
				out->flags |= SURF_DRAWSLIME;
			else
				out->flags |= SURF_DRAWWATER;
			continue;
		}

		if (out->texinfo->texture->name[0] == '{')
			out->flags |= SURF_DRAWFENCE;
	}
}


/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents < 0)
		return;
	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes (lump_t *l)
{
	int			i, j, count, p;
	dnode_t		*in;
	mnode_t 	*out;

	in = (dnode_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.nodes.emplace_back(count);
	out = mod_pool.nodes.back().data();

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}
	
		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = (unsigned short)LittleShort (in->firstface);
		out->numsurfaces = (unsigned short)LittleShort (in->numfaces);
		
		if (count > 32767)
		{
			for (j=0 ; j<2 ; j++)
			{
				p = (unsigned short)LittleShort (in->children[j]);
				if (p < count)
					out->children[j] = loadmodel->nodes + p;
				else
					out->children[j] = (mnode_t *)(loadmodel->leafs + 65535 - p);
			}
		}
		else
		{
			for (j=0 ; j<2 ; j++)
			{
				p = LittleShort (in->children[j]);
				if (p >= 0)
					out->children[j] = loadmodel->nodes + p;
				else
					out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
			}
		}
	}
	
	Mod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs
}

/*
=================
Mod_LoadBSP2Nodes
=================
*/
void Mod_LoadBSP2Nodes (lump_t *l)
{
	int		 i, j, count, p;
	dbsp2node_t *in;
	mnode_t	 *out;

	in = (dbsp2node_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("Mod_LoadBSP2Nodes: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.nodes.emplace_back(count);
	out = mod_pool.nodes.back().data();

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleFloat (in->mins[j]);
			out->minmaxs[3+j] = LittleFloat (in->maxs[j]);
		}
	
		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = (unsigned int)LittleLong (in->firstface);
		out->numsurfaces = (unsigned int)LittleLong (in->numfaces);
		
		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = loadmodel->nodes + p;
			else
				out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
		}
	}
	
	Mod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs (lump_t *l)
{
	dleaf_t 	*in;
	mleaf_t 	*out;
	int			i, j, count, p;

	in = (dleaf_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.leafs.emplace_back(count);
	out = mod_pool.leafs.back().data();

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->firstmarksurface = loadmodel->marksurfaces +
			(unsigned short)LittleShort(in->firstmarksurface);
		out->nummarksurfaces = (unsigned short)LittleShort(in->nummarksurfaces);
		
		p = LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->visdata + p;
		out->efrags = NULL;
		
		for (j=0 ; j<4 ; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];
	}	
}

/*
=================
Mod_LoadBSP2Leafs
=================
*/
void Mod_LoadBSP2Leafs (lump_t *l)
{
	dbsp2leaf_t *in;
	mleaf_t	 *out;
	int		 i, j, count, p;

	in = (dbsp2leaf_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("Mod_LoadBSP2Leafs: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.leafs.emplace_back(count);
	out = mod_pool.leafs.back().data();

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleFloat (in->mins[j]);
			out->minmaxs[3+j] = LittleFloat (in->maxs[j]);
		}

		p = LittleLong(in->contents);
		out->contents = p;

		out->firstmarksurface = loadmodel->marksurfaces +
			LittleLong(in->firstmarksurface);
		out->nummarksurfaces = LittleLong(in->nummarksurfaces);
		
		p = LittleLong(in->visofs);
		if (p == -1)
			out->compressed_vis = NULL;
		else
			out->compressed_vis = loadmodel->visdata + p;
		out->efrags = NULL;
		
		for (j=0 ; j<4 ; j++)
			out->ambient_sound_level[j] = in->ambient_level[j];
	}
}

/*
=================
Mod_LoadClipnodes
=================
*/
void Mod_LoadClipnodes (lump_t *l)
{
	dclipnode_t *in;
	mclipnode_t *out;
	int			i, count;
	hull_t		*hull;

	in = (dclipnode_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.clipnodes.emplace_back(count);
	out = mod_pool.clipnodes.back().data();

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;

	hull = &loadmodel->hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 32;

	hull = &loadmodel->hulls[2];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 64;

	if (count > 32767)
	{
		for (i=0 ; i<count ; i++, out++, in++)
		{
			out->planenum = LittleLong(in->planenum);
			out->children[0] = (unsigned short)LittleShort(in->children[0]);
			if (out->children[0] >= count)
			{
				out->children[0] -= 65536;
			}
			out->children[1] = (unsigned short)LittleShort(in->children[1]);
			if (out->children[1] >= count)
			{
				out->children[1] -= 65536;
			}
		}
	}
	else
	{
		for (i=0 ; i<count ; i++, out++, in++)
		{
			out->planenum = LittleLong(in->planenum);
			out->children[0] = LittleShort(in->children[0]);
			out->children[1] = LittleShort(in->children[1]);
		}
	}
}

/*
=================
Mod_LoadBSP2Clipnodes
=================
*/
void Mod_LoadBSP2Clipnodes (lump_t *l)
{
	dbsp2clipnode_t *in;
	mclipnode_t *out;
	int			i, count;
	hull_t		*hull;

	in = (dbsp2clipnode_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("Mod_LoadBSP2Clipnodes: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.clipnodes.emplace_back(count);
	out = mod_pool.clipnodes.back().data();

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count;

	hull = &loadmodel->hulls[1];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 32;

	hull = &loadmodel->hulls[2];
	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;
	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 64;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planenum = LittleLong(in->planenum);
		out->children[0] = LittleLong(in->children[0]);
		out->children[1] = LittleLong(in->children[1]);
	}
}

/*
=================
Mod_MakeHull0

Deplicate the drawing hull structure as a clipping hull
=================
*/
void Mod_MakeHull0 (void)
{
	mnode_t		*in, *child;
	mclipnode_t *out;
	int			i, j, count;
	hull_t		*hull;
	
	hull = &loadmodel->hulls[0];	
	
	in = loadmodel->nodes;
	count = loadmodel->numnodes;
	mod_pool.clipnodes.emplace_back(count);
	out = mod_pool.clipnodes.back().data();

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->planes;
		for (j=0 ; j<2 ; j++)
		{
			child = in->children[j];
			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->nodes;
		}
	}
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces (lump_t *l)
{	
	int		i, j, count;
	short		*in;
	msurface_t **out;
	
	in = (short*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.marksurfaces.emplace_back(count);
	out = mod_pool.marksurfaces.back().data();

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = (unsigned short)LittleShort(in[i]);
		if (j >= loadmodel->numsurfaces)
			Sys_Error ("Mod_ParseMarksurfaces: bad surface number");
		out[i] = loadmodel->surfaces + j;
	}
}

/*
=================
Mod_LoadBSP2Marksurfaces
=================
*/
void Mod_LoadBSP2Marksurfaces (lump_t *l)
{
	int	 i, j, count;
	int		*in;
	msurface_t **out;
	
	in = (int*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("Mod_LoadBSP2Marksurfaces: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.marksurfaces.emplace_back(count);
	out = mod_pool.marksurfaces.back().data();

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = LittleLong(in[i]);
		if (j >= loadmodel->numsurfaces)
			Sys_Error ("Mod_LoadBSP2Marksurfaces: bad surface number");
		out[i] = loadmodel->surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges (lump_t *l)
{	
	int		i, count;
	int		*in, *out;
	
	in = (int*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.surfedges.emplace_back(count + 24); // Extra in case a skybox is loaded
	out = mod_pool.surfedges.back().data();

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);
}

/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes (lump_t *l)
{
	int			i, j;
	mplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;
	
	in = (dplane_t*)(void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Sys_Error ("MOD_LoadBmodel: funny lump size in %s",pr_strings + loadmodel->name);
	count = l->filelen / sizeof(*in);
	mod_pool.planes.emplace_back(count * 2);
	out = mod_pool.planes.back().data();
	
	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = bits;
	}
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds (vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return Length (corner);
}

/*
=================
Mod_LoadBrushModel
=================
*/
void Mod_LoadBrushModel (model_t *mod, void *buffer)
{
	int			i, j;
	dheader_t	*header;
	dmodel_t 	*bm;
	
	loadmodel->type = mod_brush;
	
	header = (dheader_t *)buffer;

	i = LittleLong (header->version);
	auto isbsp2 = (i == BSP2VERSION);
	if (i != BSPVERSION && !isbsp2)
		Sys_Error ("Mod_LoadBrushModel: %s has wrong version number (%i should be %i or '2PSB')", pr_strings + mod->name, i, BSPVERSION);
	
// swap all the lumps
	mod_base = (byte *)header;

	for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

// load into heap
	
	Mod_LoadVertexes (&header->lumps[LUMP_VERTEXES]);
	if (isbsp2)
	{
		Mod_LoadBSP2Edges (&header->lumps[LUMP_EDGES]);
	}
	else
	{
		Mod_LoadEdges (&header->lumps[LUMP_EDGES]);
	}
	Mod_LoadSurfedges (&header->lumps[LUMP_SURFEDGES]);
	Mod_LoadTextures (&header->lumps[LUMP_TEXTURES]);
	Mod_LoadLighting (&header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes (&header->lumps[LUMP_PLANES]);
	Mod_LoadTexinfo (&header->lumps[LUMP_TEXINFO]);
	if (isbsp2)
	{
		Mod_LoadBSP2Faces (&header->lumps[LUMP_FACES]);
		Mod_LoadBSP2Marksurfaces (&header->lumps[LUMP_MARKSURFACES]);
	}
	else
	{
		Mod_LoadFaces (&header->lumps[LUMP_FACES]);
		Mod_LoadMarksurfaces (&header->lumps[LUMP_MARKSURFACES]);
	}
	Mod_LoadVisibility (&header->lumps[LUMP_VISIBILITY]);
	if (isbsp2)
	{
		Mod_LoadBSP2Leafs (&header->lumps[LUMP_LEAFS]);
		Mod_LoadBSP2Nodes (&header->lumps[LUMP_NODES]);
		Mod_LoadBSP2Clipnodes (&header->lumps[LUMP_CLIPNODES]);
	}
	else
	{
		Mod_LoadLeafs (&header->lumps[LUMP_LEAFS]);
		Mod_LoadNodes (&header->lumps[LUMP_NODES]);
		Mod_LoadClipnodes (&header->lumps[LUMP_CLIPNODES]);
	}
	Mod_LoadEntities (&header->lumps[LUMP_ENTITIES]);
	Mod_LoadSubmodels (&header->lumps[LUMP_MODELS]);

	Mod_MakeHull0 ();
	
	mod->numframes = 2;		// regular and alternate animation
	mod->flags = 0;
	
//
// set up the submodels (FIXME: this is confusing)
//
	for (i=0 ; i<mod->numsubmodels ; i++)
	{
		bm = &mod->submodels[i];

		mod->hulls[0].firstclipnode = bm->headnode[0];
		for (j=1 ; j<MAX_MAP_HULLS ; j++)
		{
			mod->hulls[j].firstclipnode = bm->headnode[j];
			mod->hulls[j].lastclipnode = mod->numclipnodes-1;
		}
		
		mod->firstmodelsurface = bm->firstface;
		mod->nummodelsurfaces = bm->numfaces;
		
		VectorCopy (bm->maxs, mod->maxs);
		VectorCopy (bm->mins, mod->mins);
		mod->radius = RadiusFromBounds (mod->mins, mod->maxs);
		
		mod->numleafs = bm->visleafs;

		if (i < mod->numsubmodels-1)
		{	// duplicate the basic information
			char	name[10];

			sprintf (name, "*%i", i+1);
			loadmodel = Mod_FindName (name);
			*loadmodel = *mod;
			auto name_len = (int)strlen(name);
			loadmodel->name = ED_NewString(name_len + 1);
			Q_strncpy(pr_strings + loadmodel->name, name, name_len);
			mod = loadmodel;
		}
	}
}

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

/*
=================
Mod_LoadAliasFrame
=================
*/
void * Mod_LoadAliasFrame (void * pin, int *pframeindex, int numv,
	trivertx_t *pbboxmin, trivertx_t *pbboxmax, aliashdr_t *pheader, char *name)
{
	trivertx_t		*pframe, *pinframe;
	int				i, j;
	daliasframe_t	*pdaliasframe;

	pdaliasframe = (daliasframe_t *)pin;

	Q_memcpy (name, pdaliasframe->name, 16);
	Q_memset (name + 16, 0, 4);

	for (i=0 ; i<3 ; i++)
	{
	// these are byte values, so we don't have to worry about
	// endianness
		pbboxmin->v[i] = pdaliasframe->bboxmin.v[i];
		pbboxmax->v[i] = pdaliasframe->bboxmax.v[i];
	}

	pinframe = (trivertx_t *)(pdaliasframe + 1);
	
	int pframeindexoffset = (byte*)pframeindex - mod_aliaspool.data();
	int pooltop = mod_aliaspool.size();
	mod_aliaspool.resize(pooltop + numv * sizeof(*pframe));
	pheader = (aliashdr_t*)mod_aliaspool.data();
	pframe = (trivertx_t*)(mod_aliaspool.data() + pooltop);

	pframeindex = (int*)(mod_aliaspool.data() + pframeindexoffset);
	*pframeindex = (byte *)pframe - (byte *)pheader;

	for (j=0 ; j<numv ; j++)
	{
		int		k;

	// these are all byte values, so no need to deal with endianness
		pframe[j].lightnormalindex = pinframe[j].lightnormalindex;

		for (k=0 ; k<3 ; k++)
		{
			pframe[j].v[k] = pinframe[j].v[k];
		}
	}

	pinframe += numv;

	return (void *)pinframe;
}


/*
=================
Mod_LoadAliasGroup
=================
*/
void * Mod_LoadAliasGroup (void * pin, int *pframeindex, int numv,
	trivertx_t *pbboxmin, trivertx_t *pbboxmax, aliashdr_t *pheader, char *name)
{
	daliasgroup_t		*pingroup;
	maliasgroup_t		*paliasgroup;
	int					i, numframes;
	daliasinterval_t	*pin_intervals;
	float				*poutintervals;
	void				*ptemp;
	
	pingroup = (daliasgroup_t *)pin;

	numframes = LittleLong (pingroup->numframes);

	int pbboxminoffset = (byte*)pbboxmin - mod_aliaspool.data();
	int pbboxmaxoffset = (byte*)pbboxmax - mod_aliaspool.data();
	int pframeindexoffset = (byte*)pframeindex - mod_aliaspool.data();
	int pooltop = mod_aliaspool.size();
	mod_aliaspool.resize(pooltop + sizeof(maliasgroup_t) + (numframes - 1) * sizeof(paliasgroup->frames[0]) + numframes * sizeof(float));
	pheader = (aliashdr_t*)mod_aliaspool.data();
	paliasgroup = (maliasgroup_t*)(mod_aliaspool.data() + pooltop);

	paliasgroup->numframes = numframes;

	pbboxmin = (trivertx_t*)(mod_aliaspool.data() + pbboxminoffset);
	pbboxmax = (trivertx_t*)(mod_aliaspool.data() + pbboxmaxoffset);
	for (i=0 ; i<3 ; i++)
	{
	// these are byte values, so we don't have to worry about endianness
		pbboxmin->v[i] = pingroup->bboxmin.v[i];
		pbboxmax->v[i] = pingroup->bboxmax.v[i];
	}

	pframeindex = (int*)(mod_aliaspool.data() + pframeindexoffset);
	*pframeindex = (byte *)paliasgroup - (byte *)pheader;

	pin_intervals = (daliasinterval_t *)(pingroup + 1);

	poutintervals = (float*)(mod_aliaspool.data() + pooltop + sizeof(maliasgroup_t) + (numframes - 1) * sizeof(paliasgroup->frames[0]));

	paliasgroup->intervals = (byte *)poutintervals - (byte *)pheader;

	qboolean increasing = true;
	float firstinterval = 0;
	float previnterval = 0;
	for (i=0 ; i<numframes ; i++)
	{
		*poutintervals = LittleFloat (pin_intervals->interval);

		if (i == 0)
		{
			firstinterval = *poutintervals;
		}

		if (increasing && previnterval >= *poutintervals)
		{
			increasing = false;
		}
		previnterval = *poutintervals;

		poutintervals++;
		pin_intervals++;
	}

	if (!increasing)
	{
		if (firstinterval > 0)
		{
			Con_Printf("Mod_LoadAliasGroup: intervals do not increase - rebuilding with first interval\n");

			poutintervals = (float*)(mod_aliaspool.data() + pooltop + sizeof(maliasgroup_t) + (numframes - 1) * sizeof(paliasgroup->frames[0]));

			auto interval = firstinterval;

			for (i=0 ; i<numframes ; i++)
			{
				*poutintervals++ = interval;
				interval += firstinterval;
			}
		}
		else
		{
			Sys_Error ("Mod_LoadAliasGroup: interval<=0");
		}
	}

	ptemp = (void *)pin_intervals;

	for (i=0 ; i<numframes ; i++)
	{
		ptemp = Mod_LoadAliasFrame (ptemp,
									&paliasgroup->frames[i].frame,
									numv,
									&paliasgroup->frames[i].bboxmin,
									&paliasgroup->frames[i].bboxmax,
									pheader, name);
	}

	return ptemp;
}


/*
=================
Mod_LoadAliasSkin
=================
*/
void * Mod_LoadAliasSkin (void * pin, int *pskinindex, int skinsize,
	aliashdr_t *pheader)
{
	int		i;
	byte	*pskin, *pinskin;
	unsigned short	*pusskin;

	int pskinindexoffset = (byte*)pskinindex - mod_aliaspool.data();
	int pooltop = mod_aliaspool.size();
	mod_aliaspool.resize(pooltop + skinsize * r_pixbytes);
	pheader = (aliashdr_t*)mod_aliaspool.data();
	pskin = mod_aliaspool.data() + pooltop;
	pinskin = (byte *)pin;
	pskinindex = (int*)(mod_aliaspool.data() + pskinindexoffset);
	*pskinindex = (byte *)pskin - (byte *)pheader;

	if (r_pixbytes == 1)
	{
		Q_memcpy (pskin, pinskin, skinsize);
	}
	else if (r_pixbytes == 2)
	{
		pusskin = (unsigned short *)pskin;

		for (i=0 ; i<skinsize ; i++)
			pusskin[i] = d_8to16table[pinskin[i]];
	}
	else
	{
		Sys_Error ("Mod_LoadAliasSkin: driver set invalid r_pixbytes: %d\n",
				 r_pixbytes);
	}

	pinskin += skinsize;

	return ((void *)pinskin);
}


/*
=================
Mod_LoadAliasSkinGroup
=================
*/
void * Mod_LoadAliasSkinGroup (void * pin, int *pskinindex, int skinsize,
	aliashdr_t *pheader)
{
	daliasskingroup_t		*pinskingroup;
	maliasskingroup_t		*paliasskingroup;
	int						i, numskins;
	daliasskininterval_t	*pinskinintervals;
	float					*poutskinintervals;
	void					*ptemp;

	pinskingroup = (daliasskingroup_t *)pin;

	numskins = LittleLong (pinskingroup->numskins);
	int pskinindexoffset = (byte*)pskinindex - mod_aliaspool.data();
	int pooltop = mod_aliaspool.size();
	mod_aliaspool.resize(pooltop + sizeof(maliasskingroup_t) + (numskins - 1) * sizeof(paliasskingroup->skindescs[0]) + numskins * sizeof(float));
	pheader = (aliashdr_t*)mod_aliaspool.data();
	paliasskingroup = (maliasskingroup_t*)(mod_aliaspool.data() + pooltop);

	paliasskingroup->numskins = numskins;

	pskinindex = (int*)(mod_aliaspool.data() + pskinindexoffset);
	*pskinindex = (byte *)paliasskingroup - (byte *)pheader;

	pinskinintervals = (daliasskininterval_t *)(pinskingroup + 1);

	poutskinintervals = (float*)(mod_aliaspool.data() + pooltop + sizeof(maliasskingroup_t) + (numskins - 1) * sizeof(paliasskingroup->skindescs[0]));

	paliasskingroup->intervals = (byte *)poutskinintervals - (byte *)pheader;

	qboolean increasing = true;
	float prevskininterval = 0;
	for (i=0 ; i<numskins ; i++)
	{
		*poutskinintervals = LittleFloat (pinskinintervals->interval);

		if (increasing && prevskininterval >= *poutskinintervals)
		{
			increasing = false;
		}
		prevskininterval = *poutskinintervals;

		poutskinintervals++;
		pinskinintervals++;
	}

	if (!increasing)
	{
		Con_Printf("Mod_LoadAliasGroup: skin intervals do not increase - rebuilding with 0.1 second intervals\n");

		poutskinintervals = (float*)(mod_aliaspool.data() + pooltop + sizeof(maliasskingroup_t) + (numskins - 1) * sizeof(paliasskingroup->skindescs[0]));

		auto interval = 0.1;

		for (i=0 ; i<numskins ; i++)
		{
			*poutskinintervals++ = interval;
			interval += 0.1;
		}
	}

	ptemp = (void *)pinskinintervals;

	for (i=0 ; i<numskins ; i++)
	{
		ptemp = Mod_LoadAliasSkin (ptemp,
				&paliasskingroup->skindescs[i].skin, skinsize, pheader);
		paliasskingroup = (maliasskingroup_t*)(mod_aliaspool.data() + pooltop);
	}

	return ptemp;
}


/*
=================
Mod_LoadAliasModel
=================
*/
void Mod_LoadAliasModel (model_t *mod, void *buffer)
{
	int					i;
	mdl_t				*pmodel, *pinmodel;
	stvert_t			*pstverts, *pinstverts;
	aliashdr_t			*pheader;
	mtriangle_t			*ptri;
	dtriangle_t			*pintriangles;
	int					version, numframes, numskins;
	int					size;
	daliasframetype_t	*pframetype;
	daliasskintype_t	*pskintype;
	maliasskindesc_t	*pskindesc;
	int					skinsize;
	int					total;
	
	pinmodel = (mdl_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != ALIAS_VERSION)
		Sys_Error ("%s has wrong version number (%i should be %i)",
				 pr_strings + mod->name, version, ALIAS_VERSION);

//
// allocate space for a working header, plus all the data except the frames,
// skin and group info
//
	size = 	sizeof (aliashdr_t) + (LittleLong (pinmodel->numframes) - 1) *
			 sizeof (pheader->frames[0]) +
			sizeof (mdl_t) +
			LittleLong (pinmodel->numverts) * sizeof (stvert_t) +
			LittleLong (pinmodel->numtris) * sizeof (mtriangle_t);

	mod_aliaspool.resize(size);
	pheader = (aliashdr_t*)mod_aliaspool.data();
	pmodel = (mdl_t *) ((byte *)&pheader[1] +
			(LittleLong (pinmodel->numframes) - 1) *
			 sizeof (pheader->frames[0]));
	auto pmodeloffset = (byte*)pmodel - (byte*)pheader;
	
//	mod->cache.data = pheader;
	mod->flags = LittleLong (pinmodel->flags);

//
// endian-adjust and copy the data, starting with the alias model header
//
	pmodel->boundingradius = LittleFloat (pinmodel->boundingradius);
	pmodel->numskins = LittleLong (pinmodel->numskins);
	pmodel->skinwidth = LittleLong (pinmodel->skinwidth);
	pmodel->skinheight = LittleLong (pinmodel->skinheight);

	pmodel->numverts = LittleLong (pinmodel->numverts);

	if (pmodel->numverts <= 0)
		Sys_Error ("model %s has no vertices", pr_strings + mod->name);

	pmodel->numtris = LittleLong (pinmodel->numtris);

	if (pmodel->numtris <= 0)
		Sys_Error ("model %s has no triangles", pr_strings + mod->name);

	pmodel->numframes = LittleLong (pinmodel->numframes);
	pmodel->size = LittleFloat (pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
	mod->synctype = (synctype_t)LittleLong (pinmodel->synctype);
	mod->numframes = pmodel->numframes;

	for (i=0 ; i<3 ; i++)
	{
		pmodel->scale[i] = LittleFloat (pinmodel->scale[i]);
		pmodel->scale_origin[i] = LittleFloat (pinmodel->scale_origin[i]);
		pmodel->eyeposition[i] = LittleFloat (pinmodel->eyeposition[i]);
	}

	numskins = pmodel->numskins;
	numframes = pmodel->numframes;

	if (pmodel->skinwidth & 0x03)
		Sys_Error ("Mod_LoadAliasModel: skinwidth not multiple of 4");

	pheader->model = (byte *)pmodel - (byte *)pheader;

//
// load the skins
//
	skinsize = pmodel->skinheight * pmodel->skinwidth;

	if (numskins < 1)
		Sys_Error ("Mod_LoadAliasModel: Invalid # of skins: %d\n", numskins);

	pskintype = (daliasskintype_t *)&pinmodel[1];

	int pooltop = mod_aliaspool.size();
	mod_aliaspool.resize(pooltop + numskins * sizeof(maliasskindesc_t));
	pheader = (aliashdr_t*)mod_aliaspool.data();
	pskindesc = (maliasskindesc_t*)(mod_aliaspool.data() + pooltop);

	pheader->skindesc = (byte *)pskindesc - (byte *)pheader;

	for (i=0 ; i<numskins ; i++)
	{
		aliasskintype_t	skintype;

		skintype = (aliasskintype_t)LittleLong (pskintype->type);
		pskindesc[i].type = skintype;

		if (skintype == ALIAS_SKIN_SINGLE)
		{
			pskintype = (daliasskintype_t *)
					Mod_LoadAliasSkin (pskintype + 1,
									   &pskindesc[i].skin,
									   skinsize, pheader);
		}
		else
		{
			pskintype = (daliasskintype_t *)
					Mod_LoadAliasSkinGroup (pskintype + 1,
											&pskindesc[i].skin,
											skinsize, pheader);
		}
		pheader = (aliashdr_t*)mod_aliaspool.data();
		pskindesc = (maliasskindesc_t*)(mod_aliaspool.data() + pooltop);
	}

//
// set base s and t vertices
//
	pmodel = (mdl_t *)(mod_aliaspool.data() + pmodeloffset);
	pstverts = (stvert_t *)&pmodel[1];
	pinstverts = (stvert_t *)pskintype;

	pheader->stverts = (byte *)pstverts - (byte *)pheader;

	for (i=0 ; i<pmodel->numverts ; i++)
	{
		pstverts[i].onseam = LittleLong (pinstverts[i].onseam);
	// put s and t in 16.16 format
		pstverts[i].s = LittleLong (pinstverts[i].s) << 16;
		pstverts[i].t = LittleLong (pinstverts[i].t) << 16;
	}

//
// set up the triangles
//
	ptri = (mtriangle_t *)&pstverts[pmodel->numverts];
	pintriangles = (dtriangle_t *)&pinstverts[pmodel->numverts];

	pheader->triangles = (byte *)ptri - (byte *)pheader;

	for (i=0 ; i<pmodel->numtris ; i++)
	{
		int		j;

		ptri[i].facesfront = LittleLong (pintriangles[i].facesfront);

		for (j=0 ; j<3 ; j++)
		{
			ptri[i].vertindex[j] =
					LittleLong (pintriangles[i].vertindex[j]);
		}
	}

//
// load the frames
//
	if (numframes < 1)
		Sys_Error ("Mod_LoadAliasModel: Invalid # of frames: %d\n", numframes);

	pframetype = (daliasframetype_t *)&pintriangles[pmodel->numtris];

	for (i=0 ; i<numframes ; i++)
	{
		aliasframetype_t	frametype;

		frametype = (aliasframetype_t)LittleLong (pframetype->type);
		pheader->frames[i].type = frametype;

		if (frametype == ALIAS_SINGLE)
		{
			pframetype = (daliasframetype_t *)
					Mod_LoadAliasFrame (pframetype + 1,
										&pheader->frames[i].frame,
										pmodel->numverts,
										&pheader->frames[i].bboxmin,
										&pheader->frames[i].bboxmax,
										pheader, pheader->frames[i].name);
		}
		else
		{
			pframetype = (daliasframetype_t *)
					Mod_LoadAliasGroup (pframetype + 1,
										&pheader->frames[i].frame,
										pmodel->numverts,
										&pheader->frames[i].bboxmin,
										&pheader->frames[i].bboxmax,
										pheader, pheader->frames[i].name);
		}
		pheader = (aliashdr_t*)mod_aliaspool.data();
		pmodel = (mdl_t *)(mod_aliaspool.data() + pmodeloffset);
	}

	mod->type = mod_alias;

// FIXME: do this right
	mod->mins[0] = mod->mins[1] = mod->mins[2] = -16;
	mod->maxs[0] = mod->maxs[1] = mod->maxs[2] = 16;

//
// move the complete, relocatable alias model to the cache
//	
	total = mod_aliaspool.size();
	
	mod->extradata = new byte[total];
	memcpy (mod->extradata, pheader, total);

	mod_aliaspool.clear();
}

//=============================================================================

/*
=================
Mod_LoadSpriteFrame
=================
*/
void * Mod_LoadSpriteFrame (void * pin, mspriteframe_t **ppframe)
{
	dspriteframe_t		*pinframe;
	mspriteframe_t		*pspriteframe;
	int					i, width, height, size, origin[2];
	unsigned short		*ppixout;
	byte				*ppixin;

	pinframe = (dspriteframe_t *)pin;

	width = LittleLong (pinframe->width);
	height = LittleLong (pinframe->height);
	size = width * height;
	
	int ppframeoffset = (byte*)ppframe - mod_pool.sprites.back().data();
	int pooltop = mod_pool.sprites.back().size();
	mod_pool.sprites.back().resize(pooltop + sizeof (mspriteframe_t) + size*r_pixbytes);
	pspriteframe = (mspriteframe_t*)(mod_pool.sprites.back().data() + pooltop);
	ppframe = (mspriteframe_t**)(mod_pool.sprites.back().data() + ppframeoffset);

	Q_memset (pspriteframe, 0, sizeof (mspriteframe_t) + size);
	*ppframe = pspriteframe;

	pspriteframe->width = width;
	pspriteframe->height = height;
	origin[0] = LittleLong (pinframe->origin[0]);
	origin[1] = LittleLong (pinframe->origin[1]);

	pspriteframe->up = origin[1];
	pspriteframe->down = origin[1] - height;
	pspriteframe->left = origin[0];
	pspriteframe->right = width + origin[0];

	if (r_pixbytes == 1)
	{
		Q_memcpy (&pspriteframe->pixels[0], (byte *)(pinframe + 1), size);
	}
	else if (r_pixbytes == 2)
	{
		ppixin = (byte *)(pinframe + 1);
		ppixout = (unsigned short *)&pspriteframe->pixels[0];

		for (i=0 ; i<size ; i++)
			ppixout[i] = d_8to16table[ppixin[i]];
	}
	else
	{
		Sys_Error ("Mod_LoadSpriteFrame: driver set invalid r_pixbytes: %d\n",
				 r_pixbytes);
	}

	return (void *)((byte *)pinframe + sizeof (dspriteframe_t) + size);
}


/*
=================
Mod_LoadSpriteGroup
=================
*/
void * Mod_LoadSpriteGroup (void * pin, mspriteframe_t **ppframe)
{
	dspritegroup_t		*pingroup;
	mspritegroup_t		*pspritegroup;
	int					i, numframes;
	dspriteinterval_t	*pin_intervals;
	float				*poutintervals;
	void				*ptemp;

	pingroup = (dspritegroup_t *)pin;

	numframes = LittleLong (pingroup->numframes);

	int ppframeoffset = (byte*)ppframe - mod_pool.sprites.back().data();
	int pooltop = mod_pool.sprites.back().size();
	mod_pool.sprites.back().resize(pooltop + sizeof(mspritegroup_t) + (numframes - 1) * sizeof(pspritegroup->frames[0]) + numframes * sizeof (float));
	pspritegroup = (mspritegroup_t*)(mod_pool.sprites.back().data() + pooltop);
	ppframe = (mspriteframe_t**)(mod_pool.sprites.back().data() + ppframeoffset);

	pspritegroup->numframes = numframes;

	*ppframe = (mspriteframe_t *)pspritegroup;

	pin_intervals = (dspriteinterval_t *)(pingroup + 1);

	poutintervals = (float*)(mod_pool.sprites.back().data() + pooltop + sizeof(mspritegroup_t) + (numframes - 1) * sizeof(pspritegroup->frames[0]));

	pspritegroup->intervals = poutintervals;

	for (i=0 ; i<numframes ; i++)
	{
		*poutintervals = LittleFloat (pin_intervals->interval);
		if (*poutintervals <= 0.0)
			Sys_Error ("Mod_LoadSpriteGroup: interval<=0");

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (void *)pin_intervals;

	for (i=0 ; i<numframes ; i++)
	{
		ptemp = Mod_LoadSpriteFrame (ptemp, &pspritegroup->frames[i]);
		pspritegroup = (mspritegroup_t*)(mod_pool.sprites.back().data() + pooltop);
	}

	return ptemp;
}


/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel (model_t *mod, void *buffer)
{
	int					i;
	int					version;
	dsprite_t			*pin;
	msprite_t			*psprite;
	int					numframes;
	int					size;
	dspriteframetype_t	*pframetype;
	
	pin = (dsprite_t *)buffer;

	version = LittleLong (pin->version);
	if (version != SPRITE_VERSION)
		Sys_Error ("%s has wrong version number "
				 "(%i should be %i)", pr_strings + mod->name, version, SPRITE_VERSION);

	numframes = LittleLong (pin->numframes);

	size = sizeof (msprite_t) +	(numframes - 1) * sizeof (psprite->frames);
	mod_pool.sprites.emplace_back(size);
	psprite = (msprite_t*)(mod_pool.sprites.back().data());

	psprite->type = LittleLong (pin->type);
	psprite->maxwidth = LittleLong (pin->width);
	psprite->maxheight = LittleLong (pin->height);
	psprite->beamlength = LittleFloat (pin->beamlength);
	mod->synctype = (synctype_t)LittleLong (pin->synctype);
	psprite->numframes = numframes;

	mod->mins[0] = mod->mins[1] = -psprite->maxwidth/2;
	mod->maxs[0] = mod->maxs[1] = psprite->maxwidth/2;
	mod->mins[2] = -psprite->maxheight/2;
	mod->maxs[2] = psprite->maxheight/2;
	
//
// load the frames
//
	if (numframes < 1)
		Sys_Error ("Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes);

	mod->numframes = numframes;
	mod->flags = 0;

	pframetype = (dspriteframetype_t *)(pin + 1);

	std::vector<int> frameptroffsets;
	for (i=0 ; i<numframes ; i++)
	{
		spriteframetype_t	frametype;

		frametype = (spriteframetype_t)LittleLong (pframetype->type);
		psprite->frames[i].type = frametype;

		if (frametype == SPR_SINGLE)
		{
			pframetype = (dspriteframetype_t *)
					Mod_LoadSpriteFrame (pframetype + 1,
										 &psprite->frames[i].frameptr);
		}
		else
		{
			pframetype = (dspriteframetype_t *)
					Mod_LoadSpriteGroup (pframetype + 1,
										 &psprite->frames[i].frameptr);
		}
		psprite = (msprite_t*)(mod_pool.sprites.back().data());
		frameptroffsets.push_back((byte*)psprite->frames[i].frameptr - mod_pool.sprites.back().data());
	}
	
	for (i = 0; i < frameptroffsets.size(); i++)
	{
		psprite->frames[i].frameptr = (mspriteframe_t*)(mod_pool.sprites.back().data() + frameptroffsets[i]);
	}

	mod->extradata = (byte*)psprite;
	
	mod->type = mod_sprite;
}

//=============================================================================

/*
================
Mod_Print
================
*/
void Mod_Print (void)
{
	Con_Printf ("Cached models:\n");
	for (auto& mod : mod_known)
	{
		Con_Printf ("%8p : %s",mod.extradata, pr_strings + mod.name);
		if (mod.needload & NL_UNREFERENCED)
			Con_Printf (" (!R)");
		if (mod.needload & NL_NEEDS_LOADED)
			Con_Printf (" (!P)");
		Con_Printf ("\n");
	}
}


