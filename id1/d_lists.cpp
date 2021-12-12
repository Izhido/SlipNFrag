#include "quakedef.h"
#include "d_lists.h"
#include "r_shared.h"
#include "r_local.h"
#include "d_local.h"

#define UPPER_16BIT_LIMIT 65520

dlists_t d_lists { -1, -1, -1, -1, -1, -1, -1, -1, -1,-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

qboolean d_uselists = false;
qboolean d_awayfromviewmodel = false;

extern int r_ambientlight;
extern float r_shadelight;
#define NUMVERTEXNORMALS 162
extern float r_avertexnormals[NUMVERTEXNORMALS][3];
extern vec3_t r_plightvec;

void D_ResetLists ()
{
	d_lists.last_surface16 = -1;
	d_lists.last_surface32 = -1;
	d_lists.last_surface_rotated16 = -1;
	d_lists.last_surface_rotated32 = -1;
	d_lists.last_fence16 = -1;
	d_lists.last_fence32 = -1;
	d_lists.last_fence_rotated16 = -1;
	d_lists.last_fence_rotated32 = -1;
	d_lists.last_sprite = -1;
	d_lists.last_turbulent16 = -1;
	d_lists.last_turbulent32 = -1;
	d_lists.last_turbulent_rotated16 = -1;
	d_lists.last_turbulent_rotated32 = -1;
	d_lists.last_alias16 = -1;
	d_lists.last_alias32 = -1;
	d_lists.last_viewmodel16 = -1;
	d_lists.last_viewmodel32 = -1;
	d_lists.last_sky = -1;
    d_lists.last_skybox = -1;
	d_lists.last_textured_vertex = -1;
	d_lists.last_textured_attribute = -1;
	d_lists.last_colormapped_attribute = -1;
	d_lists.last_colored_vertex = -1;
	d_lists.last_colored_attribute = -1;
	d_lists.last_colored_index16 = -1;
	d_lists.last_colored_index32 = -1;
	d_lists.last_lightmap_texel = -1;
	d_lists.clear_color = -1;
}

void D_FillSurfaceData (dsurface_t& surface, msurface_t* face, surfcache_t* cache, entity_t* entity, int created)
{
	surface.surface = face;
	surface.entity = entity;
	surface.model = entity->model;
	surface.created = created;
	auto texture = (texture_t*)(cache->texture);
	surface.texture_width = texture->width;
	surface.texture_height = texture->height;
	surface.texture_size = surface.texture_width * surface.texture_height;
	surface.texture = (unsigned char *)(texture + 1);
	surface.lightmap_width = cache->width / sizeof(unsigned);
	surface.lightmap_height = cache->height;
	surface.lightmap_size = surface.lightmap_width * surface.lightmap_height;
	if (d_lists.last_lightmap_texel + surface.lightmap_size >= d_lists.lightmap_texels.size())
	{
		d_lists.lightmap_texels.resize(d_lists.lightmap_texels.size() + 64 * 1024);
	}
	surface.lightmap_texels = d_lists.last_lightmap_texel + 1;
	d_lists.last_lightmap_texel += surface.lightmap_size;
	memcpy(d_lists.lightmap_texels.data() + surface.lightmap_texels, &cache->data[0], surface.lightmap_size * sizeof(unsigned));
	surface.count = face->numedges;
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
}

void D_FillSurfaceRotatedData (dsurfacerotated_t& surface, msurface_t* face, surfcache_t* cache, entity_t* entity, int created)
{
	surface.surface = face;
	surface.entity = entity;
	surface.model = entity->model;
	surface.created = created;
	auto texture = (texture_t*)(cache->texture);
	surface.texture_width = texture->width;
	surface.texture_height = texture->height;
	surface.texture_size = surface.texture_width * surface.texture_height;
	surface.texture = (unsigned char *)(texture + 1);
	surface.lightmap_width = cache->width / sizeof(unsigned);
	surface.lightmap_height = cache->height;
	surface.lightmap_size = surface.lightmap_width * surface.lightmap_height;
	if (d_lists.last_lightmap_texel + surface.lightmap_size >= d_lists.lightmap_texels.size())
	{
		d_lists.lightmap_texels.resize(d_lists.lightmap_texels.size() + 64 * 1024);
	}
	surface.lightmap_texels = d_lists.last_lightmap_texel + 1;
	d_lists.last_lightmap_texel += surface.lightmap_size;
	memcpy(d_lists.lightmap_texels.data() + surface.lightmap_texels, &cache->data[0], surface.lightmap_size * sizeof(unsigned));
	surface.count = face->numedges;
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	surface.yaw = entity->angles[YAW];
	surface.pitch = entity->angles[PITCH];
	surface.roll = entity->angles[ROLL];
}

void D_AddSurfaceToLists (msurface_t* face, surfcache_t* cache, entity_t* entity, int created)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	if (entity->model->numvertexes <= UPPER_16BIT_LIMIT)
	{
		d_lists.last_surface16++;
		if (d_lists.last_surface16 >= d_lists.surfaces16.size())
		{
			d_lists.surfaces16.emplace_back();
		}
		auto& surface = d_lists.surfaces16[d_lists.last_surface16];
		D_FillSurfaceData(surface, face, cache, entity, created);
		return;
	}
	d_lists.last_surface32++;
	if (d_lists.last_surface32 >= d_lists.surfaces32.size())
	{
		d_lists.surfaces32.emplace_back();
	}
	auto& surface = d_lists.surfaces32[d_lists.last_surface32];
	D_FillSurfaceData(surface, face, cache, entity, created);
}

