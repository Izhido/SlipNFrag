#include "quakedef.h"
#include "d_lists.h"
#include "r_shared.h"
#include "r_local.h"
#include "d_local.h"

std::array<dlists_t, 3> d_lists
{
	{
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
	}
};

std::array<dlists_state_t, 3> d_lists_state { { dlists_produced, dlists_consumed, dlists_consumed } };

dlists_t* d_lists_producing = &d_lists[0];
int d_lists_producer_index;

dlists_t* d_lists_consuming = &d_lists[1];
int d_lists_consumer_index = 1;

qboolean d_uselists;

extern int r_ambientlight;
extern argbcolor_t r_ambientcoloredlight;
extern float r_shadelight;
extern argbcolorf_t r_shadecoloredlight;
#define NUMVERTEXNORMALS 162
extern float r_avertexnormals[NUMVERTEXNORMALS][3];
extern vec3_t r_plightvec;

void D_ResetLists (dlists_t* lists)
{
	lists->last_surface = -1;
	lists->last_surface_colored_lights = -1;
	lists->last_surface_rgba = -1;
	lists->last_surface_rgba_colored_lights = -1;
	lists->last_surface_rgba_no_glow = -1;
	lists->last_surface_rgba_no_glow_colored_lights = -1;
	lists->last_surface_rotated = -1;
	lists->last_surface_rotated_colored_lights = -1;
	lists->last_surface_rotated_rgba = -1;
	lists->last_surface_rotated_rgba_colored_lights = -1;
	lists->last_surface_rotated_rgba_no_glow = -1;
	lists->last_surface_rotated_rgba_no_glow_colored_lights = -1;
	lists->last_fence = -1;
	lists->last_fence_colored_lights = -1;
	lists->last_fence_rgba = -1;
	lists->last_fence_rgba_colored_lights = -1;
	lists->last_fence_rgba_no_glow = -1;
	lists->last_fence_rgba_no_glow_colored_lights = -1;
	lists->last_fence_rotated = -1;
	lists->last_fence_rotated_colored_lights = -1;
	lists->last_fence_rotated_rgba = -1;
	lists->last_fence_rotated_rgba_colored_lights = -1;
	lists->last_fence_rotated_rgba_no_glow = -1;
	lists->last_fence_rotated_rgba_no_glow_colored_lights = -1;
	lists->last_turbulent = -1;
	lists->last_turbulent_rgba = -1;
	lists->last_turbulent_lit = -1;
	lists->last_turbulent_colored_lights = -1;
	lists->last_turbulent_rgba_lit = -1;
	lists->last_turbulent_rgba_colored_lights = -1;
	lists->last_turbulent_rotated = -1;
	lists->last_turbulent_rotated_rgba = -1;
	lists->last_turbulent_rotated_lit = -1;
	lists->last_turbulent_rotated_colored_lights = -1;
	lists->last_turbulent_rotated_rgba_lit = -1;
	lists->last_turbulent_rotated_rgba_colored_lights = -1;
	lists->last_sprite = -1;
	lists->last_alias = -1;
	lists->last_alias_alpha = -1;
	lists->last_alias_colored_lights = -1;
	lists->last_alias_alpha_colored_lights = -1;
	lists->last_alias_holey = -1;
	lists->last_alias_holey_alpha = -1;
	lists->last_alias_holey_colored_lights = -1;
	lists->last_alias_holey_alpha_colored_lights = -1;
	lists->last_viewmodel = -1;
	lists->last_viewmodel_colored_lights = -1;
	lists->last_viewmodel_holey = -1;
	lists->last_viewmodel_holey_colored_lights = -1;
	lists->last_sky = -1;
	lists->last_sky_rgba = -1;
    lists->last_skybox = -1;
	lists->last_textured_vertex = -1;
	lists->last_textured_attribute = -1;
	lists->last_alias_light = -1;
	lists->last_particle = -1;
	lists->last_colored_vertex = -1;
	lists->last_colored_color = -1;
	lists->last_colored_index8 = -1;
	lists->last_colored_index16 = -1;
	lists->last_colored_index32 = -1;
	lists->last_cutout_vertex = -1;
	lists->last_cutout_index8 = -1;
	lists->last_cutout_index16 = -1;
	lists->last_cutout_index32 = -1;
	lists->last_extra_dlightbit = -1;
	lists->last_dynamic_light = -1;
	lists->clear_color = -1;
}

void D_ClearLists (dlists_t* lists)
{
	D_ResetLists (lists);
	std::vector<dsurface_t>().swap(lists->surfaces);
	std::vector<dsurface_t>().swap(lists->surfaces_colored_lights);
	std::vector<dsurfacewithglow_t>().swap(lists->surfaces_rgba);
	std::vector<dsurfacewithglow_t>().swap(lists->surfaces_rgba_colored_lights);
	std::vector<dsurface_t>().swap(lists->surfaces_rgba_no_glow);
	std::vector<dsurface_t>().swap(lists->surfaces_rgba_no_glow_colored_lights);
	std::vector<dsurfacerotated_t>().swap(lists->surfaces_rotated);
	std::vector<dsurfacerotated_t>().swap(lists->surfaces_rotated_colored_lights);
	std::vector<dsurfacerotatedwithglow_t>().swap(lists->surfaces_rotated_rgba);
	std::vector<dsurfacerotatedwithglow_t>().swap(lists->surfaces_rotated_rgba_colored_lights);
	std::vector<dsurfacerotated_t>().swap(lists->surfaces_rotated_rgba_no_glow);
	std::vector<dsurfacerotated_t>().swap(lists->surfaces_rotated_rgba_no_glow_colored_lights);
	std::vector<dsurface_t>().swap(lists->fences);
	std::vector<dsurface_t>().swap(lists->fences_colored_lights);
	std::vector<dsurfacewithglow_t>().swap(lists->fences_rgba);
	std::vector<dsurfacewithglow_t>().swap(lists->fences_rgba_colored_lights);
	std::vector<dsurface_t>().swap(lists->fences_rgba_no_glow);
	std::vector<dsurface_t>().swap(lists->fences_rgba_no_glow_colored_lights);
	std::vector<dsurfacerotated_t>().swap(lists->fences_rotated);
	std::vector<dsurfacerotated_t>().swap(lists->fences_rotated_colored_lights);
	std::vector<dsurfacerotatedwithglow_t>().swap(lists->fences_rotated_rgba);
	std::vector<dsurfacerotatedwithglow_t>().swap(lists->fences_rotated_rgba_colored_lights);
	std::vector<dsurfacerotated_t>().swap(lists->fences_rotated_rgba_no_glow);
	std::vector<dsurfacerotated_t>().swap(lists->fences_rotated_rgba_no_glow_colored_lights);
	std::vector<dturbulent_t>().swap(lists->turbulent);
	std::vector<dturbulent_t>().swap(lists->turbulent_rgba);
	std::vector<dsurface_t>().swap(lists->turbulent_lit);
	std::vector<dsurface_t>().swap(lists->turbulent_colored_lights);
	std::vector<dsurface_t>().swap(lists->turbulent_rgba_lit);
	std::vector<dsurface_t>().swap(lists->turbulent_rgba_colored_lights);
	std::vector<dturbulentrotated_t>().swap(lists->turbulent_rotated);
	std::vector<dturbulentrotated_t>().swap(lists->turbulent_rotated_rgba);
	std::vector<dsurfacerotated_t>().swap(lists->turbulent_rotated_lit);
	std::vector<dsurfacerotated_t>().swap(lists->turbulent_rotated_colored_lights);
	std::vector<dsurfacerotated_t>().swap(lists->turbulent_rotated_rgba_lit);
	std::vector<dsurfacerotated_t>().swap(lists->turbulent_rotated_rgba_colored_lights);
	std::vector<dspritedata_t>().swap(lists->sprites);
	std::vector<dalias_t>().swap(lists->alias);
	std::vector<dalias_t>().swap(lists->alias_alpha);
	std::vector<daliascoloredlights_t>().swap(lists->alias_colored_lights);
	std::vector<daliascoloredlights_t>().swap(lists->alias_alpha_colored_lights);
	std::vector<dalias_t>().swap(lists->alias_holey);
	std::vector<dalias_t>().swap(lists->alias_holey_alpha);
	std::vector<daliascoloredlights_t>().swap(lists->alias_holey_colored_lights);
	std::vector<daliascoloredlights_t>().swap(lists->alias_holey_alpha_colored_lights);
	std::vector<dviewmodel_t>().swap(lists->viewmodels);
	std::vector<dviewmodelcoloredlights_t>().swap(lists->viewmodels_colored_lights);
	std::vector<dviewmodel_t>().swap(lists->viewmodels_holey);
	std::vector<dviewmodelcoloredlights_t>().swap(lists->viewmodels_holey_colored_lights);
	std::vector<dsky_t>().swap(lists->sky);
	std::vector<dsky_t>().swap(lists->sky_rgba);
	std::vector<dskybox_t>().swap(lists->skyboxes);
	std::vector<float>().swap(lists->textured_vertices);
	std::vector<float>().swap(lists->textured_attributes);
	std::vector<float>().swap(lists->alias_lights);
	std::vector<float>().swap(lists->particles);
	std::vector<float>().swap(lists->colored_vertices);
	std::vector<float>().swap(lists->colored_colors);
	std::vector<unsigned char>().swap(lists->colored_indices8);
	std::vector<uint16_t>().swap(lists->colored_indices16);
	std::vector<uint32_t>().swap(lists->colored_indices32);
	std::vector<float>().swap(lists->cutout_vertices);
	std::vector<unsigned char>().swap(lists->cutout_indices8);
	std::vector<uint16_t>().swap(lists->cutout_indices16);
	std::vector<uint32_t>().swap(lists->cutout_indices32);
	std::vector<ddynamiclight_t>().swap(lists->dynamic_lights);
	std::vector<unsigned char>().swap(lists->extra_dlightbits);
	std::vector<unsigned char>().swap(lists->con_buffer);
}

