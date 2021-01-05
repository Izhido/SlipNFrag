#include "quakedef.h"
#include "d_lists.h"
#include "r_shared.h"
#include "r_local.h"
#include "d_local.h"

dlists_t d_lists { -1, -1, -1, -1, -1, -1, -1, -1, -1,-1, -1, -1, -1, -1, -1, -1, -1 };

qboolean d_uselists = false;
qboolean d_awayfromviewmodel = false;

extern int r_ambientlight;
extern float r_shadelight;
#define NUMVERTEXNORMALS 162
extern float r_avertexnormals[NUMVERTEXNORMALS][3];
extern vec3_t r_plightvec;

void D_ResetLists ()
{
	d_lists.last_surface = -1;
	d_lists.last_sprite = -1;
	d_lists.last_turbulent = -1;
	d_lists.last_alias = -1;
	d_lists.last_viewmodel = -1;
	d_lists.last_particle = -1;
	d_lists.last_sky = -1;
    d_lists.last_skybox = -1;
	d_lists.last_textured_vertex = -1;
	d_lists.last_textured_attribute = -1;
	d_lists.last_colormapped_attribute = -1;
	d_lists.last_colormapped_index16 = -1;
	d_lists.last_colormapped_index32 = -1;
	d_lists.last_colored_vertex = -1;
	d_lists.last_colored_index16 = -1;
	d_lists.last_colored_index32 = -1;
	d_lists.clear_color = -1;
}