void D_AddSurfaceRotatedToLists (msurface_t* face, surfcache_t* cache, entity_t* entity, int created)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	if (entity->model->numvertexes <= UPPER_16BIT_LIMIT)
	{
		d_lists.last_surface_rotated16++;
		if (d_lists.last_surface_rotated16 >= d_lists.surfaces_rotated16.size())
		{
			d_lists.surfaces_rotated16.emplace_back();
		}
		auto& surface = d_lists.surfaces_rotated16[d_lists.last_surface_rotated16];
		D_FillSurfaceRotatedData(surface, face, cache, entity, created);
		return;
	}
	d_lists.last_surface_rotated32++;
	if (d_lists.last_surface_rotated32 >= d_lists.surfaces_rotated32.size())
	{
		d_lists.surfaces_rotated32.emplace_back();
	}
	auto& surface = d_lists.surfaces_rotated32[d_lists.last_surface_rotated32];
	D_FillSurfaceRotatedData(surface, face, cache, entity, created);
}

void D_AddFenceToLists (msurface_t* face, surfcache_t* cache, entity_t* entity, int created)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	if (entity->model->numvertexes <= UPPER_16BIT_LIMIT)
	{
		d_lists.last_fence16++;
		if (d_lists.last_fence16 >= d_lists.fences16.size())
		{
			d_lists.fences16.emplace_back();
		}
		auto& fence = d_lists.fences16[d_lists.last_fence16];
		D_FillSurfaceData(fence, face, cache, entity, created);
		return;
	}
	d_lists.last_fence32++;
	if (d_lists.last_fence32 >= d_lists.fences32.size())
	{
		d_lists.fences32.emplace_back();
	}
	auto& fence = d_lists.fences32[d_lists.last_fence32];
	D_FillSurfaceData(fence, face, cache, entity, created);
}

void D_AddFenceRotatedToLists (msurface_t* face, surfcache_t* cache, entity_t* entity, int created)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	if (entity->model->numvertexes <= UPPER_16BIT_LIMIT)
	{
		d_lists.last_fence_rotated16++;
		if (d_lists.last_fence_rotated16 >= d_lists.fences_rotated16.size())
		{
			d_lists.fences_rotated16.emplace_back();
		}
		auto& fence = d_lists.fences_rotated16[d_lists.last_fence_rotated16];
		D_FillSurfaceRotatedData(fence, face, cache, entity, created);
		return;
	}
	d_lists.last_fence_rotated32++;
	if (d_lists.last_fence_rotated32 >= d_lists.fences_rotated32.size())
	{
		d_lists.fences_rotated32.emplace_back();
	}
	auto& fence = d_lists.fences_rotated32[d_lists.last_fence_rotated32];
	D_FillSurfaceRotatedData(fence, face, cache, entity, created);
}