void D_PickProducer ()
{
	auto new_producer = -1;
	for (auto i = 0; i < (int)d_lists_state.size(); i++)
	{
		if (d_lists_state[i] == dlists_consumed)
		{
			new_producer = i;
			break;
		}
	}
	if (new_producer < 0)
	{
		for (auto i = 0; i < (int)d_lists_state.size(); i++)
		{
			if (d_lists_state[i] == dlists_produced && i != d_lists_producer_index && i != d_lists_consumer_index)
			{
				new_producer = i;
				break;
			}
		}
	}
	if (new_producer >= 0)
	{
		d_lists_producer_index = new_producer;
		d_lists_producing = &d_lists[d_lists_producer_index];
		d_lists_state[d_lists_producer_index] = dlists_producing;
	}
}

void D_MarkAsProduced ()
{
	d_lists_state[d_lists_producer_index] = dlists_produced;
}

void D_PickConsumer ()
{
	auto new_consumer = -1;
	for (auto i = 0; i < (int)d_lists_state.size(); i++)
	{
		if (d_lists_state[i] == dlists_produced)
		{
			new_consumer = i;
			break;
		}
	}
	if (new_consumer >= 0)
	{
		d_lists_state[d_lists_consumer_index] = dlists_consumed;
		d_lists_consumer_index = new_consumer;
		d_lists_consuming = &d_lists[d_lists_consumer_index];
		d_lists_state[d_lists_consumer_index] = dlists_consuming;
	}
}

void D_FillSurfaceSize (dturbulent_t& turbulent, int component_size, int mips)
{
	auto size = turbulent.width * turbulent.height * component_size;
	turbulent.size = size;
	turbulent.mips = mips;
	mips--;
	while (mips > 0)
	{
		size /= 4;
		if (size < 1)
		{
			size = 1;
		}
		turbulent.size += size;
		mips--;
	}
}

void D_FillSurfaceDynamicLights(dsurface_t& surface, msurface_t* face)
{
	if (face->dlightframe == r_framecount)
	{
		surface.dlightbits = face->dlightbits;
		surface.num_extra_dlightbits = (int)face->dlightbits_vec.size();
		if (surface.num_extra_dlightbits > 0)
		{
			surface.first_extra_dlightbit = d_lists_producing->last_extra_dlightbit + 1;
			auto new_size = d_lists_producing->last_extra_dlightbit + 1 + surface.num_extra_dlightbits;
			if (d_lists_producing->extra_dlightbits.size() < new_size)
			{
				d_lists_producing->extra_dlightbits.resize(new_size);
			}
			for (int i = 0; i < surface.num_extra_dlightbits; i++)
			{
				d_lists_producing->extra_dlightbits[surface.first_extra_dlightbit + i] = face->dlightbits_vec[i];
			}
			d_lists_producing->last_extra_dlightbit += surface.num_extra_dlightbits;
		}
		else
		{
			surface.first_extra_dlightbit = -1;
		}
	}
	else
	{
		surface.dlightbits = 0;
		surface.first_extra_dlightbit = -1;
		surface.num_extra_dlightbits = 0;
	}
}

void D_FillSurfaceData (dsurface_t& surface, msurface_t* face, entity_t* entity, texture_t* texture, int mips)
{
	surface.face = face;
	surface.model = entity->model;
	surface.width = texture->width;
	surface.height = texture->height;
	D_FillSurfaceSize(surface, 1, mips);
	surface.data = (unsigned char*)texture + texture->offsets[0];
	surface.count = face->numedges;
	D_FillSurfaceDynamicLights(surface, face);
}

void D_FillSurfaceColoredLightsData (dsurface_t& surface, msurface_t* face, entity_t* entity, texture_t* texture, int mips)
{
	surface.face = face;
	surface.model = entity->model;
	surface.width = texture->width;
	surface.height = texture->height;
	D_FillSurfaceSize(surface, 1, mips);
	surface.data = (unsigned char*)texture + texture->offsets[0];
	surface.count = face->numedges;
	D_FillSurfaceDynamicLights(surface, face);
}

void D_FillSurfaceRGBAData (dsurfacewithglow_t& surface, msurface_t* face, entity_t* entity, miptex_t* color_texture, miptex_t* glow_texture, int mips)
{
	surface.face = face;
	surface.model = entity->model;
	surface.width = color_texture->width;
	surface.height = color_texture->height;
	D_FillSurfaceSize(surface, sizeof(unsigned), mips);
	surface.data = (unsigned char*)color_texture + color_texture->offsets[0];
	surface.glow_data = (unsigned char*)glow_texture + glow_texture->offsets[0];
	surface.count = face->numedges;
	D_FillSurfaceDynamicLights(surface, face);
}

void D_FillSurfaceRGBAColoredLightsData (dsurfacewithglow_t& surface, msurface_t* face, entity_t* entity, miptex_t* color_texture, miptex_t* glow_texture, int mips)
{
	surface.face = face;
	surface.model = entity->model;
	surface.width = color_texture->width;
	surface.height = color_texture->height;
	D_FillSurfaceSize(surface, sizeof(unsigned), mips);
	surface.data = (unsigned char*)color_texture + color_texture->offsets[0];
	surface.glow_data = (unsigned char*)glow_texture + glow_texture->offsets[0];
	surface.count = face->numedges;
	D_FillSurfaceDynamicLights(surface, face);
}

void D_FillSurfaceRGBANoGlowData (dsurface_t& surface, msurface_t* face, entity_t* entity, miptex_t* texture, int mips)
{
	surface.face = face;
	surface.model = entity->model;
	surface.width = texture->width;
	surface.height = texture->height;
	D_FillSurfaceSize(surface, sizeof(unsigned), mips);
	surface.data = (unsigned char*)texture + texture->offsets[0];
	surface.count = face->numedges;
	D_FillSurfaceDynamicLights(surface, face);
}

void D_FillSurfaceRGBANoGlowColoredLightsData (dsurface_t& surface, msurface_t* face, entity_t* entity, miptex_t* texture, int mips)
{
	surface.face = face;
	surface.model = entity->model;
	surface.width = texture->width;
	surface.height = texture->height;
	D_FillSurfaceSize(surface, sizeof(unsigned), mips);
	surface.data = (unsigned char*)texture + texture->offsets[0];
	surface.count = face->numedges;
	D_FillSurfaceDynamicLights(surface, face);
}

void D_FillSurfaceRotatedData (dsurfacerotated_t& surface, msurface_t* face, entity_t* entity, texture_t* texture, byte alpha, int mips)
{
	D_FillSurfaceData(surface, face, entity, texture, mips);
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	surface.yaw = entity->angles[YAW];
	surface.pitch = entity->angles[PITCH];
	surface.roll = entity->angles[ROLL];
	surface.alpha = alpha;
}

void D_FillSurfaceRotatedColoredLightsData (dsurfacerotated_t& surface, msurface_t* face, entity_t* entity, texture_t* texture, byte alpha, int mips)
{
	D_FillSurfaceColoredLightsData(surface, face, entity, texture, mips);
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	surface.yaw = entity->angles[YAW];
	surface.pitch = entity->angles[PITCH];
	surface.roll = entity->angles[ROLL];
	surface.alpha = alpha;
}

void D_FillSurfaceRotatedRGBAData (dsurfacerotatedwithglow_t& surface, msurface_t* face, entity_t* entity, miptex_t* texture, miptex_t* glow_texture, byte alpha, int mips)
{
	D_FillSurfaceRGBAData(surface, face, entity, texture, glow_texture, mips);
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	surface.yaw = entity->angles[YAW];
	surface.pitch = entity->angles[PITCH];
	surface.roll = entity->angles[ROLL];
	surface.alpha = alpha;
}

void D_FillSurfaceRotatedRGBAColoredLightsData (dsurfacerotatedwithglow_t& surface, msurface_t* face, entity_t* entity, miptex_t* texture, miptex_t* glow_texture, byte alpha, int mips)
{
	D_FillSurfaceRGBAColoredLightsData(surface, face, entity, texture, glow_texture, mips);
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	surface.yaw = entity->angles[YAW];
	surface.pitch = entity->angles[PITCH];
	surface.roll = entity->angles[ROLL];
	surface.alpha = alpha;
}

void D_FillSurfaceRotatedRGBANoGlowData (dsurfacerotated_t& surface, msurface_t* face, entity_t* entity, miptex_t* texture, byte alpha, int mips)
{
	D_FillSurfaceRGBANoGlowData(surface, face, entity, texture, mips);
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	surface.yaw = entity->angles[YAW];
	surface.pitch = entity->angles[PITCH];
	surface.roll = entity->angles[ROLL];
	surface.alpha = alpha;
}

void D_FillSurfaceRotatedRGBANoGlowColoredLightsData (dsurfacerotated_t& surface, msurface_t* face, entity_t* entity, miptex_t* texture, byte alpha, int mips)
{
	D_FillSurfaceRGBANoGlowColoredLightsData(surface, face, entity, texture, mips);
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	surface.yaw = entity->angles[YAW];
	surface.pitch = entity->angles[PITCH];
	surface.roll = entity->angles[ROLL];
	surface.alpha = alpha;
}

void D_AddSurfaceToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_surface++;
	if (d_lists_producing->last_surface >= d_lists_producing->surfaces.size())
	{
		d_lists_producing->surfaces.emplace_back();
	}
	auto& surface = d_lists_producing->surfaces[d_lists_producing->last_surface];
	D_FillSurfaceData(surface, face, entity, texture, MIPLEVELS);
}