void D_AddSurfaceToLists (msurface_t* face, surfcache_t* cache, entity_t* entity, qboolean created)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_surface++;
	if (d_lists.last_surface >= d_lists.surfaces.size())
	{
		d_lists.surfaces.emplace_back();
	}
	auto& surface = d_lists.surfaces[d_lists.last_surface];
	surface.surface = face;
	surface.entity = entity;
	surface.created = (created ? 1: 0);
	surface.width = cache->width;
	surface.height = cache->height;
	surface.size = surface.width * surface.height;
	if (surface.size > surface.data.size())
	{
		surface.data.resize(surface.size);
	}
	memcpy(surface.data.data(), cache->data, surface.size);
	surface.first_vertex = (d_lists.last_textured_vertex + 1) / 3;
	surface.count = face->numedges;
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	auto new_size = d_lists.last_textured_vertex + 1 + 3 * face->numedges;
	if (d_lists.textured_vertices.size() < new_size)
	{
		d_lists.textured_vertices.resize(new_size);
	}
	new_size = d_lists.last_textured_attribute + 1 + 2 * face->numedges;
	if (d_lists.textured_attributes.size() < new_size)
	{
		d_lists.textured_attributes.resize(new_size);
	}
	auto texinfo = face->texinfo;
	auto edge = entity->model->surfedges[face->firstedge];
	mvertex_t* vertex;
	if (edge >= 0)
	{
		vertex = &entity->model->vertexes[entity->model->edges[edge].v[0]];
	}
	else
	{
		vertex = &entity->model->vertexes[entity->model->edges[-edge].v[1]];
	}
	auto x = vertex->position[0];
	auto y = vertex->position[1];
	auto z = vertex->position[2];
	auto s = x * texinfo->vecs[0][0] + y * texinfo->vecs[0][1] + z * texinfo->vecs[0][2] + texinfo->vecs[0][3];
	auto t = x * texinfo->vecs[1][0] + y * texinfo->vecs[1][1] + z * texinfo->vecs[1][2] + texinfo->vecs[1][3];
	s = (s - face->texturemins[0]) / face->extents[0];
	t = (t - face->texturemins[1]) / face->extents[1];
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
	auto next_front = 0;
	auto next_back = face->numedges;
	auto use_back = false;
	for (auto i = 1; i < face->numedges; i++)
	{
		if (use_back)
		{
			next_back--;
			edge = entity->model->surfedges[face->firstedge + next_back];
		}
		else
		{
			next_front++;
			edge = entity->model->surfedges[face->firstedge + next_front];
		}
		use_back = !use_back;
		if (edge >= 0)
		{
			vertex = &entity->model->vertexes[entity->model->edges[edge].v[0]];
		}
		else
		{
			vertex = &entity->model->vertexes[entity->model->edges[-edge].v[1]];
		}
		x = vertex->position[0];
		y = vertex->position[1];
		z = vertex->position[2];
		s = x * texinfo->vecs[0][0] + y * texinfo->vecs[0][1] + z * texinfo->vecs[0][2] + texinfo->vecs[0][3];
		t = x * texinfo->vecs[1][0] + y * texinfo->vecs[1][1] + z * texinfo->vecs[1][2] + texinfo->vecs[1][3];
		s = (s - face->texturemins[0]) / face->extents[0];
		t = (t - face->texturemins[1]) / face->extents[1];
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

void D_AddTurbulentToLists (msurface_t* face, entity_t* entity)
{
	if (face->numedges < 3)
	{
		return;
	}
	d_lists.last_turbulent++;
	if (d_lists.last_turbulent >= d_lists.turbulent.size())
	{
		d_lists.turbulent.emplace_back();
	}
	auto texinfo = face->texinfo;
	auto texture = texinfo->texture;
	auto& turbulent = d_lists.turbulent[d_lists.last_turbulent];
	turbulent.texture = texture;
	turbulent.width = texture->width;
	turbulent.height = texture->height;
	turbulent.size = turbulent.width * turbulent.height;
	turbulent.data = (unsigned char*)texture + texture->offsets[0];
	turbulent.first_vertex = (d_lists.last_textured_vertex + 1) / 3;
	turbulent.count = face->numedges;
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	auto new_size = d_lists.last_textured_vertex + 1 + 3 * face->numedges;
	if (d_lists.textured_vertices.size() < new_size)
	{
		d_lists.textured_vertices.resize(new_size);
	}
	new_size = d_lists.last_textured_attribute + 1 + 2 * face->numedges;
	if (d_lists.textured_attributes.size() < new_size)
	{
		d_lists.textured_attributes.resize(new_size);
	}
	auto edge = entity->model->surfedges[face->firstedge];
	mvertex_t* vertex;
	if (edge >= 0)
	{
		vertex = &entity->model->vertexes[entity->model->edges[edge].v[0]];
	}
	else
	{
		vertex = &entity->model->vertexes[entity->model->edges[-edge].v[1]];
	}
	auto x = vertex->position[0];
	auto y = vertex->position[1];
	auto z = vertex->position[2];
	auto s = x * texinfo->vecs[0][0] + y * texinfo->vecs[0][1] + z * texinfo->vecs[0][2] + texinfo->vecs[0][3];
	auto t = x * texinfo->vecs[1][0] + y * texinfo->vecs[1][1] + z * texinfo->vecs[1][2] + texinfo->vecs[1][3];
	s /= turbulent.width;
	t /= turbulent.height;
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
	auto next_front = 0;
	auto next_back = face->numedges;
	auto use_back = false;
	for (auto i = 1; i < face->numedges; i++)
	{
		if (use_back)
		{
			next_back--;
			edge = entity->model->surfedges[face->firstedge + next_back];
		}
		else
		{
			next_front++;
			edge = entity->model->surfedges[face->firstedge + next_front];
		}
		use_back = !use_back;
		if (edge >= 0)
		{
			vertex = &entity->model->vertexes[entity->model->edges[edge].v[0]];
		}
		else
		{
			vertex = &entity->model->vertexes[entity->model->edges[-edge].v[1]];
		}
		x = vertex->position[0];
		y = vertex->position[1];
		z = vertex->position[2];
		s = x * texinfo->vecs[0][0] + y * texinfo->vecs[0][1] + z * texinfo->vecs[0][2] + texinfo->vecs[0][3];
		t = x * texinfo->vecs[1][0] + y * texinfo->vecs[1][1] + z * texinfo->vecs[1][2] + texinfo->vecs[1][3];
		s /= turbulent.width;
		t /= turbulent.height;
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
}

void D_AddAliasIndicesToLists (dalias_t& alias, aliashdr_t* aliashdr, bool is_index16, mdl_t* mdl)
{
	auto texcoordsbase = alias.texture_coordinates;
	auto triangle = (mtriangle_t *)((byte *)aliashdr + aliashdr->triangles);
	if (is_index16)
	{
		auto new_size = d_lists.last_colormapped_index16 + 1 + 3 * mdl->numtris;
		if (d_lists.colormapped_indices16.size() < new_size)
		{
			d_lists.colormapped_indices16.resize(new_size);
		}
		for (auto i = 0; i < mdl->numtris; i++)
		{
			auto v0 = triangle->vertindex[0];
			auto v1 = triangle->vertindex[1];
			auto v2 = triangle->vertindex[2];
			auto v0back = (((texcoordsbase[v0].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v1back = (((texcoordsbase[v1].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v2back = (((texcoordsbase[v2].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			v0 = v0 * 2 + (v0back ? 1 : 0);
			v1 = v1 * 2 + (v1back ? 1 : 0);
			v2 = v2 * 2 + (v2back ? 1 : 0);
			d_lists.last_colormapped_index16++;
			d_lists.colormapped_indices16[d_lists.last_colormapped_index16] = v0;
			d_lists.last_colormapped_index16++;
			d_lists.colormapped_indices16[d_lists.last_colormapped_index16] = v1;
			d_lists.last_colormapped_index16++;
			d_lists.colormapped_indices16[d_lists.last_colormapped_index16] = v2;
			triangle++;
		}
	}
	else
	{
		auto new_size = d_lists.last_colormapped_index32 + 1 + 3 * mdl->numtris;
		if (d_lists.colormapped_indices32.size() < new_size)
		{
			d_lists.colormapped_indices32.resize(new_size);
		}
		for (auto i = 0; i < mdl->numtris; i++)
		{
			auto v0 = triangle->vertindex[0];
			auto v1 = triangle->vertindex[1];
			auto v2 = triangle->vertindex[2];
			auto v0back = (((texcoordsbase[v0].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v1back = (((texcoordsbase[v1].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v2back = (((texcoordsbase[v2].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			v0 = v0 * 2 + (v0back ? 1 : 0);
			v1 = v1 * 2 + (v1back ? 1 : 0);
			v2 = v2 * 2 + (v2back ? 1 : 0);
			d_lists.last_colormapped_index32++;
			d_lists.colormapped_indices32[d_lists.last_colormapped_index32] = v0;
			d_lists.last_colormapped_index32++;
			d_lists.colormapped_indices32[d_lists.last_colormapped_index32] = v1;
			d_lists.last_colormapped_index32++;
			d_lists.colormapped_indices32[d_lists.last_colormapped_index32] = v2;
			triangle++;
		}
	}
}

void D_AddAliasToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* vertices)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists.last_alias++;
	if (d_lists.last_alias >= d_lists.alias.size())
	{
		d_lists.alias.emplace_back();
	}
	auto& alias = d_lists.alias[d_lists.last_alias];
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
	alias.vertices = vertices;
	alias.texture_coordinates = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);
	alias.vertex_count = mdl->numverts;
	auto is_index16 = (mdl->numverts * 2 <= 65520);
	if (is_index16)
	{
		alias.first_index16 = d_lists.last_colormapped_index16 + 1;
		alias.first_index32 = -1;
	}
	else
	{
		alias.first_index16 = -1;
		alias.first_index32 = d_lists.last_colormapped_index32 + 1;
	}
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
	auto vertex = vertices;
	for (auto i = 0; i < mdl->numverts; i++)
	{
		// lighting
		auto plightnormal = r_avertexnormals[vertex->lightnormalindex];
		auto lightcos = DotProduct (plightnormal, r_plightvec);
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

		d_lists.last_colormapped_attribute++;
		d_lists.colormapped_attributes[d_lists.last_colormapped_attribute] = light;
		d_lists.last_colormapped_attribute++;
		d_lists.colormapped_attributes[d_lists.last_colormapped_attribute] = light;
		vertex++;
	}
	alias.count = mdl->numtris * 3;
	D_AddAliasIndicesToLists (alias, aliashdr, is_index16, mdl);
}

void D_AddViewModelToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* vertices)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists.last_viewmodel++;
	if (d_lists.last_viewmodel >= d_lists.viewmodel.size())
	{
		d_lists.viewmodel.emplace_back();
	}
	auto& view_model = d_lists.viewmodel[d_lists.last_viewmodel];
	view_model.width = mdl->skinwidth;
	view_model.height = mdl->skinheight;
	view_model.size = view_model.width * view_model.height;
	view_model.data = (byte *)aliashdr + skindesc->skin;
	if (colormap == host_colormap.data())
	{
		view_model.is_host_colormap = true;
	}
	else
	{
		view_model.is_host_colormap = false;
		if (view_model.colormap.size() < 16384)
		{
			view_model.colormap.resize(16384);
		}
		memcpy(view_model.colormap.data(), colormap, 16384);
	}
	view_model.vertices = vertices;
	view_model.texture_coordinates = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);
	view_model.vertex_count = mdl->numverts;
	auto is_index16 = (mdl->numverts * 2 <= 65520);
	if (is_index16)
	{
		view_model.first_index16 = d_lists.last_colormapped_index16 + 1;
		view_model.first_index32 = -1;
	}
	else
	{
		view_model.first_index16 = -1;
		view_model.first_index32 = d_lists.last_colormapped_index32 + 1;
	}
	view_model.first_attribute = d_lists.last_colormapped_attribute + 1;
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
	R_ConcatTransforms (t2matrix, tmatrix, view_model.transform);
	auto vertex = vertices;
	auto texcoordsbase = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);
	auto texcoords = texcoordsbase;
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
			auto plightnormal = r_avertexnormals[vertex->lightnormalindex];
			auto lightcos = DotProduct (plightnormal, r_plightvec);
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
		d_lists.last_colormapped_attribute++;
		d_lists.colormapped_attributes[d_lists.last_colormapped_attribute] = light;
		d_lists.last_colormapped_attribute++;
		d_lists.colormapped_attributes[d_lists.last_colormapped_attribute] = light;
		vertex++;
		texcoords++;
	}
	view_model.count = mdl->numtris * 3;
	D_AddAliasIndicesToLists (view_model, aliashdr, is_index16, mdl);
}

void D_AddParticleToLists (particle_t* part)
{
	auto first_vertex = (d_lists.last_colored_vertex + 1) / 3;
	auto is_index16 = (first_vertex + 4 <= 65520);
	auto is_new = true;
	if (d_lists.last_particle >= 0)
	{
		auto& particle = d_lists.particles[d_lists.last_particle];
		if (particle.color == part->color)
		{
			if (particle.first_index16 >= 0)
			{
				if (is_index16 && particle.first_index16 + particle.count == d_lists.last_colored_index16 + 1)
				{
					is_new = false;
				}
			}
			else if (particle.first_index32 >= 0)
			{
				if (!is_index16 && particle.first_index32 + particle.count == d_lists.last_colored_index32 + 1)
				{
					is_new = false;
				}
			}
		}
	}
	if (is_new)
	{
		d_lists.last_particle++;
		if (d_lists.last_particle >= d_lists.particles.size())
		{
			d_lists.particles.emplace_back();
		}
	}
	auto& particle = d_lists.particles[d_lists.last_particle];
	if (is_new)
	{
		particle.color = part->color;
		particle.count = 0;
		if (is_index16)
		{
			particle.first_index16 = d_lists.last_colored_index16 + 1;
			particle.first_index32 = -1;
		}
		else
		{
			particle.first_index16 = -1;
			particle.first_index32 = d_lists.last_colored_index32 + 1;
		}
	}
	auto new_size = d_lists.last_colored_vertex + 1 + 3 * 4;
	if (d_lists.colored_vertices.size() < new_size)
	{
		d_lists.colored_vertices.resize(new_size);
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
	x += vright[0];
	y += vright[1];
	z += vright[2];
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = x;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = z;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = -y;
	x -= vup[0];
	y -= vup[1];
	z -= vup[2];
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = x;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = z;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = -y;
	x -= vright[0];
	y -= vright[1];
	z -= vright[2];
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = x;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = z;
	d_lists.last_colored_vertex++;
	d_lists.colored_vertices[d_lists.last_colored_vertex] = -y;
	particle.count += 6;
	if (is_index16)
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

void D_AddSkyToLists (surf_t* surf)
{
	auto pspan = surf->spans;
	if (pspan == nullptr)
	{
		return;
	}
	int left;
	int right;
	int top;
	int bottom;
	while (pspan != nullptr)
	{
		if (pspan == surf->spans)
		{
			left = pspan->u;
			right = left + pspan->count;
			top = pspan->v;
			bottom = pspan->v;
		}
		else
		{
			left = std::min(left, pspan->u);
			right = std::max(right, left + pspan->count);
			top = std::min(top, pspan->v);
			bottom = std::max(bottom, pspan->v);
		}
		pspan = pspan->pnext;
	}
	left -= vid.width / 10;
	right += vid.height / 10;
	top -= vid.height / 10;
	bottom += vid.height / 10;
	auto is_new = false;
	if (d_lists.last_sky < 0)
	{
		d_lists.last_sky++;
		if (d_lists.last_sky >= d_lists.sky.size())
		{
			d_lists.sky.emplace_back();
		}
		is_new = true;
	}
	auto& sky = d_lists.sky[d_lists.last_sky];
	if (is_new)
	{
		sky.left = (float)left / (float)vid.width;
		sky.right = (float)right / (float)vid.width;
		sky.top = (float)top / (float)vid.height;
		sky.bottom = (float)bottom / (float)vid.height;
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
	else
	{
		sky.left = std::min(sky.left, (float)left / (float)vid.width);
		sky.right = std::max(sky.right, (float)right / (float)vid.width);
		sky.top = std::min(sky.top, (float)top / (float)vid.height);
		sky.bottom = std::max(sky.bottom, (float)bottom / (float)vid.height);
		auto vertex = sky.first_vertex * 3;
		auto attribute = sky.first_vertex * 2;
		float x = sky.left * 2 - 1;
		float y = 1;
		float z = 1 - sky.top * 2;
		float s = sky.left;
		float t = sky.top;
		d_lists.textured_vertices[vertex] = x;
		vertex++;
		d_lists.textured_vertices[vertex] = z;
		vertex++;
		d_lists.textured_vertices[vertex] = -y;
		vertex++;
		d_lists.textured_attributes[attribute] = s;
		attribute++;
		d_lists.textured_attributes[attribute] = t;
		attribute++;
		x = sky.right * 2 - 1;
		s = sky.right;
		d_lists.textured_vertices[vertex] = x;
		vertex++;
		d_lists.textured_vertices[vertex] = z;
		vertex++;
		d_lists.textured_vertices[vertex] = -y;
		vertex++;
		d_lists.textured_attributes[attribute] = s;
		attribute++;
		d_lists.textured_attributes[attribute] = t;
		attribute++;
		x = sky.left * 2 - 1;
		z = 1 - sky.bottom * 2;
		s = sky.left;
		t = sky.bottom;
		d_lists.textured_vertices[vertex] = x;
		vertex++;
		d_lists.textured_vertices[vertex] = z;
		vertex++;
		d_lists.textured_vertices[vertex] = -y;
		vertex++;
		d_lists.textured_attributes[attribute] = s;
		attribute++;
		d_lists.textured_attributes[attribute] = t;
		attribute++;
		x = sky.right * 2 - 1;
		s = sky.right;
		d_lists.textured_vertices[vertex] = x;
		vertex++;
		d_lists.textured_vertices[vertex] = z;
		vertex++;
		d_lists.textured_vertices[vertex] = -y;
		d_lists.textured_attributes[attribute] = s;
		attribute++;
		d_lists.textured_attributes[attribute] = t;
	}
}

void D_AddSkyboxToLists (mtexinfo_t* textures)
{
    if (d_lists.last_skybox < 0)
    {
        d_lists.last_skybox++;
        if (d_lists.last_skybox >= d_lists.skyboxes.size())
        {
            d_lists.skyboxes.emplace_back();
        }
        auto& sky = d_lists.skyboxes[d_lists.last_skybox];
        sky.textures = textures;
    }
}