void D_AddSpriteToLists (vec5_t* pverts, spritedesc_t* spritedesc)
{
	d_lists.last_sprite++;
	if (d_lists.last_sprite >= d_lists.sprites.size())
	{
		d_lists.sprites.emplace_back();
	}
	auto& sprite = d_lists.sprites[d_lists.last_sprite];
	sprite.width = spritedesc->pspriteframe->width;
	sprite.height = spritedesc->pspriteframe->height;
	sprite.size = sprite.width * sprite.height;
	sprite.data = &spritedesc->pspriteframe->pixels[0];
	sprite.first_vertex = (d_lists.last_textured_vertex + 1) / 3;
	sprite.count = 4;
	auto new_size = d_lists.last_textured_vertex + 1 + 3 * 4;
	if (d_lists.textured_vertices.size() < new_size)
	{
		d_lists.textured_vertices.resize(new_size);
	}
	new_size = d_lists.last_textured_attribute + 1 + 2 * 4;
	if (d_lists.textured_attributes.size() < new_size)
	{
		d_lists.textured_attributes.resize(new_size);
	}
	auto x = pverts[0][0];
	auto y = pverts[0][1];
	auto z = pverts[0][2];
	auto s = pverts[0][3] / sprite.width;
	auto t = pverts[0][4] / sprite.height;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = x;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = z;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = -y;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = s;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = t;
	x = pverts[1][0];
	y = pverts[1][1];
	z = pverts[1][2];
	s = pverts[1][3] / sprite.width;
	t = pverts[1][4] / sprite.height;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = x;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = z;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = -y;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = s;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = t;
	x = pverts[3][0];
	y = pverts[3][1];
	z = pverts[3][2];
	s = pverts[3][3] / sprite.width;
	t = pverts[3][4] / sprite.height;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = x;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = z;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = -y;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = s;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = t;
	x = pverts[2][0];
	y = pverts[2][1];
	z = pverts[2][2];
	s = pverts[2][3] / sprite.width;
	t = pverts[2][4] / sprite.height;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = x;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = z;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = -y;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = s;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = t;
}

void D_FillTurbulentData (dturbulent_t& turbulent, msurface_t* face, entity_t* entity)
{
	auto texinfo = face->texinfo;
	auto texture = texinfo->texture;
	turbulent.surface = face;
	turbulent.entity = entity;
	turbulent.model = entity->model;
	turbulent.texture = texture;
	turbulent.width = texture->width;
	turbulent.height = texture->height;
	turbulent.size = turbulent.width * turbulent.height;
	turbulent.data = (unsigned char*)texture + texture->offsets[0];
	turbulent.count = face->numedges;
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
}

void D_FillTurbulentRotatedData (dturbulentrotated_t& turbulent, msurface_t* face, entity_t* entity)
{
	auto texinfo = face->texinfo;
	auto texture = texinfo->texture;
	turbulent.surface = face;
	turbulent.entity = entity;
	turbulent.model = entity->model;
	turbulent.texture = texture;
	turbulent.width = texture->width;
	turbulent.height = texture->height;
	turbulent.size = turbulent.width * turbulent.height;
	turbulent.data = (unsigned char*)texture + texture->offsets[0];
	turbulent.count = face->numedges;
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	turbulent.yaw = entity->angles[YAW];
	turbulent.pitch = entity->angles[PITCH];
	turbulent.roll = entity->angles[ROLL];
}

void D_AddTurbulentToLists (msurface_t* face, entity_t* entity)
{
	if (face->numedges < 3)
	{
		return;
	}
	if (entity->model->numvertexes <= UPPER_16BIT_LIMIT)
	{
		d_lists.last_turbulent16++;
		if (d_lists.last_turbulent16 >= d_lists.turbulent16.size())
		{
			d_lists.turbulent16.emplace_back();
		}
		auto& turbulent = d_lists.turbulent16[d_lists.last_turbulent16];
		D_FillTurbulentData(turbulent, face, entity);
		return;
	}
	d_lists.last_turbulent32++;
	if (d_lists.last_turbulent32 >= d_lists.turbulent32.size())
	{
		d_lists.turbulent32.emplace_back();
	}
	auto& turbulent = d_lists.turbulent32[d_lists.last_turbulent32];
	D_FillTurbulentData(turbulent, face, entity);
}

void D_AddTurbulentRotatedToLists (msurface_t* face, entity_t* entity)
{
	if (face->numedges < 3)
	{
		return;
	}
	if (entity->model->numvertexes <= UPPER_16BIT_LIMIT)
	{
		d_lists.last_turbulent_rotated16++;
		if (d_lists.last_turbulent_rotated16 >= d_lists.turbulent_rotated16.size())
		{
			d_lists.turbulent_rotated16.emplace_back();
		}
		auto& turbulent = d_lists.turbulent_rotated16[d_lists.last_turbulent_rotated16];
		D_FillTurbulentRotatedData(turbulent, face, entity);
		return;
	}
	d_lists.last_turbulent_rotated32++;
	if (d_lists.last_turbulent_rotated32 >= d_lists.turbulent_rotated32.size())
	{
		d_lists.turbulent_rotated32.emplace_back();
	}
	auto& turbulent = d_lists.turbulent_rotated32[d_lists.last_turbulent_rotated32];
	D_FillTurbulentRotatedData(turbulent, face, entity);
}