void D_AddSurfaceColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_surface_colored_lights++;
	if (d_lists_producing->last_surface_colored_lights >= d_lists_producing->surfaces_colored_lights.size())
	{
		d_lists_producing->surfaces_colored_lights.emplace_back();
	}
	auto& surface = d_lists_producing->surfaces_colored_lights[d_lists_producing->last_surface_colored_lights];
	D_FillSurfaceColoredLightsData(surface, face, entity, texture, MIPLEVELS);
}

void D_AddSurfaceRGBAToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	auto glow_texture = texture->external_glow;
	if (glow_texture->width == 0 || glow_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_surface_rgba++;
	if (d_lists_producing->last_surface_rgba >= d_lists_producing->surfaces_rgba.size())
	{
		d_lists_producing->surfaces_rgba.emplace_back();
	}
	auto& surface = d_lists_producing->surfaces_rgba[d_lists_producing->last_surface_rgba];
	D_FillSurfaceRGBAData(surface, face, entity, color_texture, glow_texture, MIPLEVELS);
}

void D_AddSurfaceRGBAColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	auto glow_texture = texture->external_glow;
	if (glow_texture->width == 0 || glow_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_surface_rgba_colored_lights++;
	if (d_lists_producing->last_surface_rgba_colored_lights >= d_lists_producing->surfaces_rgba_colored_lights.size())
	{
		d_lists_producing->surfaces_rgba_colored_lights.emplace_back();
	}
	auto& surface = d_lists_producing->surfaces_rgba_colored_lights[d_lists_producing->last_surface_rgba_colored_lights];
	D_FillSurfaceRGBAColoredLightsData(surface, face, entity, color_texture, glow_texture, MIPLEVELS);
}

void D_AddSurfaceRGBANoGlowToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_surface_rgba_no_glow++;
	if (d_lists_producing->last_surface_rgba_no_glow >= d_lists_producing->surfaces_rgba_no_glow.size())
	{
		d_lists_producing->surfaces_rgba_no_glow.emplace_back();
	}
	auto& surface = d_lists_producing->surfaces_rgba_no_glow[d_lists_producing->last_surface_rgba_no_glow];
	D_FillSurfaceRGBANoGlowData(surface, face, entity, color_texture, MIPLEVELS);
}

void D_AddSurfaceRGBANoGlowColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_surface_rgba_no_glow_colored_lights++;
	if (d_lists_producing->last_surface_rgba_no_glow_colored_lights >= d_lists_producing->surfaces_rgba_no_glow_colored_lights.size())
	{
		d_lists_producing->surfaces_rgba_no_glow_colored_lights.emplace_back();
	}
	auto& surface = d_lists_producing->surfaces_rgba_no_glow_colored_lights[d_lists_producing->last_surface_rgba_no_glow_colored_lights];
	D_FillSurfaceRGBANoGlowColoredLightsData(surface, face, entity, color_texture, MIPLEVELS);
}

void D_AddSurfaceRotatedToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_surface_rotated++;
	if (d_lists_producing->last_surface_rotated >= d_lists_producing->surfaces_rotated.size())
	{
		d_lists_producing->surfaces_rotated.emplace_back();
	}
	auto& surface = d_lists_producing->surfaces_rotated[d_lists_producing->last_surface_rotated];
	D_FillSurfaceRotatedData(surface, face, entity, texture, alpha, MIPLEVELS);
}

void D_AddSurfaceRotatedColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_surface_rotated_colored_lights++;
	if (d_lists_producing->last_surface_rotated_colored_lights >= d_lists_producing->surfaces_rotated_colored_lights.size())
	{
		d_lists_producing->surfaces_rotated_colored_lights.emplace_back();
	}
	auto& surface = d_lists_producing->surfaces_rotated_colored_lights[d_lists_producing->last_surface_rotated_colored_lights];
	D_FillSurfaceRotatedColoredLightsData(surface, face, entity, texture, alpha, MIPLEVELS);
}

void D_AddSurfaceRotatedRGBAToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	auto glow_texture = texture->external_glow;
	if (glow_texture->width == 0 || glow_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_surface_rotated_rgba++;
	if (d_lists_producing->last_surface_rotated_rgba >= d_lists_producing->surfaces_rotated_rgba.size())
	{
		d_lists_producing->surfaces_rotated_rgba.emplace_back();
	}
	auto& surface = d_lists_producing->surfaces_rotated_rgba[d_lists_producing->last_surface_rotated_rgba];
	D_FillSurfaceRotatedRGBAData(surface, face, entity, color_texture, glow_texture, alpha, MIPLEVELS);
}

void D_AddSurfaceRotatedRGBAColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	auto glow_texture = texture->external_glow;
	if (glow_texture->width == 0 || glow_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_surface_rotated_rgba_colored_lights++;
	if (d_lists_producing->last_surface_rotated_rgba_colored_lights >= d_lists_producing->surfaces_rotated_rgba_colored_lights.size())
	{
		d_lists_producing->surfaces_rotated_rgba_colored_lights.emplace_back();
	}
	auto& surface = d_lists_producing->surfaces_rotated_rgba_colored_lights[d_lists_producing->last_surface_rotated_rgba_colored_lights];
	D_FillSurfaceRotatedRGBAColoredLightsData(surface, face, entity, color_texture, glow_texture, alpha, MIPLEVELS);
}

void D_AddSurfaceRotatedRGBANoGlowToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_surface_rotated_rgba_no_glow++;
	if (d_lists_producing->last_surface_rotated_rgba_no_glow >= d_lists_producing->surfaces_rotated_rgba_no_glow.size())
	{
		d_lists_producing->surfaces_rotated_rgba_no_glow.emplace_back();
	}
	auto& surface = d_lists_producing->surfaces_rotated_rgba_no_glow[d_lists_producing->last_surface_rotated_rgba_no_glow];
	D_FillSurfaceRotatedRGBANoGlowData(surface, face, entity, color_texture, alpha, MIPLEVELS);
}

void D_AddSurfaceRotatedRGBANoGlowColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_surface_rotated_rgba_no_glow_colored_lights++;
	if (d_lists_producing->last_surface_rotated_rgba_no_glow_colored_lights >= d_lists_producing->surfaces_rotated_rgba_no_glow_colored_lights.size())
	{
		d_lists_producing->surfaces_rotated_rgba_no_glow_colored_lights.emplace_back();
	}
	auto& surface = d_lists_producing->surfaces_rotated_rgba_no_glow_colored_lights[d_lists_producing->last_surface_rotated_rgba_no_glow_colored_lights];
	D_FillSurfaceRotatedRGBANoGlowColoredLightsData(surface, face, entity, color_texture, alpha, MIPLEVELS);
}

void D_AddFenceToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_fence++;
	if (d_lists_producing->last_fence >= d_lists_producing->fences.size())
	{
		d_lists_producing->fences.emplace_back();
	}
	auto& fence = d_lists_producing->fences[d_lists_producing->last_fence];
	D_FillSurfaceData(fence, face, entity, texture, MIPLEVELS);
}

void D_AddFenceColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_fence_colored_lights++;
	if (d_lists_producing->last_fence_colored_lights >= d_lists_producing->fences_colored_lights.size())
	{
		d_lists_producing->fences_colored_lights.emplace_back();
	}
	auto& fence = d_lists_producing->fences_colored_lights[d_lists_producing->last_fence_colored_lights];
	D_FillSurfaceColoredLightsData(fence, face, entity, texture, MIPLEVELS);
}

void D_AddFenceRGBAToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	auto glow_texture = texture->external_glow;
	if (glow_texture->width == 0 || glow_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_fence_rgba++;
	if (d_lists_producing->last_fence_rgba >= d_lists_producing->fences_rgba.size())
	{
		d_lists_producing->fences_rgba.emplace_back();
	}
	auto& fence = d_lists_producing->fences_rgba[d_lists_producing->last_fence_rgba];
	D_FillSurfaceRGBAData(fence, face, entity, color_texture, glow_texture, MIPLEVELS);
}

void D_AddFenceRGBAColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	auto glow_texture = texture->external_glow;
	if (glow_texture->width == 0 || glow_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_fence_rgba_colored_lights++;
	if (d_lists_producing->last_fence_rgba_colored_lights >= d_lists_producing->fences_rgba_colored_lights.size())
	{
		d_lists_producing->fences_rgba_colored_lights.emplace_back();
	}
	auto& fence = d_lists_producing->fences_rgba_colored_lights[d_lists_producing->last_fence_rgba_colored_lights];
	D_FillSurfaceRGBAColoredLightsData(fence, face, entity, color_texture, glow_texture, MIPLEVELS);
}

void D_AddFenceRGBANoGlowToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_fence_rgba_no_glow++;
	if (d_lists_producing->last_fence_rgba_no_glow >= d_lists_producing->fences_rgba_no_glow.size())
	{
		d_lists_producing->fences_rgba_no_glow.emplace_back();
	}
	auto& fence = d_lists_producing->fences_rgba_no_glow[d_lists_producing->last_fence_rgba_no_glow];
	D_FillSurfaceRGBANoGlowData(fence, face, entity, color_texture, MIPLEVELS);
}

void D_AddFenceRGBANoGlowColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_fence_rgba_no_glow_colored_lights++;
	if (d_lists_producing->last_fence_rgba_no_glow_colored_lights >= d_lists_producing->fences_rgba_no_glow_colored_lights.size())
	{
		d_lists_producing->fences_rgba_no_glow_colored_lights.emplace_back();
	}
	auto& fence = d_lists_producing->fences_rgba_no_glow_colored_lights[d_lists_producing->last_fence_rgba_no_glow_colored_lights];
	D_FillSurfaceRGBANoGlowColoredLightsData(fence, face, entity, color_texture, MIPLEVELS);
}