void D_FillAliasData(dalias_t& alias, aliashdr_t* aliashdr, mdl_t* mdl, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* apverts)
{
	alias.aliashdr = aliashdr;
	alias.width = mdl->skinwidth;
	alias.height = mdl->skinheight;
	alias.size = alias.width * alias.height;
	alias.data = (byte *)aliashdr + skindesc->skin;
	if (colormap == host_colormap.data())
	{
		alias.is_host_colormap = true;
	}
	else
	{
		alias.is_host_colormap = false;
		if (alias.colormap.size() < 16384)
		{
			alias.colormap.resize(16384);
		}
		memcpy(alias.colormap.data(), colormap, 16384);
	}
	alias.apverts = apverts;
	alias.texture_coordinates = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);
	alias.vertex_count = mdl->numverts;
	alias.first_attribute = d_lists.last_colormapped_attribute + 1;
	auto new_size = d_lists.last_colormapped_attribute + 1 + 2 * mdl->numverts;
	if (d_lists.colormapped_attributes.size() < new_size)
	{
		d_lists.colormapped_attributes.resize(new_size);
	}
	vec3_t angles;
	angles[ROLL] = currententity->angles[ROLL];
	angles[PITCH] = -currententity->angles[PITCH];
	angles[YAW] = currententity->angles[YAW];
	vec3_t forward, right, up;
	AngleVectors (angles, forward, right, up);
	float tmatrix[3][4] { };
	tmatrix[0][0] = mdl->scale[0];
	tmatrix[1][1] = mdl->scale[1];
	tmatrix[2][2] = mdl->scale[2];
	tmatrix[0][3] = mdl->scale_origin[0];
	tmatrix[1][3] = mdl->scale_origin[1];
	tmatrix[2][3] = mdl->scale_origin[2];
	float t2matrix[3][4] { };
	for (auto i = 0; i < 3; i++)
	{
		t2matrix[i][0] = forward[i];
		t2matrix[i][1] = -right[i];
		t2matrix[i][2] = up[i];
	}
	t2matrix[0][3] = currententity->origin[0];
	t2matrix[1][3] = currententity->origin[1];
	t2matrix[2][3] = currententity->origin[2];
	R_ConcatTransforms (t2matrix, tmatrix, alias.transform);
	auto vertex = apverts;
	auto attribute = d_lists.colormapped_attributes.data() + d_lists.last_colormapped_attribute + 1;
	vec3_t lightvec { r_plightvec[0], r_plightvec[1], r_plightvec[2] };
	for (auto i = 0; i < mdl->numverts; i++)
	{
		// lighting
		float* plightnormal = r_avertexnormals[vertex->lightnormalindex];
		auto lightcos = DotProduct (plightnormal, lightvec);
		auto temp = r_ambientlight;

		if (lightcos < 0)
		{
			temp += (int)(r_shadelight * lightcos);

			// clamp; because we limited the minimum ambient and shading light, we
			// don't have to clamp low light, just bright
			if (temp < 0)
				temp = 0;
		}

		float light = temp / 256;

		*attribute++ = light;
		*attribute++ = light;
		vertex++;
	}
	d_lists.last_colormapped_attribute += mdl->numverts;
	d_lists.last_colormapped_attribute += mdl->numverts;
	alias.count = mdl->numtris * 3;
}

void D_AddAliasToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* apverts)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	if (mdl->numverts * 2 <= UPPER_16BIT_LIMIT)
	{
		d_lists.last_alias16++;
		if (d_lists.last_alias16 >= d_lists.alias16.size())
		{
			d_lists.alias16.emplace_back();
		}
		auto& alias = d_lists.alias16[d_lists.last_alias16];
		D_FillAliasData(alias, aliashdr, mdl, skindesc, colormap, apverts);
		return;
	}
	d_lists.last_alias32++;
	if (d_lists.last_alias32 >= d_lists.alias32.size())
	{
		d_lists.alias32.emplace_back();
	}
	auto& alias = d_lists.alias32[d_lists.last_alias32];
	D_FillAliasData(alias, aliashdr, mdl, skindesc, colormap, apverts);
}

void D_FillViewmodelData (dalias_t& viewmodel, aliashdr_t* aliashdr, mdl_t* mdl, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* apverts)
{
	viewmodel.aliashdr = aliashdr;
	viewmodel.width = mdl->skinwidth;
	viewmodel.height = mdl->skinheight;
	viewmodel.size = viewmodel.width * viewmodel.height;
	viewmodel.data = (byte *)aliashdr + skindesc->skin;
	if (colormap == host_colormap.data())
	{
		viewmodel.is_host_colormap = true;
	}
	else
	{
		viewmodel.is_host_colormap = false;
		if (viewmodel.colormap.size() < 16384)
		{
			viewmodel.colormap.resize(16384);
		}
		memcpy(viewmodel.colormap.data(), colormap, 16384);
	}
	viewmodel.apverts = apverts;
	viewmodel.texture_coordinates = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);
	viewmodel.vertex_count = mdl->numverts;
	viewmodel.first_attribute = d_lists.last_colormapped_attribute + 1;
	auto new_size = d_lists.last_colormapped_attribute + 1 + 2 * mdl->numverts;
	if (d_lists.colormapped_attributes.size() < new_size)
	{
		d_lists.colormapped_attributes.resize(new_size);
	}
	vec3_t angles;
	if (d_awayfromviewmodel)
	{
		angles[ROLL] = 0;
		angles[PITCH] = 0;
		angles[YAW] = 0;
	}
	else
	{
		angles[ROLL] = currententity->angles[ROLL];
		angles[PITCH] = -currententity->angles[PITCH];
		angles[YAW] = currententity->angles[YAW];
	}
	vec3_t forward, right, up;
	AngleVectors (angles, forward, right, up);
	float tmatrix[3][4] { };
	tmatrix[0][0] = mdl->scale[0];
	tmatrix[1][1] = mdl->scale[1];
	tmatrix[2][2] = mdl->scale[2];
	tmatrix[0][3] = mdl->scale_origin[0];
	tmatrix[1][3] = mdl->scale_origin[1];
	tmatrix[2][3] = mdl->scale_origin[2];
	float t2matrix[3][4] { };
	for (auto i = 0; i < 3; i++)
	{
		t2matrix[i][0] = forward[i];
		t2matrix[i][1] = -right[i];
		t2matrix[i][2] = up[i];
	}
	t2matrix[0][3] = currententity->origin[0];
	t2matrix[1][3] = currententity->origin[1];
	t2matrix[2][3] = currententity->origin[2];
	if (d_awayfromviewmodel)
	{
		t2matrix[0][3] -= forward[0] * 8;
		t2matrix[1][3] -= forward[1] * 8;
		t2matrix[2][3] -= forward[2] * 8;
	}
	R_ConcatTransforms (t2matrix, tmatrix, viewmodel.transform);
	auto vertex = apverts;
	auto attribute = d_lists.colormapped_attributes.data() + d_lists.last_colormapped_attribute + 1;
	vec3_t lightvec { r_plightvec[0], r_plightvec[1], r_plightvec[2] };
	for (auto i = 0; i < mdl->numverts; i++)
	{
		// lighting
		float light;
		if (d_awayfromviewmodel)
		{
			light = 0;
		}
		else
		{
			float* plightnormal = r_avertexnormals[vertex->lightnormalindex];
			auto lightcos = DotProduct (plightnormal, lightvec);
			auto temp = r_ambientlight;

			if (lightcos < 0)
			{
				temp += (int)(r_shadelight * lightcos);

				// clamp; because we limited the minimum ambient and shading light, we
				// don't have to clamp low light, just bright
				if (temp < 0)
					temp = 0;
			}

			light = temp / 256;
		}
		*attribute++ = light;
		*attribute++ = light;
		vertex++;
	}
	d_lists.last_colormapped_attribute += mdl->numverts;
	d_lists.last_colormapped_attribute += mdl->numverts;
	viewmodel.count = mdl->numtris * 3;
}

void D_AddViewmodelToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* apverts)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	if (mdl->numverts * 2 <= UPPER_16BIT_LIMIT)
	{
		d_lists.last_viewmodel16++;
		if (d_lists.last_viewmodel16 >= d_lists.viewmodels16.size())
		{
			d_lists.viewmodels16.emplace_back();
		}
		auto& viewmodel = d_lists.viewmodels16[d_lists.last_viewmodel16];
		D_FillViewmodelData(viewmodel, aliashdr, mdl, skindesc, colormap, apverts);
		return;
	}
	d_lists.last_viewmodel32++;
	if (d_lists.last_viewmodel32 >= d_lists.viewmodels32.size())
	{
		d_lists.viewmodels32.emplace_back();
	}
	auto& viewmodel = d_lists.viewmodels32[d_lists.last_viewmodel32];
	D_FillViewmodelData(viewmodel, aliashdr, mdl, skindesc, colormap, apverts);
}