void D_AddFenceRotatedToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_fence_rotated++;
	if (d_lists_producing->last_fence_rotated >= d_lists_producing->fences_rotated.size())
	{
		d_lists_producing->fences_rotated.emplace_back();
	}
	auto& fence = d_lists_producing->fences_rotated[d_lists_producing->last_fence_rotated];
	D_FillSurfaceRotatedData(fence, face, entity, texture, alpha, MIPLEVELS);
}

void D_AddFenceRotatedColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_fence_rotated_colored_lights++;
	if (d_lists_producing->last_fence_rotated_colored_lights >= d_lists_producing->fences_rotated_colored_lights.size())
	{
		d_lists_producing->fences_rotated_colored_lights.emplace_back();
	}
	auto& fence = d_lists_producing->fences_rotated_colored_lights[d_lists_producing->last_fence_rotated_colored_lights];
	D_FillSurfaceRotatedColoredLightsData(fence, face, entity, texture, alpha, MIPLEVELS);
}

void D_AddFenceRotatedRGBAToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	auto glow_texture = texture->external_glow;
	if (glow_texture->width == 0 || glow_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_fence_rotated_rgba++;
	if (d_lists_producing->last_fence_rotated_rgba >= d_lists_producing->fences_rotated_rgba.size())
	{
		d_lists_producing->fences_rotated_rgba.emplace_back();
	}
	auto& fence = d_lists_producing->fences_rotated_rgba[d_lists_producing->last_fence_rotated_rgba];
	D_FillSurfaceRotatedRGBAData(fence, face, entity, color_texture, glow_texture, alpha, MIPLEVELS);
}

void D_AddFenceRotatedRGBAColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	auto glow_texture = texture->external_glow;
	if (glow_texture->width == 0 || glow_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_fence_rotated_rgba_colored_lights++;
	if (d_lists_producing->last_fence_rotated_rgba_colored_lights >= d_lists_producing->fences_rotated_rgba_colored_lights.size())
	{
		d_lists_producing->fences_rotated_rgba_colored_lights.emplace_back();
	}
	auto& fence = d_lists_producing->fences_rotated_rgba_colored_lights[d_lists_producing->last_fence_rotated_rgba_colored_lights];
	D_FillSurfaceRotatedRGBAColoredLightsData(fence, face, entity, color_texture, glow_texture, alpha, MIPLEVELS);
}

void D_AddFenceRotatedRGBANoGlowToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_fence_rotated_rgba_no_glow++;
	if (d_lists_producing->last_fence_rotated_rgba_no_glow >= d_lists_producing->fences_rotated_rgba_no_glow.size())
	{
		d_lists_producing->fences_rotated_rgba_no_glow.emplace_back();
	}
	auto& fence = d_lists_producing->fences_rotated_rgba_no_glow[d_lists_producing->last_fence_rotated_rgba_no_glow];
	D_FillSurfaceRotatedRGBANoGlowData(fence, face, entity, color_texture, alpha, MIPLEVELS);
}

void D_AddFenceRotatedRGBANoGlowColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_fence_rotated_rgba_no_glow_colored_lights++;
	if (d_lists_producing->last_fence_rotated_rgba_no_glow_colored_lights >= d_lists_producing->fences_rotated_rgba_no_glow_colored_lights.size())
	{
		d_lists_producing->fences_rotated_rgba_no_glow_colored_lights.emplace_back();
	}
	auto& fence = d_lists_producing->fences_rotated_rgba_no_glow_colored_lights[d_lists_producing->last_fence_rotated_rgba_no_glow_colored_lights];
	D_FillSurfaceRotatedRGBANoGlowColoredLightsData(fence, face, entity, color_texture, alpha, MIPLEVELS);
}

void D_FillTurbulentData (dturbulent_t& turbulent, msurface_t* face, entity_t* entity, texture_t* texture, int mips)
{
	turbulent.face = face;
	turbulent.model = entity->model;
	turbulent.width = texture->width;
	turbulent.height = texture->height;
	D_FillSurfaceSize(turbulent, 1, mips);
	turbulent.data = (unsigned char*)texture + texture->offsets[0];
	turbulent.count = face->numedges;
}

void D_FillTurbulentRGBAData (dturbulent_t& turbulent, msurface_t* face, entity_t* entity, miptex_t* texture, int mips)
{
	turbulent.face = face;
	turbulent.model = entity->model;
	turbulent.width = texture->width;
	turbulent.height = texture->height;
	D_FillSurfaceSize(turbulent, sizeof(unsigned), mips);
	turbulent.data = (unsigned char*)texture + texture->offsets[0];
	turbulent.count = face->numedges;
}

void D_AddTurbulentToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_turbulent++;
	if (d_lists_producing->last_turbulent >= d_lists_producing->turbulent.size())
	{
		d_lists_producing->turbulent.emplace_back();
	}
	auto& turbulent = d_lists_producing->turbulent[d_lists_producing->last_turbulent];
	D_FillTurbulentData(turbulent, face, entity, texture, MIPLEVELS);
}

void D_AddTurbulentRGBAToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_turbulent_rgba++;
	if (d_lists_producing->last_turbulent_rgba >= d_lists_producing->turbulent_rgba.size())
	{
		d_lists_producing->turbulent_rgba.emplace_back();
	}
	auto& turbulent = d_lists_producing->turbulent_rgba[d_lists_producing->last_turbulent_rgba];
	D_FillTurbulentRGBAData(turbulent, face, entity, color_texture, MIPLEVELS);
}

void D_AddTurbulentLitToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_turbulent_lit++;
	if (d_lists_producing->last_turbulent_lit >= d_lists_producing->turbulent_lit.size())
	{
		d_lists_producing->turbulent_lit.emplace_back();
	}
	auto& turbulent = d_lists_producing->turbulent_lit[d_lists_producing->last_turbulent_lit];
	D_FillTurbulentData(turbulent, face, entity, texture, MIPLEVELS);
	D_FillSurfaceDynamicLights(turbulent, face);
}

void D_AddTurbulentColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_turbulent_colored_lights++;
	if (d_lists_producing->last_turbulent_colored_lights >= d_lists_producing->turbulent_colored_lights.size())
	{
		d_lists_producing->turbulent_colored_lights.emplace_back();
	}
	auto& turbulent = d_lists_producing->turbulent_colored_lights[d_lists_producing->last_turbulent_colored_lights];
	D_FillTurbulentData(turbulent, face, entity, texture, MIPLEVELS);
	D_FillSurfaceDynamicLights(turbulent, face);
}

void D_AddTurbulentRGBALitToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_turbulent_rgba_lit++;
	if (d_lists_producing->last_turbulent_rgba_lit >= d_lists_producing->turbulent_rgba_lit.size())
	{
		d_lists_producing->turbulent_rgba_lit.emplace_back();
	}
	auto& turbulent = d_lists_producing->turbulent_rgba_lit[d_lists_producing->last_turbulent_rgba_lit];
	D_FillTurbulentRGBAData(turbulent, face, entity, color_texture, MIPLEVELS);
	D_FillSurfaceDynamicLights(turbulent, face);
}

void D_AddTurbulentRGBAColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_turbulent_rgba_colored_lights++;
	if (d_lists_producing->last_turbulent_rgba_colored_lights >= d_lists_producing->turbulent_rgba_colored_lights.size())
	{
		d_lists_producing->turbulent_rgba_colored_lights.emplace_back();
	}
	auto& turbulent = d_lists_producing->turbulent_rgba_colored_lights[d_lists_producing->last_turbulent_rgba_colored_lights];
	D_FillTurbulentRGBAData(turbulent, face, entity, color_texture, MIPLEVELS);
	D_FillSurfaceDynamicLights(turbulent, face);
}

void D_AddTurbulentRotatedToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_turbulent_rotated++;
	if (d_lists_producing->last_turbulent_rotated >= d_lists_producing->turbulent_rotated.size())
	{
		d_lists_producing->turbulent_rotated.emplace_back();
	}
	auto& turbulent = d_lists_producing->turbulent_rotated[d_lists_producing->last_turbulent_rotated];
	D_FillTurbulentData(turbulent, face, entity, texture, MIPLEVELS);
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	turbulent.yaw = entity->angles[YAW];
	turbulent.pitch = entity->angles[PITCH];
	turbulent.roll = entity->angles[ROLL];
	turbulent.alpha = alpha;
}

void D_AddTurbulentRotatedRGBAToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_turbulent_rotated_rgba++;
	if (d_lists_producing->last_turbulent_rotated_rgba >= d_lists_producing->turbulent_rotated_rgba.size())
	{
		d_lists_producing->turbulent_rotated_rgba.emplace_back();
	}
	auto& turbulent = d_lists_producing->turbulent_rotated_rgba[d_lists_producing->last_turbulent_rotated_rgba];
	D_FillTurbulentRGBAData(turbulent, face, entity, color_texture, MIPLEVELS);
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	turbulent.yaw = entity->angles[YAW];
	turbulent.pitch = entity->angles[PITCH];
	turbulent.roll = entity->angles[ROLL];
	turbulent.alpha = alpha;
}

void D_AddTurbulentRotatedLitToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_turbulent_rotated_lit++;
	if (d_lists_producing->last_turbulent_rotated_lit >= d_lists_producing->turbulent_rotated_lit.size())
	{
		d_lists_producing->turbulent_rotated_lit.emplace_back();
	}
	auto& turbulent = d_lists_producing->turbulent_rotated_lit[d_lists_producing->last_turbulent_rotated_lit];
	D_FillTurbulentData(turbulent, face, entity, texture, MIPLEVELS);
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	turbulent.yaw = entity->angles[YAW];
	turbulent.pitch = entity->angles[PITCH];
	turbulent.roll = entity->angles[ROLL];
	turbulent.alpha = alpha;
	D_FillSurfaceDynamicLights(turbulent, face);
}