void D_AddParticleToLists (particle_t* part)
{
	auto new_size = d_lists.last_colored_vertex + 1 + 3 * 4;
	if (d_lists.colored_vertices.size() < new_size)
	{
		d_lists.colored_vertices.resize(new_size);
	}
	new_size = d_lists.last_colored_attribute + 1 + 4;
	if (d_lists.colored_attributes.size() < new_size)
	{
		d_lists.colored_attributes.resize(new_size);
	}
	auto x = part->org[0] - vright[0] * 0.5 + vup[0] * 0.5;
	auto y = part->org[1] - vright[1] * 0.5 + vup[1] * 0.5;
	auto z = part->org[2] - vright[2] * 0.5 + vup[2] * 0.5;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = x;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = z;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = -y;
	d_lists.last_colored_attribute++;
	d_lists.colored_attributes[d_lists.last_colored_attribute] = part->color;
	x += vright[0];
	y += vright[1];
	z += vright[2];
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = x;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = z;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = -y;
	d_lists.last_colored_attribute++;
	d_lists.colored_attributes[d_lists.last_colored_attribute] = part->color;
	x -= vup[0];
	y -= vup[1];
	z -= vup[2];
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = x;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = z;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = -y;
	d_lists.last_colored_attribute++;
	d_lists.colored_attributes[d_lists.last_colored_attribute] = part->color;
	x -= vright[0];
	y -= vright[1];
	z -= vright[2];
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = x;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = z;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = -y;
	d_lists.last_colored_attribute++;
	d_lists.colored_attributes[d_lists.last_colored_attribute] = part->color;
	auto first_vertex = (d_lists.last_colored_vertex + 1) / 3;
	if (first_vertex + 4 <= UPPER_16BIT_LIMIT)
	{
		new_size = d_lists.last_colored_index16 + 1 + 6;
		if (d_lists.colored_indices16.size() < new_size)
		{
			d_lists.colored_indices16.resize(new_size);
		}
		auto v0 = first_vertex;
		auto v1 = first_vertex + 1;
		auto v2 = first_vertex + 2;
		d_lists.last_colored_index16++;
		d_lists.colored_indices16[d_lists.last_colored_index16] = v0;
		d_lists.last_colored_index16++;
		d_lists.colored_indices16[d_lists.last_colored_index16] = v1;
		d_lists.last_colored_index16++;
		d_lists.colored_indices16[d_lists.last_colored_index16] = v2;
		v0 = first_vertex + 2;
		v1 = first_vertex + 3;
		v2 = first_vertex;
		d_lists.last_colored_index16++;
		d_lists.colored_indices16[d_lists.last_colored_index16] = v0;
		d_lists.last_colored_index16++;
		d_lists.colored_indices16[d_lists.last_colored_index16] = v1;
		d_lists.last_colored_index16++;
		d_lists.colored_indices16[d_lists.last_colored_index16] = v2;
	}
	else
	{
		new_size = d_lists.last_colored_index32 + 1 + 6;
		if (d_lists.colored_indices32.size() < new_size)
		{
			d_lists.colored_indices32.resize(new_size);
		}
		auto v0 = first_vertex;
		auto v1 = first_vertex + 1;
		auto v2 = first_vertex + 2;
		d_lists.last_colored_index32++;
		d_lists.colored_indices32[d_lists.last_colored_index32] = v0;
		d_lists.last_colored_index32++;
		d_lists.colored_indices32[d_lists.last_colored_index32] = v1;
		d_lists.last_colored_index32++;
		d_lists.colored_indices32[d_lists.last_colored_index32] = v2;
		v0 = first_vertex + 2;
		v1 = first_vertex + 3;
		v2 = first_vertex;
		d_lists.last_colored_index32++;
		d_lists.colored_indices32[d_lists.last_colored_index32] = v0;
		d_lists.last_colored_index32++;
		d_lists.colored_indices32[d_lists.last_colored_index32] = v1;
		d_lists.last_colored_index32++;
		d_lists.colored_indices32[d_lists.last_colored_index32] = v2;
	}
}

void D_AddSkyToLists (qboolean full_area)
{
	if (d_lists.last_sky >= 0)
	{
		return;
	}
	d_lists.last_sky++;
	if (d_lists.last_sky >= d_lists.sky.size())
	{
		d_lists.sky.emplace_back();
	}
	auto& sky = d_lists.sky[d_lists.last_sky];
	if (full_area)
	{
		sky.left = -0.1;
		sky.right = 1.1;
		sky.top = -0.1;
		sky.bottom = 1.1;
	}
	else
	{
		auto left = r_skyleft;
		auto right = r_skyright;
		auto top = r_skytop;
		auto bottom = r_skybottom;
		int extra_horizontal = vid.width / 10;
		int extra_vertical = vid.height / 10;
		sky.left = (float)(left - extra_horizontal) / (float)vid.width;
		sky.right = (float)(right + extra_horizontal) / (float)vid.width;
		sky.top = (float)(top - extra_vertical) / (float)vid.height;
		sky.bottom = (float)(bottom + extra_vertical) / (float)vid.height;
	}
	sky.first_vertex = (d_lists.last_textured_vertex + 1) / 3;
	sky.count = 4;
	auto new_size = d_lists.last_textured_vertex + 1 + 3 * 4;
	if (d_lists.textured_vertices.size() < new_size)
	{
		d_lists.textured_vertices.resize(new_size);
	}
	new_size = d_lists.last_textured_attribute + 1 + 2 * 4;
	if (d_lists.textured_attributes.size() < new_size)
	{
		d_lists.textured_attributes.resize(new_size);
	}
	float x = sky.left * 2 - 1;
	float y = 1;
	float z = 1 - sky.top * 2;
	float s = sky.left;
	float t = sky.top;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = x;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = z;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = -y;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = s;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = t;
	x = sky.right * 2 - 1;
	s = sky.right;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = x;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = z;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = -y;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = s;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = t;
	x = sky.left * 2 - 1;
	z = 1 - sky.bottom * 2;
	s = sky.left;
	t = sky.bottom;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = x;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = z;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = -y;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = s;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = t;
	x = sky.right * 2 - 1;
	s = sky.right;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = x;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = z;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = -y;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = s;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = t;
}

void D_AddSkyboxToLists (mtexinfo_t* textures)
{
    if (d_lists.last_skybox >= 0)
	{
		return;
	}
	d_lists.last_skybox++;
	if (d_lists.last_skybox >= d_lists.skyboxes.size())
	{
		d_lists.skyboxes.emplace_back();
	}
	auto& sky = d_lists.skyboxes[d_lists.last_skybox];
	sky.textures = textures;
}