void D_AddTurbulentRotatedColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_turbulent_rotated_colored_lights++;
	if (d_lists_producing->last_turbulent_rotated_colored_lights >= d_lists_producing->turbulent_rotated_colored_lights.size())
	{
		d_lists_producing->turbulent_rotated_colored_lights.emplace_back();
	}
	auto& turbulent = d_lists_producing->turbulent_rotated_colored_lights[d_lists_producing->last_turbulent_rotated_colored_lights];
	D_FillTurbulentData(turbulent, face, entity, texture, MIPLEVELS);
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	turbulent.yaw = entity->angles[YAW];
	turbulent.pitch = entity->angles[PITCH];
	turbulent.roll = entity->angles[ROLL];
	turbulent.alpha = alpha;
	D_FillSurfaceDynamicLights(turbulent, face);
}

void D_AddTurbulentRotatedRGBALitToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_turbulent_rotated_rgba_lit++;
	if (d_lists_producing->last_turbulent_rotated_rgba_lit >= d_lists_producing->turbulent_rotated_rgba_lit.size())
	{
		d_lists_producing->turbulent_rotated_rgba_lit.emplace_back();
	}
	auto& turbulent = d_lists_producing->turbulent_rotated_rgba_lit[d_lists_producing->last_turbulent_rotated_rgba_lit];
	D_FillTurbulentRGBAData(turbulent, face, entity, color_texture, MIPLEVELS);
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	turbulent.yaw = entity->angles[YAW];
	turbulent.pitch = entity->angles[PITCH];
	turbulent.roll = entity->angles[ROLL];
	turbulent.alpha = alpha;
	D_FillSurfaceDynamicLights(turbulent, face);
}

void D_AddTurbulentRotatedRGBAColoredLightsToLists (msurface_t* face, texture_t* texture, entity_t* entity, byte alpha)
{
	if (face->numedges < 3 || texture->width == 0 || texture->height == 0)
	{
		return;
	}
	auto color_texture = texture->external_color;
	if (color_texture->width == 0 || color_texture->height == 0)
	{
		return;
	}
	d_lists_producing->last_turbulent_rotated_rgba_colored_lights++;
	if (d_lists_producing->last_turbulent_rotated_rgba_colored_lights >= d_lists_producing->turbulent_rotated_rgba_colored_lights.size())
	{
		d_lists_producing->turbulent_rotated_rgba_colored_lights.emplace_back();
	}
	auto& turbulent = d_lists_producing->turbulent_rotated_rgba_colored_lights[d_lists_producing->last_turbulent_rotated_rgba_colored_lights];
	D_FillTurbulentRGBAData(turbulent, face, entity, color_texture, MIPLEVELS);
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	turbulent.yaw = entity->angles[YAW];
	turbulent.pitch = entity->angles[PITCH];
	turbulent.roll = entity->angles[ROLL];
	turbulent.alpha = alpha;
	D_FillSurfaceDynamicLights(turbulent, face);
}

void D_AddSpriteToLists (vec5_t* pverts, spritedesc_t* spritedesc)
{
	d_lists_producing->last_sprite++;
	if (d_lists_producing->last_sprite >= d_lists_producing->sprites.size())
	{
		d_lists_producing->sprites.emplace_back();
	}
	auto& sprite = d_lists_producing->sprites[d_lists_producing->last_sprite];
	sprite.width = spritedesc->pspriteframe->width;
	sprite.height = spritedesc->pspriteframe->height;
	sprite.size = sprite.width * sprite.height;
	sprite.data = &spritedesc->pspriteframe->pixels[0];
	sprite.first_vertex = (d_lists_producing->last_textured_vertex + 1) / 3;
	sprite.count = 4;
	auto new_size = d_lists_producing->last_textured_vertex + 1 + 3 * 4;
	if (d_lists_producing->textured_vertices.size() < new_size)
	{
		d_lists_producing->textured_vertices.resize(new_size);
	}
	new_size = d_lists_producing->last_textured_attribute + 1 + 2 * 4;
	if (d_lists_producing->textured_attributes.size() < new_size)
	{
		d_lists_producing->textured_attributes.resize(new_size);
	}
	auto x = pverts[0][0];
	auto y = pverts[0][1];
	auto z = pverts[0][2];
	auto s = pverts[0][3] / sprite.width;
	auto t = pverts[0][4] / sprite.height;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = x;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = y;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = z;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = s;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = t;
	x = pverts[1][0];
	y = pverts[1][1];
	z = pverts[1][2];
	s = pverts[1][3] / sprite.width;
	t = pverts[1][4] / sprite.height;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = x;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = y;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = z;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = s;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = t;
	x = pverts[2][0];
	y = pverts[2][1];
	z = pverts[2][2];
	s = pverts[2][3] / sprite.width;
	t = pverts[2][4] / sprite.height;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = x;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = y;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = z;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = s;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = t;
	x = pverts[3][0];
	y = pverts[3][1];
	z = pverts[3][2];
	s = pverts[3][3] / sprite.width;
	t = pverts[3][4] / sprite.height;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = x;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = y;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = z;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = s;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = t;
}

void D_FillAliasData(daliascoloredlights_t& alias, aliashdr_t* aliashdr, mdl_t* mdl, maliasskindesc_t* skindesc, trivertx_t* apverts)
{
	alias.aliashdr = aliashdr;
	alias.width = mdl->skinwidth;
	alias.height = mdl->skinheight;
	alias.size = alias.width * alias.height;
	alias.data = (byte *)aliashdr + skindesc->skin;
	alias.apverts = apverts;
	alias.texture_coordinates = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);
	alias.vertex_count = mdl->numverts;
	alias.first_light = d_lists_producing->last_alias_light + 1;
	alias.count = mdl->numtris * 3;
}

void D_FillAliasData(dalias_t& alias, aliashdr_t* aliashdr, mdl_t* mdl, maliasskindesc_t* skindesc, trivertx_t* apverts, byte* colormap)
{
	D_FillAliasData((daliascoloredlights_t&)alias, aliashdr, mdl, skindesc, apverts);
	if (colormap == vid.colormap)
	{
		alias.colormap = nullptr;
	}
	else
	{
		alias.colormap = colormap;
	}
}

void D_FillViewmodelData(dviewmodel_t& viewmodel, aliashdr_t* aliashdr, mdl_t* mdl, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* apverts)
{
	D_FillAliasData((daliascoloredlights_t&)viewmodel, aliashdr, mdl, skindesc, apverts);
	if (colormap == vid.colormap)
	{
		viewmodel.colormap = nullptr;
	}
	else
	{
		viewmodel.colormap = colormap;
	}
}

void D_FillAliasTransform (daliascoloredlights_t& alias, entity_t* entity, mdl_t* mdl)
{
	vec3_t angles;
	angles[ROLL] = entity->angles[ROLL];
	angles[PITCH] = -entity->angles[PITCH];
	angles[YAW] = entity->angles[YAW];
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
	t2matrix[0][3] = entity->origin[0];
	t2matrix[1][3] = entity->origin[1];
	t2matrix[2][3] = entity->origin[2];
	R_ConcatTransforms (t2matrix, tmatrix, alias.transform);
}

void D_FillAliasAttributes (trivertx_t* apverts, mdl_t* mdl)
{
	auto new_size = d_lists_producing->last_alias_light + 1 + 2 * mdl->numverts;
	if (d_lists_producing->alias_lights.size() < new_size)
	{
		d_lists_producing->alias_lights.resize(new_size);
	}
	auto vertex = apverts;
	auto attribute = d_lists_producing->alias_lights.data() + d_lists_producing->last_alias_light + 1;
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

		auto light = (float)(temp & VID_CMASK) / 256;

		*attribute++ = light;
		*attribute++ = light;
		vertex++;
	}
	d_lists_producing->last_alias_light += 2 * mdl->numverts;
}

void D_FillAliasAlphaAttributes (trivertx_t* apverts, mdl_t* mdl, unsigned char alpha)
{
	auto new_size = d_lists_producing->last_alias_light + 1 + 2 * 2 * mdl->numverts;
	if (d_lists_producing->alias_lights.size() < new_size)
	{
		d_lists_producing->alias_lights.resize(new_size);
	}
	auto vertex = apverts;
	auto attribute = d_lists_producing->alias_lights.data() + d_lists_producing->last_alias_light + 1;
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

		auto light = (float)(temp & VID_CMASK) / 256;

		*attribute++ = light;
		*attribute++ = alpha;
		*attribute++ = light;
		*attribute++ = alpha;
		vertex++;
	}
	d_lists_producing->last_alias_light += 2 * 2 * mdl->numverts;
}

void D_FillAliasColoredLightsAttributes (trivertx_t* apverts, mdl_t* mdl)
{
	auto new_size = d_lists_producing->last_alias_light + 1 + 2 * 3 * mdl->numverts;
	if (d_lists_producing->alias_lights.size() < new_size)
	{
		d_lists_producing->alias_lights.resize(new_size);
	}
	auto vertex = apverts;
	auto attribute = d_lists_producing->alias_lights.data() + d_lists_producing->last_alias_light + 1;
	vec3_t lightvec { r_plightvec[0], r_plightvec[1], r_plightvec[2] };
	for (auto i = 0; i < mdl->numverts; i++)
	{
	// lighting
		float* plightnormal = r_avertexnormals[vertex->lightnormalindex];
		auto lightcos = DotProduct (plightnormal, lightvec);
		auto temp = r_ambientcoloredlight;

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

		*attribute++ = (float)(std::max(VID_CMAX - temp.color[0], 0));
		*attribute++ = (float)(std::max(VID_CMAX - temp.color[1], 0));
		*attribute++ = (float)(std::max(VID_CMAX - temp.color[2], 0));
		*attribute++ = (float)(std::max(VID_CMAX - temp.color[0], 0));
		*attribute++ = (float)(std::max(VID_CMAX - temp.color[1], 0));
		*attribute++ = (float)(std::max(VID_CMAX - temp.color[2], 0));
		vertex++;
	}
	d_lists_producing->last_alias_light += 2 * 3 * mdl->numverts;
}