void D_AddColoredSurfaceToLists (msurface_t* face, entity_t* entity, int color)
{
	if (face->numedges < 3)
	{
		return;
	}
	auto new_size = d_lists.last_colored_vertex + 1 + 3 * face->numedges;
	if (d_lists.colored_vertices.size() < new_size)
	{
		d_lists.colored_vertices.resize(new_size);
	}
	new_size = d_lists.last_colored_attribute + 1 + face->numedges;
	if (d_lists.colored_attributes.size() < new_size)
	{
		d_lists.colored_attributes.resize(new_size);
	}
	auto first_vertex = (d_lists.last_colored_vertex + 1) / 3;
	if (first_vertex + face->numedges <= UPPER_16BIT_LIMIT)
	{
		auto edge = entity->model->surfedges[face->firstedge];
		unsigned int index;
		if (edge >= 0)
		{
			index = entity->model->edges[edge].v[0];
		}
		else
		{
			index = entity->model->edges[-edge].v[1];
		}
		new_size = d_lists.last_colored_index16 + 1 + (face->numedges - 2) * 3;
		if (d_lists.colored_indices16.size() < new_size)
		{
			d_lists.colored_indices16.resize(new_size);
		}
		auto& vertex = entity->model->vertexes[index];
		auto x = vertex.position[0];
		auto y = vertex.position[1];
		auto z = vertex.position[2];
		auto target = d_lists.colored_vertices.data() + d_lists.last_colored_vertex + 1;
		*target++ = x;
		*target++ = z;
		*target = -y;
		target = d_lists.colored_attributes.data() + d_lists.last_colored_attribute + 1;
		*target = color;
		d_lists.last_colored_index16++;
		d_lists.colored_indices16[d_lists.last_colored_index16] = first_vertex;
		auto next_front = 0;
		auto next_back = face->numedges;
		auto use_back = false;
		uint16_t previous_index = 0;
		uint16_t before_previous_index;
		for (auto i = 1; i < face->numedges; i++)
		{
			uint16_t current_index;
			if (use_back)
			{
				next_back--;
				current_index = next_back;
			}
			else
			{
				next_front++;
				current_index = next_front;
			}
			edge = entity->model->surfedges[face->firstedge + current_index];
			if (edge >= 0)
			{
				index = entity->model->edges[edge].v[0];
			}
			else
			{
				index = entity->model->edges[-edge].v[1];
			}
			use_back = !use_back;
			auto& vertex = entity->model->vertexes[index];
			x = vertex.position[0];
			y = vertex.position[1];
			z = vertex.position[2];
			target = d_lists.colored_vertices.data() + d_lists.last_colored_vertex + 1 + current_index * 3;
			*target++ = x;
			*target++ = z;
			*target = -y;
			target = d_lists.colored_attributes.data() + d_lists.last_colored_attribute + 1 + current_index;
			*target = color;
			if (i >= 3)
			{
				if (use_back)
				{
					d_lists.last_colored_index16++;
					d_lists.colored_indices16[d_lists.last_colored_index16] = first_vertex + previous_index;
					d_lists.last_colored_index16++;
					d_lists.colored_indices16[d_lists.last_colored_index16] = first_vertex + before_previous_index;
				}
				else
				{
					d_lists.last_colored_index16++;
					d_lists.colored_indices16[d_lists.last_colored_index16] = first_vertex + before_previous_index;
					d_lists.last_colored_index16++;
					d_lists.colored_indices16[d_lists.last_colored_index16] = first_vertex + previous_index;
				}
			}
			d_lists.last_colored_index16++;
			d_lists.colored_indices16[d_lists.last_colored_index16] = first_vertex + current_index;
			before_previous_index = previous_index;
			previous_index = current_index;
		}
	}
	else
	{
		auto edge = entity->model->surfedges[face->firstedge];
		unsigned int index;
		if (edge >= 0)
		{
			index = entity->model->edges[edge].v[0];
		}
		else
		{
			index = entity->model->edges[-edge].v[1];
		}
		new_size = d_lists.last_colored_index32 + 1 + (face->numedges - 2) * 3;
		if (d_lists.colored_indices32.size() < new_size)
		{
			d_lists.colored_indices32.resize(new_size);
		}
		auto& vertex = entity->model->vertexes[index];
		auto x = vertex.position[0];
		auto y = vertex.position[1];
		auto z = vertex.position[2];
		auto target = d_lists.colored_vertices.data() + d_lists.last_colored_vertex + 1;
		*target++ = x;
		*target++ = z;
		*target = -y;
		target = d_lists.colored_attributes.data() + d_lists.last_colored_attribute + 1;
		*target = color;
		d_lists.last_colored_index32++;
		d_lists.colored_indices32[d_lists.last_colored_index32] = first_vertex;
		auto next_front = 0;
		auto next_back = face->numedges;
		auto use_back = false;
		uint32_t previous_index = first_vertex;
		uint32_t before_previous_index;
		for (auto i = 1; i < face->numedges; i++)
		{
			uint32_t current_index;
			if (use_back)
			{
				next_back--;
				current_index = next_back;
			}
			else
			{
				next_front++;
				current_index = next_front;
			}
			edge = entity->model->surfedges[face->firstedge + current_index];
			if (edge >= 0)
			{
				index = entity->model->edges[edge].v[0];
			}
			else
			{
				index = entity->model->edges[-edge].v[1];
			}
			use_back = !use_back;
			auto& vertex = entity->model->vertexes[index];
			x = vertex.position[0];
			y = vertex.position[1];
			z = vertex.position[2];
			target = d_lists.colored_vertices.data() + d_lists.last_colored_vertex + 1 + current_index * 3;
			*target++ = x;
			*target++ = z;
			*target = -y;
			target = d_lists.colored_attributes.data() + d_lists.last_colored_attribute + 1 + current_index;
			*target = color;
			if (i >= 3)
			{
				if (use_back)
				{
					d_lists.last_colored_index32++;
					d_lists.colored_indices32[d_lists.last_colored_index32] = first_vertex + previous_index;
					d_lists.last_colored_index32++;
					d_lists.colored_indices32[d_lists.last_colored_index32] = first_vertex + before_previous_index;
				}
				else
				{
					d_lists.last_colored_index32++;
					d_lists.colored_indices32[d_lists.last_colored_index32] = first_vertex + before_previous_index;
					d_lists.last_colored_index32++;
					d_lists.colored_indices32[d_lists.last_colored_index32] = first_vertex + previous_index;
				}
			}
			d_lists.last_colored_index32++;
			d_lists.colored_indices32[d_lists.last_colored_index32] = first_vertex + current_index;
			before_previous_index = previous_index;
			previous_index = current_index;
		}
	}
	d_lists.last_colored_vertex += 3 * face->numedges;
	d_lists.last_colored_attribute += face->numedges;
}