void D_FillAliasAlphaColoredLightsAttributes (trivertx_t* apverts, mdl_t* mdl, unsigned char alpha)
{
	auto new_size = d_lists_producing->last_alias_light + 1 + 2 * 4 * mdl->numverts;
	if (d_lists_producing->alias_lights.size() < new_size)
	{
		d_lists_producing->alias_lights.resize(new_size);
	}
	auto vertex = apverts;
	auto attribute = d_lists_producing->alias_lights.data() + d_lists_producing->last_alias_light + 1;
	vec3_t lightvec { r_plightvec[0], r_plightvec[1], r_plightvec[2] };
	for (auto i = 0; i < mdl->numverts; i++)
	{
	// lighting
		float* plightnormal = r_avertexnormals[vertex->lightnormalindex];
		auto lightcos = DotProduct (plightnormal, lightvec);
		auto temp = r_ambientcoloredlight;

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

		*attribute++ = (float)(std::max(VID_CMAX - temp.color[0], 0));
		*attribute++ = (float)(std::max(VID_CMAX - temp.color[1], 0));
		*attribute++ = (float)(std::max(VID_CMAX - temp.color[2], 0));
		*attribute++ = alpha;
		*attribute++ = (float)(std::max(VID_CMAX - temp.color[0], 0));
		*attribute++ = (float)(std::max(VID_CMAX - temp.color[1], 0));
		*attribute++ = (float)(std::max(VID_CMAX - temp.color[2], 0));
		*attribute++ = alpha;
		vertex++;
	}
	d_lists_producing->last_alias_light += 2 * 4 * mdl->numverts;
}

void D_AddAliasToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists_producing->last_alias++;
	if (d_lists_producing->last_alias >= d_lists_producing->alias.size())
	{
		d_lists_producing->alias.emplace_back();
	}
	auto& alias = d_lists_producing->alias[d_lists_producing->last_alias];
	D_FillAliasData(alias, aliashdr, mdl, skindesc, apverts, entity->colormap);
	D_FillAliasTransform(alias, entity, mdl);
	D_FillAliasAttributes(apverts, mdl);
}

void D_AddAliasAlphaToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists_producing->last_alias_alpha++;
	if (d_lists_producing->last_alias_alpha >= d_lists_producing->alias_alpha.size())
	{
		d_lists_producing->alias_alpha.emplace_back();
	}
	auto& alias = d_lists_producing->alias_alpha[d_lists_producing->last_alias_alpha];
	D_FillAliasData(alias, aliashdr, mdl, skindesc, apverts, entity->colormap);
	D_FillAliasTransform(alias, entity, mdl);
	D_FillAliasAlphaAttributes(apverts, mdl, entity->alpha);
}

void D_AddAliasColoredLightsToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists_producing->last_alias_colored_lights++;
	if (d_lists_producing->last_alias_colored_lights >= d_lists_producing->alias_colored_lights.size())
	{
		d_lists_producing->alias_colored_lights.emplace_back();
	}
	auto& alias = d_lists_producing->alias_colored_lights[d_lists_producing->last_alias_colored_lights];
	D_FillAliasData(alias, aliashdr, mdl, skindesc, apverts);
	D_FillAliasTransform(alias, entity, mdl);
	D_FillAliasColoredLightsAttributes(apverts, mdl);
}

void D_AddAliasAlphaColoredLightsToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists_producing->last_alias_alpha_colored_lights++;
	if (d_lists_producing->last_alias_alpha_colored_lights >= d_lists_producing->alias_alpha_colored_lights.size())
	{
		d_lists_producing->alias_alpha_colored_lights.emplace_back();
	}
	auto& alias = d_lists_producing->alias_alpha_colored_lights[d_lists_producing->last_alias_alpha_colored_lights];
	D_FillAliasData(alias, aliashdr, mdl, skindesc, apverts);
	D_FillAliasTransform(alias, entity, mdl);
	D_FillAliasAlphaColoredLightsAttributes(apverts, mdl, entity->alpha);
}

void D_AddAliasHoleyToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists_producing->last_alias_holey++;
	if (d_lists_producing->last_alias_holey >= d_lists_producing->alias_holey.size())
	{
		d_lists_producing->alias_holey.emplace_back();
	}
	auto& alias = d_lists_producing->alias_holey[d_lists_producing->last_alias_holey];
	D_FillAliasData(alias, aliashdr, mdl, skindesc, apverts, entity->colormap);
	D_FillAliasTransform(alias, entity, mdl);
	D_FillAliasAttributes(apverts, mdl);
}

void D_AddAliasHoleyAlphaToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists_producing->last_alias_holey_alpha++;
	if (d_lists_producing->last_alias_holey_alpha >= d_lists_producing->alias_holey_alpha.size())
	{
		d_lists_producing->alias_holey_alpha.emplace_back();
	}
	auto& alias = d_lists_producing->alias_holey_alpha[d_lists_producing->last_alias_holey_alpha];
	D_FillAliasData(alias, aliashdr, mdl, skindesc, apverts, entity->colormap);
	D_FillAliasTransform(alias, entity, mdl);
	D_FillAliasAlphaAttributes(apverts, mdl, entity->alpha);
}

void D_AddAliasHoleyColoredLightsToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists_producing->last_alias_holey_colored_lights++;
	if (d_lists_producing->last_alias_holey_colored_lights >= d_lists_producing->alias_holey_colored_lights.size())
	{
		d_lists_producing->alias_holey_colored_lights.emplace_back();
	}
	auto& alias = d_lists_producing->alias_holey_colored_lights[d_lists_producing->last_alias_holey_colored_lights];
	D_FillAliasData(alias, aliashdr, mdl, skindesc, apverts);
	D_FillAliasTransform(alias, entity, mdl);
	D_FillAliasColoredLightsAttributes(apverts, mdl);
}

void D_AddAliasHoleyAlphaColoredLightsToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists_producing->last_alias_holey_alpha_colored_lights++;
	if (d_lists_producing->last_alias_holey_alpha_colored_lights >= d_lists_producing->alias_holey_alpha_colored_lights.size())
	{
		d_lists_producing->alias_holey_alpha_colored_lights.emplace_back();
	}
	auto& alias = d_lists_producing->alias_holey_alpha_colored_lights[d_lists_producing->last_alias_holey_alpha_colored_lights];
	D_FillAliasData(alias, aliashdr, mdl, skindesc, apverts);
	D_FillAliasTransform(alias, entity, mdl);
	D_FillAliasAlphaColoredLightsAttributes(apverts, mdl, entity->alpha);
}

void D_FillViewmodelTransforms (dviewmodelcoloredlights_t& viewmodel, entity_t* entity, mdl_t* mdl)
{
	vec3_t angles;
	angles[ROLL] = entity->angles[ROLL];
	angles[PITCH] = -entity->angles[PITCH];
	angles[YAW] = entity->angles[YAW];
	vec3_t forward, right, up;
	AngleVectors (angles, forward, right, up);
	viewmodel.transform[0][0] = mdl->scale[0];
	viewmodel.transform[1][1] = mdl->scale[1];
	viewmodel.transform[2][2] = mdl->scale[2];
	viewmodel.transform[0][3] = mdl->scale_origin[0];
	viewmodel.transform[1][3] = mdl->scale_origin[1];
	viewmodel.transform[2][3] = mdl->scale_origin[2];
	for (auto i = 0; i < 3; i++)
	{
		viewmodel.transform2[i][0] = forward[i];
		viewmodel.transform2[i][1] = -right[i];
		viewmodel.transform2[i][2] = up[i];
	}
	viewmodel.transform2[0][3] = entity->origin[0] + r_modelorg_delta[0];
	viewmodel.transform2[1][3] = entity->origin[1] + r_modelorg_delta[1];
	viewmodel.transform2[2][3] = entity->origin[2] + r_modelorg_delta[2];
}

void D_AddViewmodelToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists_producing->last_viewmodel++;
	if (d_lists_producing->last_viewmodel >= d_lists_producing->viewmodels.size())
	{
		d_lists_producing->viewmodels.emplace_back();
	}
	auto& viewmodel = d_lists_producing->viewmodels[d_lists_producing->last_viewmodel];
	D_FillViewmodelData(viewmodel, aliashdr, mdl, skindesc, entity->colormap, apverts);
	D_FillViewmodelTransforms(viewmodel, entity, mdl);
	D_FillAliasAttributes(apverts, mdl);
}

void D_AddViewmodelColoredLightsToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists_producing->last_viewmodel_colored_lights++;
	if (d_lists_producing->last_viewmodel_colored_lights >= d_lists_producing->viewmodels_colored_lights.size())
	{
		d_lists_producing->viewmodels_colored_lights.emplace_back();
	}
	auto& viewmodel = d_lists_producing->viewmodels_colored_lights[d_lists_producing->last_viewmodel_colored_lights];
	D_FillAliasData(viewmodel, aliashdr, mdl, skindesc, apverts);
	D_FillViewmodelTransforms(viewmodel, entity, mdl);
	D_FillAliasColoredLightsAttributes(apverts, mdl);
}

void D_AddViewmodelHoleyToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists_producing->last_viewmodel_holey++;
	if (d_lists_producing->last_viewmodel_holey >= d_lists_producing->viewmodels_holey.size())
	{
		d_lists_producing->viewmodels_holey.emplace_back();
	}
	auto& viewmodel = d_lists_producing->viewmodels_holey[d_lists_producing->last_viewmodel_holey];
	D_FillViewmodelData(viewmodel, aliashdr, mdl, skindesc, entity->colormap, apverts);
	D_FillViewmodelTransforms(viewmodel, entity, mdl);
	D_FillAliasAttributes(apverts, mdl);
}

void D_AddViewmodelHoleyColoredLightsToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists_producing->last_viewmodel_holey_colored_lights++;
	if (d_lists_producing->last_viewmodel_holey_colored_lights >= d_lists_producing->viewmodels_holey_colored_lights.size())
	{
		d_lists_producing->viewmodels_holey_colored_lights.emplace_back();
	}
	auto& viewmodel = d_lists_producing->viewmodels_holey_colored_lights[d_lists_producing->last_viewmodel_holey_colored_lights];
	D_FillAliasData(viewmodel, aliashdr, mdl, skindesc, apverts);
	D_FillViewmodelTransforms(viewmodel, entity, mdl);
	D_FillAliasColoredLightsAttributes(apverts, mdl);
}

void D_AddParticleToLists (particle_t* part)
{
	auto new_size = d_lists_producing->last_particle + 1 + 4;
	if (d_lists_producing->particles.size() < new_size)
	{
		d_lists_producing->particles.resize(new_size);
	}
	auto x = part->org[0];
	auto y = part->org[1];
	auto z = part->org[2];
	d_lists_producing->last_particle++;
	d_lists_producing->particles[d_lists_producing->last_particle] = x;
	d_lists_producing->last_particle++;
	d_lists_producing->particles[d_lists_producing->last_particle] = y;
	d_lists_producing->last_particle++;
	d_lists_producing->particles[d_lists_producing->last_particle] = z;
	d_lists_producing->last_particle++;
	d_lists_producing->particles[d_lists_producing->last_particle] = part->color;
}

void D_FillSkyData (dsky_t& sky)
{
    constexpr float left = 0;
    constexpr float right = 1;
    constexpr float top = 0;
    constexpr float bottom = 1;
	sky.first_vertex = (d_lists_producing->last_textured_vertex + 1) / 3;
	sky.count = 4;
	auto new_size = d_lists_producing->last_textured_vertex + 1 + 3 * 4;
	if (d_lists_producing->textured_vertices.size() < new_size)
	{
		d_lists_producing->textured_vertices.resize(new_size);
	}
	new_size = d_lists_producing->last_textured_attribute + 1 + 2 * 4;
	if (d_lists_producing->textured_attributes.size() < new_size)
	{
		d_lists_producing->textured_attributes.resize(new_size);
	}
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = left;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = top;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = 1;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = left;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = top;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = right;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = top;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = 1;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = right;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = top;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = left;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = bottom;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = 1;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = left;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = bottom;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = right;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = bottom;
	d_lists_producing->last_textured_vertex++;
	d_lists_producing->textured_vertices[d_lists_producing->last_textured_vertex] = 1;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = right;
	d_lists_producing->last_textured_attribute++;
	d_lists_producing->textured_attributes[d_lists_producing->last_textured_attribute] = bottom;
}

void D_AddSkyToLists (skydesc_t& skydesc)
{
	if (d_lists_producing->last_sky >= 0)
	{
		return;
	}
	d_lists_producing->last_sky++;
	if (d_lists_producing->last_sky >= d_lists_producing->sky.size())
	{
		d_lists_producing->sky.emplace_back();
	}
	auto& sky = d_lists_producing->sky[d_lists_producing->last_sky];
	sky.width = 128;
	sky.height = 128;
	sky.size = sky.width * sky.height;
	sky.data = skydesc.source;
	D_FillSkyData(sky);
}

void D_AddSkyRGBAToLists (skydesc_t& skydesc)
{
	if (d_lists_producing->last_sky_rgba >= 0)
	{
		return;
	}
	d_lists_producing->last_sky_rgba++;
	if (d_lists_producing->last_sky_rgba >= d_lists_producing->sky_rgba.size())
	{
		d_lists_producing->sky_rgba.emplace_back();
	}
	auto& sky = d_lists_producing->sky_rgba[d_lists_producing->last_sky_rgba];
	sky.width = skydesc.widthRGBA;
	sky.height = skydesc.heightRGBA;
	sky.size = sky.width * 2 * sky.height * (int)sizeof(unsigned);
	sky.data = (byte*)skydesc.sourceRGBA;
	D_FillSkyData(sky);
}

void D_AddSkyboxToLists (mtexinfo_t* textures)
{
    if (d_lists_producing->last_skybox >= 0)
	{
		return;
	}
	d_lists_producing->last_skybox++;
	if (d_lists_producing->last_skybox >= d_lists_producing->skyboxes.size())
	{
		d_lists_producing->skyboxes.emplace_back();
	}
	auto& sky = d_lists_producing->skyboxes[d_lists_producing->last_skybox];
	sky.textures = textures;
}

void D_AddColoredSurfaceToLists (msurface_t* face, entity_t* entity, int color)
{
	if (face->numedges < 3)
	{
		return;
	}
	auto new_size = d_lists_producing->last_colored_vertex + 1 + 3 * face->numedges;
	if (d_lists_producing->colored_vertices.size() < new_size)
	{
		d_lists_producing->colored_vertices.resize(new_size);
	}
	new_size = d_lists_producing->last_colored_color + 1 + face->numedges;
	if (d_lists_producing->colored_colors.size() < new_size)
	{
		d_lists_producing->colored_colors.resize(new_size);
	}
	auto first_vertex = (d_lists_producing->last_colored_vertex + 1) / 3;
	if (first_vertex + face->numedges <= UPPER_8BIT_LIMIT)
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
		new_size = d_lists_producing->last_colored_index8 + 1 + (face->numedges - 2) * 3;
		if (d_lists_producing->colored_indices8.size() < new_size)
		{
			d_lists_producing->colored_indices8.resize(new_size);
		}
		auto& vertex = entity->model->vertexes[index];
		auto x = vertex.position[0];
		auto y = vertex.position[1];
		auto z = vertex.position[2];
		auto target = d_lists_producing->colored_vertices.data() + d_lists_producing->last_colored_vertex + 1;
		*target++ = x;
		*target++ = y;
		*target = z;
		target = d_lists_producing->colored_colors.data() + d_lists_producing->last_colored_color + 1;
		*target = color;
		d_lists_producing->last_colored_index8++;
		d_lists_producing->colored_indices8[d_lists_producing->last_colored_index8] = first_vertex;
		auto next_front = 0;
		auto next_back = face->numedges;
		auto use_back = false;
		unsigned char previous_index = 0;
		unsigned char before_previous_index;
		for (auto i = 1; i < face->numedges; i++)
		{
			unsigned char current_index;
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
			target = d_lists_producing->colored_vertices.data() + d_lists_producing->last_colored_vertex + 1 + current_index * 3;
			*target++ = x;
			*target++ = y;
			*target = z;
			target = d_lists_producing->colored_colors.data() + d_lists_producing->last_colored_color + 1 + current_index;
			*target = color;
			if (i >= 3)
			{
				if (use_back)
				{
					d_lists_producing->last_colored_index8++;
					d_lists_producing->colored_indices8[d_lists_producing->last_colored_index8] = first_vertex + previous_index;
					d_lists_producing->last_colored_index8++;
					d_lists_producing->colored_indices8[d_lists_producing->last_colored_index8] = first_vertex + before_previous_index;
				}
				else
				{
					d_lists_producing->last_colored_index8++;
					d_lists_producing->colored_indices8[d_lists_producing->last_colored_index8] = first_vertex + before_previous_index;
					d_lists_producing->last_colored_index8++;
					d_lists_producing->colored_indices8[d_lists_producing->last_colored_index8] = first_vertex + previous_index;
				}
			}
			d_lists_producing->last_colored_index8++;
			d_lists_producing->colored_indices8[d_lists_producing->last_colored_index8] = first_vertex + current_index;
			before_previous_index = previous_index;
			previous_index = current_index;
		}
	}
	else if (first_vertex + face->numedges <= UPPER_16BIT_LIMIT)
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
		new_size = d_lists_producing->last_colored_index16 + 1 + (face->numedges - 2) * 3;
		if (d_lists_producing->colored_indices16.size() < new_size)
		{
			d_lists_producing->colored_indices16.resize(new_size);
		}
		auto& vertex = entity->model->vertexes[index];
		auto x = vertex.position[0];
		auto y = vertex.position[1];
		auto z = vertex.position[2];
		auto target = d_lists_producing->colored_vertices.data() + d_lists_producing->last_colored_vertex + 1;
		*target++ = x;
		*target++ = y;
		*target = z;
		target = d_lists_producing->colored_colors.data() + d_lists_producing->last_colored_color + 1;
		*target = color;
		d_lists_producing->last_colored_index16++;
		d_lists_producing->colored_indices16[d_lists_producing->last_colored_index16] = first_vertex;
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
			target = d_lists_producing->colored_vertices.data() + d_lists_producing->last_colored_vertex + 1 + current_index * 3;
			*target++ = x;
			*target++ = y;
			*target = z;
			target = d_lists_producing->colored_colors.data() + d_lists_producing->last_colored_color + 1 + current_index;
			*target = color;
			if (i >= 3)
			{
				if (use_back)
				{
					d_lists_producing->last_colored_index16++;
					d_lists_producing->colored_indices16[d_lists_producing->last_colored_index16] = first_vertex + previous_index;
					d_lists_producing->last_colored_index16++;
					d_lists_producing->colored_indices16[d_lists_producing->last_colored_index16] = first_vertex + before_previous_index;
				}
				else
				{
					d_lists_producing->last_colored_index16++;
					d_lists_producing->colored_indices16[d_lists_producing->last_colored_index16] = first_vertex + before_previous_index;
					d_lists_producing->last_colored_index16++;
					d_lists_producing->colored_indices16[d_lists_producing->last_colored_index16] = first_vertex + previous_index;
				}
			}
			d_lists_producing->last_colored_index16++;
			d_lists_producing->colored_indices16[d_lists_producing->last_colored_index16] = first_vertex + current_index;
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
		new_size = d_lists_producing->last_colored_index32 + 1 + (face->numedges - 2) * 3;
		if (d_lists_producing->colored_indices32.size() < new_size)
		{
			d_lists_producing->colored_indices32.resize(new_size);
		}
		auto& vertex = entity->model->vertexes[index];
		auto x = vertex.position[0];
		auto y = vertex.position[1];
		auto z = vertex.position[2];
		auto target = d_lists_producing->colored_vertices.data() + d_lists_producing->last_colored_vertex + 1;
		*target++ = x;
		*target++ = y;
		*target = z;
		target = d_lists_producing->colored_colors.data() + d_lists_producing->last_colored_color + 1;
		*target = color;
		d_lists_producing->last_colored_index32++;
		d_lists_producing->colored_indices32[d_lists_producing->last_colored_index32] = first_vertex;
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
			target = d_lists_producing->colored_vertices.data() + d_lists_producing->last_colored_vertex + 1 + current_index * 3;
			*target++ = x;
			*target++ = y;
			*target = z;
			target = d_lists_producing->colored_colors.data() + d_lists_producing->last_colored_color + 1 + current_index;
			*target = color;
			if (i >= 3)
			{
				if (use_back)
				{
					d_lists_producing->last_colored_index32++;
					d_lists_producing->colored_indices32[d_lists_producing->last_colored_index32] = first_vertex + previous_index;
					d_lists_producing->last_colored_index32++;
					d_lists_producing->colored_indices32[d_lists_producing->last_colored_index32] = first_vertex + before_previous_index;
				}
				else
				{
					d_lists_producing->last_colored_index32++;
					d_lists_producing->colored_indices32[d_lists_producing->last_colored_index32] = first_vertex + before_previous_index;
					d_lists_producing->last_colored_index32++;
					d_lists_producing->colored_indices32[d_lists_producing->last_colored_index32] = first_vertex + previous_index;
				}
			}
			d_lists_producing->last_colored_index32++;
			d_lists_producing->colored_indices32[d_lists_producing->last_colored_index32] = first_vertex + current_index;
			before_previous_index = previous_index;
			previous_index = current_index;
		}
	}
	d_lists_producing->last_colored_vertex += 3 * face->numedges;
	d_lists_producing->last_colored_color += face->numedges;
}

void D_AddCutoutSurfaceToLists (msurface_t* face, entity_t* entity)
{
	if (face->numedges < 3)
	{
		return;
	}
	auto new_size = d_lists_producing->last_cutout_vertex + 1 + 3 * face->numedges;
	if (d_lists_producing->cutout_vertices.size() < new_size)
	{
		d_lists_producing->cutout_vertices.resize(new_size);
	}
	auto first_vertex = (d_lists_producing->last_cutout_vertex + 1) / 3;
	if (first_vertex + face->numedges <= UPPER_8BIT_LIMIT)
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
		new_size = d_lists_producing->last_cutout_index8 + 1 + (face->numedges - 2) * 3;
		if (d_lists_producing->cutout_indices8.size() < new_size)
		{
			d_lists_producing->cutout_indices8.resize(new_size);
		}
		auto& vertex = entity->model->vertexes[index];
		auto x = vertex.position[0];
		auto y = vertex.position[1];
		auto z = vertex.position[2];
		auto target = d_lists_producing->cutout_vertices.data() + d_lists_producing->last_cutout_vertex + 1;
		*target++ = x;
		*target++ = y;
		*target = z;
		d_lists_producing->last_cutout_index8++;
		d_lists_producing->cutout_indices8[d_lists_producing->last_cutout_index8] = first_vertex;
		auto next_front = 0;
		auto next_back = face->numedges;
		auto use_back = false;
		unsigned char previous_index = 0;
		unsigned char before_previous_index;
		for (auto i = 1; i < face->numedges; i++)
		{
			unsigned char current_index;
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
			target = d_lists_producing->cutout_vertices.data() + d_lists_producing->last_cutout_vertex + 1 + current_index * 3;
			*target++ = x;
			*target++ = y;
			*target = z;
			if (i >= 3)
			{
				if (use_back)
				{
					d_lists_producing->last_cutout_index8++;
					d_lists_producing->cutout_indices8[d_lists_producing->last_cutout_index8] = first_vertex + previous_index;
					d_lists_producing->last_cutout_index8++;
					d_lists_producing->cutout_indices8[d_lists_producing->last_cutout_index8] = first_vertex + before_previous_index;
				}
				else
				{
					d_lists_producing->last_cutout_index8++;
					d_lists_producing->cutout_indices8[d_lists_producing->last_cutout_index8] = first_vertex + before_previous_index;
					d_lists_producing->last_cutout_index8++;
					d_lists_producing->cutout_indices8[d_lists_producing->last_cutout_index8] = first_vertex + previous_index;
				}
			}
			d_lists_producing->last_cutout_index8++;
			d_lists_producing->cutout_indices8[d_lists_producing->last_cutout_index8] = first_vertex + current_index;
			before_previous_index = previous_index;
			previous_index = current_index;
		}
	}
	else if (first_vertex + face->numedges <= UPPER_16BIT_LIMIT)
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
		new_size = d_lists_producing->last_cutout_index16 + 1 + (face->numedges - 2) * 3;
		if (d_lists_producing->cutout_indices16.size() < new_size)
		{
			d_lists_producing->cutout_indices16.resize(new_size);
		}
		auto& vertex = entity->model->vertexes[index];
		auto x = vertex.position[0];
		auto y = vertex.position[1];
		auto z = vertex.position[2];
		auto target = d_lists_producing->cutout_vertices.data() + d_lists_producing->last_cutout_vertex + 1;
		*target++ = x;
		*target++ = y;
		*target = z;
		d_lists_producing->last_cutout_index16++;
		d_lists_producing->cutout_indices16[d_lists_producing->last_cutout_index16] = first_vertex;
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
			target = d_lists_producing->cutout_vertices.data() + d_lists_producing->last_cutout_vertex + 1 + current_index * 3;
			*target++ = x;
			*target++ = y;
			*target = z;
			if (i >= 3)
			{
				if (use_back)
				{
					d_lists_producing->last_cutout_index16++;
					d_lists_producing->cutout_indices16[d_lists_producing->last_cutout_index16] = first_vertex + previous_index;
					d_lists_producing->last_cutout_index16++;
					d_lists_producing->cutout_indices16[d_lists_producing->last_cutout_index16] = first_vertex + before_previous_index;
				}
				else
				{
					d_lists_producing->last_cutout_index16++;
					d_lists_producing->cutout_indices16[d_lists_producing->last_cutout_index16] = first_vertex + before_previous_index;
					d_lists_producing->last_cutout_index16++;
					d_lists_producing->cutout_indices16[d_lists_producing->last_cutout_index16] = first_vertex + previous_index;
				}
			}
			d_lists_producing->last_cutout_index16++;
			d_lists_producing->cutout_indices16[d_lists_producing->last_cutout_index16] = first_vertex + current_index;
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
		new_size = d_lists_producing->last_cutout_index32 + 1 + (face->numedges - 2) * 3;
		if (d_lists_producing->cutout_indices32.size() < new_size)
		{
			d_lists_producing->cutout_indices32.resize(new_size);
		}
		auto& vertex = entity->model->vertexes[index];
		auto x = vertex.position[0];
		auto y = vertex.position[1];
		auto z = vertex.position[2];
		auto target = d_lists_producing->cutout_vertices.data() + d_lists_producing->last_cutout_vertex + 1;
		*target++ = x;
		*target++ = y;
		*target = z;
		d_lists_producing->last_cutout_index32++;
		d_lists_producing->cutout_indices32[d_lists_producing->last_cutout_index32] = first_vertex;
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
			target = d_lists_producing->cutout_vertices.data() + d_lists_producing->last_cutout_vertex + 1 + current_index * 3;
			*target++ = x;
			*target++ = y;
			*target = z;
			if (i >= 3)
			{
				if (use_back)
				{
					d_lists_producing->last_cutout_index32++;
					d_lists_producing->cutout_indices32[d_lists_producing->last_cutout_index32] = first_vertex + previous_index;
					d_lists_producing->last_cutout_index32++;
					d_lists_producing->cutout_indices32[d_lists_producing->last_cutout_index32] = first_vertex + before_previous_index;
				}
				else
				{
					d_lists_producing->last_cutout_index32++;
					d_lists_producing->cutout_indices32[d_lists_producing->last_cutout_index32] = first_vertex + before_previous_index;
					d_lists_producing->last_cutout_index32++;
					d_lists_producing->cutout_indices32[d_lists_producing->last_cutout_index32] = first_vertex + previous_index;
				}
			}
			d_lists_producing->last_cutout_index32++;
			d_lists_producing->cutout_indices32[d_lists_producing->last_cutout_index32] = first_vertex + current_index;
			before_previous_index = previous_index;
			previous_index = current_index;
		}
	}
	d_lists_producing->last_cutout_vertex += 3 * face->numedges;
}

void D_AddDynamicLightsToLists ()
{
	d_lists_producing->last_dynamic_light = (int)(cl_dlights.size()) - 1;
	auto new_size = d_lists_producing->last_dynamic_light + 1;
	if (d_lists_producing->dynamic_lights.size() < new_size)
	{
		d_lists_producing->dynamic_lights.resize(new_size);
	}
	for (int i = 0; i <= d_lists_producing->last_dynamic_light; i++)
	{
		auto& l = d_lists_producing->dynamic_lights[i];
		l.origin0 = cl_dlights[i].origin[0];
		l.origin1 = cl_dlights[i].origin[1];
		l.origin2 = cl_dlights[i].origin[2];
		l.radius = cl_dlights[i].radius;
		l.minlight = cl_dlights[i].minlight;
	}
}
