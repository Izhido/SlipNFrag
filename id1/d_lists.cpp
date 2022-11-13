#include "quakedef.h"
#include "d_lists.h"
#include "r_shared.h"
#include "r_local.h"
#include "d_local.h"

dlists_t d_lists { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

qboolean d_uselists = false;

extern int r_ambientlight;
extern float r_shadelight;
#define NUMVERTEXNORMALS 162
extern float r_avertexnormals[NUMVERTEXNORMALS][3];
extern vec3_t r_plightvec;

void D_ResetLists ()
{
	d_lists.last_surface = -1;
	d_lists.last_surface_colored_lights = -1;
	d_lists.last_surface_rgba = -1;
	d_lists.last_surface_rgba_colored_lights = -1;
	d_lists.last_surface_rgba_no_glow = -1;
	d_lists.last_surface_rgba_no_glow_colored_lights = -1;
	d_lists.last_surface_rotated = -1;
	d_lists.last_surface_rotated_colored_lights = -1;
	d_lists.last_surface_rotated_rgba = -1;
	d_lists.last_surface_rotated_rgba_colored_lights = -1;
	d_lists.last_surface_rotated_rgba_no_glow = -1;
	d_lists.last_surface_rotated_rgba_no_glow_colored_lights = -1;
	d_lists.last_fence = -1;
	d_lists.last_fence_colored_lights = -1;
	d_lists.last_fence_rgba = -1;
	d_lists.last_fence_rgba_colored_lights = -1;
	d_lists.last_fence_rgba_no_glow = -1;
	d_lists.last_fence_rgba_no_glow_colored_lights = -1;
	d_lists.last_fence_rotated = -1;
	d_lists.last_fence_rotated_colored_lights = -1;
	d_lists.last_fence_rotated_rgba = -1;
	d_lists.last_fence_rotated_rgba_no_glow = -1;
	d_lists.last_turbulent = -1;
	d_lists.last_turbulent_rgba = -1;
	d_lists.last_turbulent_lit = -1;
	d_lists.last_turbulent_colored_lights = -1;
	d_lists.last_turbulent_rgba_lit = -1;
	d_lists.last_turbulent_rgba_colored_lights = -1;
	d_lists.last_turbulent_rotated = -1;
	d_lists.last_turbulent_rotated_rgba = -1;
	d_lists.last_turbulent_rotated_lit = -1;
	d_lists.last_turbulent_rotated_colored_lights = -1;
	d_lists.last_turbulent_rotated_rgba_lit = -1;
	d_lists.last_turbulent_rotated_rgba_colored_lights = -1;
	d_lists.last_sprite = -1;
	d_lists.last_alias = -1;
	d_lists.last_viewmodel = -1;
	d_lists.last_sky = -1;
	d_lists.last_sky_rgba = -1;
    d_lists.last_skybox = -1;
	d_lists.last_textured_vertex = -1;
	d_lists.last_textured_attribute = -1;
	d_lists.last_colormapped_attribute = -1;
	d_lists.last_particle_position = -1;
	d_lists.last_particle_color = -1;
	d_lists.last_colored_vertex = -1;
	d_lists.last_colored_color = -1;
	d_lists.last_colored_index8 = -1;
	d_lists.last_colored_index16 = -1;
	d_lists.last_colored_index32 = -1;
	d_lists.last_lightmap_texel = -1;
	d_lists.clear_color = -1;
}

void D_FillLightmap (dsurface_t& surface, surfcache_s* cache)
{
	surface.lightmap_width = cache->width / sizeof(unsigned);
	surface.lightmap_height = cache->height;
	surface.lightmap_size = surface.lightmap_width * surface.lightmap_height;
	while (d_lists.lightmap_texels.size() <= d_lists.last_lightmap_texel + surface.lightmap_size)
	{
		d_lists.lightmap_texels.resize(d_lists.lightmap_texels.size() + 64 * 1024);
	}
	surface.lightmap_texels = d_lists.last_lightmap_texel + 1;
	d_lists.last_lightmap_texel += surface.lightmap_size;
	auto source = (unsigned*)&cache->data[0];
	auto target = d_lists.lightmap_texels.data() + surface.lightmap_texels;
	for (auto i = 0; i < surface.lightmap_size; i++)
	{
		*target++ = *source++;
	}
}

void D_FillColoredLightmap (dsurface_t& surface, surfcache_s* cache)
{
	surface.lightmap_width = cache->width / (3 * sizeof(unsigned));
	surface.lightmap_height = cache->height;
	surface.lightmap_size = surface.lightmap_width * surface.lightmap_height * 4;
	while (d_lists.lightmap_texels.size() <= d_lists.last_lightmap_texel + surface.lightmap_size)
	{
		d_lists.lightmap_texels.resize(d_lists.lightmap_texels.size() + 64 * 1024);
	}
	surface.lightmap_texels = d_lists.last_lightmap_texel + 1;
	d_lists.last_lightmap_texel += surface.lightmap_size;
	auto source = (unsigned*)&cache->data[0];
	auto target = d_lists.lightmap_texels.data() + surface.lightmap_texels;
	for (auto i = 0; i < surface.lightmap_size; i += 4)
	{
		*target++ = *source++;
		*target++ = *source++;
		*target++ = *source++;
		*target++ = 65535;
	}
}

void D_FillSurfaceSize(dturbulent_t& turbulent, int component_size, int mips)
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

void D_FillSurfaceData (dsurface_t& surface, msurface_t* face, surfcache_s* cache, entity_t* entity, int mips)
{
	surface.face = face;
	surface.entity = entity;
	surface.model = entity->model;
	surface.created = cache->created;
	auto texture = (texture_t*)(cache->texture);
	surface.width = texture->width;
	surface.height = texture->height;
	D_FillSurfaceSize(surface, 1, mips);
	surface.data = (unsigned char*)texture + texture->offsets[0];
	D_FillLightmap(surface, cache);
	surface.count = face->numedges;
}

void D_FillSurfaceColoredLightsData (dsurface_t& surface, msurface_t* face, surfcache_s* cache, entity_t* entity, int mips)
{
	surface.face = face;
	surface.entity = entity;
	surface.model = entity->model;
	surface.created = cache->created;
	auto texture = (texture_t*)(cache->texture);
	surface.width = texture->width;
	surface.height = texture->height;
	D_FillSurfaceSize(surface, 1, mips);
	surface.data = (unsigned char*)texture + texture->offsets[0];
	D_FillColoredLightmap(surface, cache);
	surface.count = face->numedges;
}

void D_FillSurfaceRGBAData (dsurfacewithglow_t& surface, msurface_t* face, surfcache_s* cache, entity_t* entity, int mips)
{
	surface.face = face;
	surface.entity = entity;
	surface.model = entity->model;
	surface.created = cache->created;
	auto texture = ((texture_t*)(cache->texture))->external_color;
	surface.width = texture->width;
	surface.height = texture->height;
	D_FillSurfaceSize(surface, sizeof(unsigned), mips);
	surface.data = (unsigned char*)texture + texture->offsets[0];
	texture = ((texture_t*)(cache->texture))->external_glow;
	surface.glow_data = (unsigned char*)texture + texture->offsets[0];
	D_FillLightmap(surface, cache);
	surface.count = face->numedges;
}

void D_FillSurfaceRGBAColoredLightsData (dsurfacewithglow_t& surface, msurface_t* face, surfcache_s* cache, entity_t* entity, int mips)
{
	surface.face = face;
	surface.entity = entity;
	surface.model = entity->model;
	surface.created = cache->created;
	auto texture = ((texture_t*)(cache->texture))->external_color;
	surface.width = texture->width;
	surface.height = texture->height;
	D_FillSurfaceSize(surface, sizeof(unsigned), mips);
	surface.data = (unsigned char*)texture + texture->offsets[0];
	texture = ((texture_t*)(cache->texture))->external_glow;
	surface.glow_data = (unsigned char*)texture + texture->offsets[0];
	D_FillColoredLightmap(surface, cache);
	surface.count = face->numedges;
}

void D_FillSurfaceRGBANoGlowData (dsurface_t& surface, msurface_t* face, surfcache_s* cache, entity_t* entity, int mips)
{
	surface.face = face;
	surface.entity = entity;
	surface.model = entity->model;
	surface.created = cache->created;
	auto texture = ((texture_t*)(cache->texture))->external_color;
	surface.width = texture->width;
	surface.height = texture->height;
	D_FillSurfaceSize(surface, sizeof(unsigned), mips);
	surface.data = (unsigned char*)texture + texture->offsets[0];
	D_FillLightmap(surface, cache);
	surface.count = face->numedges;
}

void D_FillSurfaceRGBANoGlowColoredLightsData (dsurface_t& surface, msurface_t* face, surfcache_s* cache, entity_t* entity, int mips)
{
	surface.face = face;
	surface.entity = entity;
	surface.model = entity->model;
	surface.created = cache->created;
	auto texture = ((texture_t*)(cache->texture))->external_color;
	surface.width = texture->width;
	surface.height = texture->height;
	D_FillSurfaceSize(surface, sizeof(unsigned), mips);
	surface.data = (unsigned char*)texture + texture->offsets[0];
	D_FillColoredLightmap(surface, cache);
	surface.count = face->numedges;
}

void D_FillSurfaceRotatedData (dsurfacerotated_t& surface, msurface_t* face, surfcache_s* cache, entity_t* entity, int mips)
{
	D_FillSurfaceData(surface, face, cache, entity, mips);
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	surface.yaw = entity->angles[YAW];
	surface.pitch = entity->angles[PITCH];
	surface.roll = entity->angles[ROLL];
}

void D_FillSurfaceRotatedColoredLightsData (dsurfacerotated_t& surface, msurface_t* face, surfcache_s* cache, entity_t* entity, int mips)
{
	D_FillSurfaceColoredLightsData(surface, face, cache, entity, mips);
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	surface.yaw = entity->angles[YAW];
	surface.pitch = entity->angles[PITCH];
	surface.roll = entity->angles[ROLL];
}

void D_FillSurfaceRotatedRGBAData (dsurfacerotatedwithglow_t& surface, msurface_t* face, surfcache_s* cache, entity_t* entity, int mips)
{
	D_FillSurfaceRGBAData(surface, face, cache, entity, mips);
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	surface.yaw = entity->angles[YAW];
	surface.pitch = entity->angles[PITCH];
	surface.roll = entity->angles[ROLL];
}

void D_FillSurfaceRotatedRGBAColoredLightsData (dsurfacerotatedwithglow_t& surface, msurface_t* face, surfcache_s* cache, entity_t* entity, int mips)
{
	D_FillSurfaceRGBAColoredLightsData(surface, face, cache, entity, mips);
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	surface.yaw = entity->angles[YAW];
	surface.pitch = entity->angles[PITCH];
	surface.roll = entity->angles[ROLL];
}

void D_FillSurfaceRotatedRGBANoGlowData (dsurfacerotated_t& surface, msurface_t* face, surfcache_s* cache, entity_t* entity, int mips)
{
	D_FillSurfaceRGBANoGlowData(surface, face, cache, entity, mips);
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	surface.yaw = entity->angles[YAW];
	surface.pitch = entity->angles[PITCH];
	surface.roll = entity->angles[ROLL];
}

void D_FillSurfaceRotatedRGBANoGlowColoredLightsData (dsurfacerotated_t& surface, msurface_t* face, surfcache_s* cache, entity_t* entity, int mips)
{
	D_FillSurfaceRGBANoGlowColoredLightsData(surface, face, cache, entity, mips);
	surface.origin_x = entity->origin[0];
	surface.origin_y = entity->origin[1];
	surface.origin_z = entity->origin[2];
	surface.yaw = entity->angles[YAW];
	surface.pitch = entity->angles[PITCH];
	surface.roll = entity->angles[ROLL];
}

void D_AddSurfaceToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
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
	D_FillSurfaceData(surface, face, cache, entity, MIPLEVELS);
}

void D_AddSurfaceColoredLightsToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_surface_colored_lights++;
	if (d_lists.last_surface_colored_lights >= d_lists.surfaces_colored_lights.size())
	{
		d_lists.surfaces_colored_lights.emplace_back();
	}
	auto& surface = d_lists.surfaces_colored_lights[d_lists.last_surface_colored_lights];
	D_FillSurfaceColoredLightsData(surface, face, cache, entity, MIPLEVELS);
}

void D_AddSurfaceRGBAToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_surface_rgba++;
	if (d_lists.last_surface_rgba >= d_lists.surfaces_rgba.size())
	{
		d_lists.surfaces_rgba.emplace_back();
	}
	auto& surface = d_lists.surfaces_rgba[d_lists.last_surface_rgba];
	D_FillSurfaceRGBAData(surface, face, cache, entity, MIPLEVELS);
}

void D_AddSurfaceRGBAColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_surface_rgba_colored_lights++;
	if (d_lists.last_surface_rgba_colored_lights >= d_lists.surfaces_rgba_colored_lights.size())
	{
		d_lists.surfaces_rgba_colored_lights.emplace_back();
	}
	auto& surface = d_lists.surfaces_rgba_colored_lights[d_lists.last_surface_rgba_colored_lights];
	D_FillSurfaceRGBAColoredLightsData(surface, face, cache, entity, MIPLEVELS);
}

void D_AddSurfaceRGBANoGlowToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_surface_rgba_no_glow++;
	if (d_lists.last_surface_rgba_no_glow >= d_lists.surfaces_rgba_no_glow.size())
	{
		d_lists.surfaces_rgba_no_glow.emplace_back();
	}
	auto& surface = d_lists.surfaces_rgba_no_glow[d_lists.last_surface_rgba_no_glow];
	D_FillSurfaceRGBANoGlowData(surface, face, cache, entity, MIPLEVELS);
}

void D_AddSurfaceRGBANoGlowColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_surface_rgba_no_glow_colored_lights++;
	if (d_lists.last_surface_rgba_no_glow_colored_lights >= d_lists.surfaces_rgba_no_glow_colored_lights.size())
	{
		d_lists.surfaces_rgba_no_glow_colored_lights.emplace_back();
	}
	auto& surface = d_lists.surfaces_rgba_no_glow_colored_lights[d_lists.last_surface_rgba_no_glow_colored_lights];
	D_FillSurfaceRGBANoGlowColoredLightsData(surface, face, cache, entity, MIPLEVELS);
}

void D_AddSurfaceRotatedToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_surface_rotated++;
	if (d_lists.last_surface_rotated >= d_lists.surfaces_rotated.size())
	{
		d_lists.surfaces_rotated.emplace_back();
	}
	auto& surface = d_lists.surfaces_rotated[d_lists.last_surface_rotated];
	D_FillSurfaceRotatedData(surface, face, cache, entity, MIPLEVELS);
}

void D_AddSurfaceRotatedColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_surface_rotated_colored_lights++;
	if (d_lists.last_surface_rotated_colored_lights >= d_lists.surfaces_rotated_colored_lights.size())
	{
		d_lists.surfaces_rotated_colored_lights.emplace_back();
	}
	auto& surface = d_lists.surfaces_rotated_colored_lights[d_lists.last_surface_rotated_colored_lights];
	D_FillSurfaceRotatedColoredLightsData(surface, face, cache, entity, MIPLEVELS);
}

void D_AddSurfaceRotatedRGBAToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_surface_rotated_rgba++;
	if (d_lists.last_surface_rotated_rgba >= d_lists.surfaces_rotated_rgba.size())
	{
		d_lists.surfaces_rotated_rgba.emplace_back();
	}
	auto& surface = d_lists.surfaces_rotated_rgba[d_lists.last_surface_rotated_rgba];
	D_FillSurfaceRotatedRGBAData(surface, face, cache, entity, MIPLEVELS);
}

void D_AddSurfaceRotatedRGBAColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_surface_rotated_rgba_colored_lights++;
	if (d_lists.last_surface_rotated_rgba_colored_lights >= d_lists.surfaces_rotated_rgba_colored_lights.size())
	{
		d_lists.surfaces_rotated_rgba_colored_lights.emplace_back();
	}
	auto& surface = d_lists.surfaces_rotated_rgba_colored_lights[d_lists.last_surface_rotated_rgba_colored_lights];
	D_FillSurfaceRotatedRGBAColoredLightsData(surface, face, cache, entity, MIPLEVELS);
}

void D_AddSurfaceRotatedRGBANoGlowToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_surface_rotated_rgba_no_glow++;
	if (d_lists.last_surface_rotated_rgba_no_glow >= d_lists.surfaces_rotated_rgba_no_glow.size())
	{
		d_lists.surfaces_rotated_rgba_no_glow.emplace_back();
	}
	auto& surface = d_lists.surfaces_rotated_rgba_no_glow[d_lists.last_surface_rotated_rgba_no_glow];
	D_FillSurfaceRotatedRGBANoGlowData(surface, face, cache, entity, MIPLEVELS);
}

void D_AddSurfaceRotatedRGBANoGlowColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_surface_rotated_rgba_no_glow_colored_lights++;
	if (d_lists.last_surface_rotated_rgba_no_glow_colored_lights >= d_lists.surfaces_rotated_rgba_no_glow_colored_lights.size())
	{
		d_lists.surfaces_rotated_rgba_no_glow_colored_lights.emplace_back();
	}
	auto& surface = d_lists.surfaces_rotated_rgba_no_glow_colored_lights[d_lists.last_surface_rotated_rgba_no_glow_colored_lights];
	D_FillSurfaceRotatedRGBANoGlowColoredLightsData(surface, face, cache, entity, MIPLEVELS);
}

void D_AddFenceToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_fence++;
	if (d_lists.last_fence >= d_lists.fences.size())
	{
		d_lists.fences.emplace_back();
	}
	auto& fence = d_lists.fences[d_lists.last_fence];
	D_FillSurfaceData(fence, face, cache, entity, MIPLEVELS);
}

void D_AddFenceColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_fence_colored_lights++;
	if (d_lists.last_fence_colored_lights >= d_lists.fences_colored_lights.size())
	{
		d_lists.fences_colored_lights.emplace_back();
	}
	auto& fence = d_lists.fences_colored_lights[d_lists.last_fence_colored_lights];
	D_FillSurfaceColoredLightsData(fence, face, cache, entity, MIPLEVELS);
}

void D_AddFenceRGBAToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_fence_rgba++;
	if (d_lists.last_fence_rgba >= d_lists.fences_rgba.size())
	{
		d_lists.fences_rgba.emplace_back();
	}
	auto& fence = d_lists.fences_rgba[d_lists.last_fence_rgba];
	D_FillSurfaceRGBAData(fence, face, cache, entity, MIPLEVELS);
}

void D_AddFenceRGBAColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_fence_rgba_colored_lights++;
	if (d_lists.last_fence_rgba_colored_lights >= d_lists.fences_rgba_colored_lights.size())
	{
		d_lists.fences_rgba_colored_lights.emplace_back();
	}
	auto& fence = d_lists.fences_rgba_colored_lights[d_lists.last_fence_rgba_colored_lights];
	D_FillSurfaceRGBAColoredLightsData(fence, face, cache, entity, MIPLEVELS);
}

void D_AddFenceRGBANoGlowToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_fence_rgba_no_glow++;
	if (d_lists.last_fence_rgba_no_glow >= d_lists.fences_rgba_no_glow.size())
	{
		d_lists.fences_rgba_no_glow.emplace_back();
	}
	auto& fence = d_lists.fences_rgba_no_glow[d_lists.last_fence_rgba_no_glow];
	D_FillSurfaceRGBANoGlowData(fence, face, cache, entity, MIPLEVELS);
}

void D_AddFenceRGBANoGlowColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_fence_rgba_no_glow_colored_lights++;
	if (d_lists.last_fence_rgba_no_glow_colored_lights >= d_lists.fences_rgba_no_glow_colored_lights.size())
	{
		d_lists.fences_rgba_no_glow_colored_lights.emplace_back();
	}
	auto& fence = d_lists.fences_rgba_no_glow_colored_lights[d_lists.last_fence_rgba_no_glow_colored_lights];
	D_FillSurfaceRGBANoGlowColoredLightsData(fence, face, cache, entity, MIPLEVELS);
}

void D_AddFenceRotatedToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_fence_rotated++;
	if (d_lists.last_fence_rotated >= d_lists.fences_rotated.size())
	{
		d_lists.fences_rotated.emplace_back();
	}
	auto& fence = d_lists.fences_rotated[d_lists.last_fence_rotated];
	D_FillSurfaceRotatedData(fence, face, cache, entity, MIPLEVELS);
}

void D_AddFenceRotatedColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_fence_rotated_colored_lights++;
	if (d_lists.last_fence_rotated_colored_lights >= d_lists.fences_rotated_colored_lights.size())
	{
		d_lists.fences_rotated_colored_lights.emplace_back();
	}
	auto& fence = d_lists.fences_rotated_colored_lights[d_lists.last_fence_rotated_colored_lights];
	D_FillSurfaceRotatedColoredLightsData(fence, face, cache, entity, MIPLEVELS);
}

void D_AddFenceRotatedRGBAToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_fence_rotated_rgba++;
	if (d_lists.last_fence_rotated_rgba >= d_lists.fences_rotated_rgba.size())
	{
		d_lists.fences_rotated_rgba.emplace_back();
	}
	auto& fence = d_lists.fences_rotated_rgba[d_lists.last_fence_rotated_rgba];
	D_FillSurfaceRotatedRGBAData(fence, face, cache, entity, MIPLEVELS);
}

void D_AddFenceRotatedRGBAColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_fence_rotated_rgba_colored_lights++;
	if (d_lists.last_fence_rotated_rgba_colored_lights >= d_lists.fences_rotated_rgba_colored_lights.size())
	{
		d_lists.fences_rotated_rgba_colored_lights.emplace_back();
	}
	auto& fence = d_lists.fences_rotated_rgba_colored_lights[d_lists.last_fence_rotated_rgba_colored_lights];
	D_FillSurfaceRotatedRGBAColoredLightsData(fence, face, cache, entity, MIPLEVELS);
}

void D_AddFenceRotatedRGBANoGlowToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_fence_rotated_rgba_no_glow++;
	if (d_lists.last_fence_rotated_rgba_no_glow >= d_lists.fences_rotated_rgba_no_glow.size())
	{
		d_lists.fences_rotated_rgba_no_glow.emplace_back();
	}
	auto& fence = d_lists.fences_rotated_rgba_no_glow[d_lists.last_fence_rotated_rgba_no_glow];
	D_FillSurfaceRotatedRGBANoGlowData(fence, face, cache, entity, MIPLEVELS);
}

void D_AddFenceRotatedRGBANoGlowColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_fence_rotated_rgba_no_glow_colored_lights++;
	if (d_lists.last_fence_rotated_rgba_no_glow_colored_lights >= d_lists.fences_rotated_rgba_no_glow_colored_lights.size())
	{
		d_lists.fences_rotated_rgba_no_glow_colored_lights.emplace_back();
	}
	auto& fence = d_lists.fences_rotated_rgba_no_glow_colored_lights[d_lists.last_fence_rotated_rgba_no_glow_colored_lights];
	D_FillSurfaceRotatedRGBANoGlowColoredLightsData(fence, face, cache, entity, MIPLEVELS);
}

void D_FillTurbulentData (dturbulent_t& turbulent, msurface_t* face, entity_t* entity, int mips)
{
	turbulent.face = face;
	turbulent.entity = entity;
	turbulent.model = entity->model;
	auto texture = face->texinfo->texture;
	turbulent.width = texture->width;
	turbulent.height = texture->height;
	D_FillSurfaceSize(turbulent, 1, mips);
	turbulent.data = (unsigned char*)texture + texture->offsets[0];
	turbulent.count = face->numedges;
}

void D_FillTurbulentRGBAData (dturbulent_t& turbulent, msurface_t* face, entity_t* entity, int mips)
{
	turbulent.face = face;
	turbulent.entity = entity;
	turbulent.model = entity->model;
	auto texture = face->texinfo->texture->external_color;
	turbulent.width = texture->width;
	turbulent.height = texture->height;
	D_FillSurfaceSize(turbulent, sizeof(unsigned), mips);
	turbulent.data = (unsigned char*)texture + texture->offsets[0];
	turbulent.count = face->numedges;
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
	auto& turbulent = d_lists.turbulent[d_lists.last_turbulent];
	D_FillTurbulentData(turbulent, face, entity, MIPLEVELS);
}

void D_AddTurbulentRGBAToLists (msurface_t* face, entity_t* entity)
{
	if (face->numedges < 3)
	{
		return;
	}
	d_lists.last_turbulent_rgba++;
	if (d_lists.last_turbulent_rgba >= d_lists.turbulent_rgba.size())
	{
		d_lists.turbulent_rgba.emplace_back();
	}
	auto& turbulent = d_lists.turbulent_rgba[d_lists.last_turbulent_rgba];
	D_FillTurbulentRGBAData(turbulent, face, entity, MIPLEVELS);
}

void D_AddTurbulentLitToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_turbulent_lit++;
	if (d_lists.last_turbulent_lit >= d_lists.turbulent_lit.size())
	{
		d_lists.turbulent_lit.emplace_back();
	}
	auto& turbulent = d_lists.turbulent_lit[d_lists.last_turbulent_lit];
	D_FillTurbulentData(turbulent, face, entity, MIPLEVELS);
	turbulent.created = cache->created;
	D_FillLightmap(turbulent, cache);
}

void D_AddTurbulentColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_turbulent_colored_lights++;
	if (d_lists.last_turbulent_colored_lights >= d_lists.turbulent_colored_lights.size())
	{
		d_lists.turbulent_colored_lights.emplace_back();
	}
	auto& turbulent = d_lists.turbulent_colored_lights[d_lists.last_turbulent_colored_lights];
	D_FillTurbulentData(turbulent, face, entity, MIPLEVELS);
	turbulent.created = cache->created;
	D_FillColoredLightmap(turbulent, cache);
}

void D_AddTurbulentRGBALitToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_turbulent_rgba_lit++;
	if (d_lists.last_turbulent_rgba_lit >= d_lists.turbulent_rgba_lit.size())
	{
		d_lists.turbulent_rgba_lit.emplace_back();
	}
	auto& turbulent = d_lists.turbulent_rgba_lit[d_lists.last_turbulent_rgba_lit];
	D_FillTurbulentRGBAData(turbulent, face, entity, MIPLEVELS);
	turbulent.created = cache->created;
	D_FillLightmap(turbulent, cache);
}

void D_AddTurbulentRGBAColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_turbulent_rgba_colored_lights++;
	if (d_lists.last_turbulent_rgba_colored_lights >= d_lists.turbulent_rgba_colored_lights.size())
	{
		d_lists.turbulent_rgba_colored_lights.emplace_back();
	}
	auto& turbulent = d_lists.turbulent_rgba_colored_lights[d_lists.last_turbulent_rgba_colored_lights];
	D_FillTurbulentRGBAData(turbulent, face, entity, MIPLEVELS);
	turbulent.created = cache->created;
	D_FillColoredLightmap(turbulent, cache);
}

void D_AddTurbulentRotatedToLists (msurface_t* face, entity_t* entity)
{
	if (face->numedges < 3)
	{
		return;
	}
	d_lists.last_turbulent_rotated++;
	if (d_lists.last_turbulent_rotated >= d_lists.turbulent_rotated.size())
	{
		d_lists.turbulent_rotated.emplace_back();
	}
	auto& turbulent = d_lists.turbulent_rotated[d_lists.last_turbulent_rotated];
	D_FillTurbulentData(turbulent, face, entity, MIPLEVELS);
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	turbulent.yaw = entity->angles[YAW];
	turbulent.pitch = entity->angles[PITCH];
	turbulent.roll = entity->angles[ROLL];
}

void D_AddTurbulentRotatedRGBAToLists (msurface_t* face, entity_t* entity)
{
	if (face->numedges < 3)
	{
		return;
	}
	d_lists.last_turbulent_rotated_rgba++;
	if (d_lists.last_turbulent_rotated_rgba >= d_lists.turbulent_rotated_rgba.size())
	{
		d_lists.turbulent_rotated_rgba.emplace_back();
	}
	auto& turbulent = d_lists.turbulent_rotated_rgba[d_lists.last_turbulent_rotated_rgba];
	D_FillTurbulentRGBAData(turbulent, face, entity, MIPLEVELS);
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	turbulent.yaw = entity->angles[YAW];
	turbulent.pitch = entity->angles[PITCH];
	turbulent.roll = entity->angles[ROLL];
}

void D_AddTurbulentRotatedLitToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_turbulent_rotated_lit++;
	if (d_lists.last_turbulent_rotated_lit >= d_lists.turbulent_rotated_lit.size())
	{
		d_lists.turbulent_rotated_lit.emplace_back();
	}
	auto& turbulent = d_lists.turbulent_rotated_lit[d_lists.last_turbulent_rotated_lit];
	D_FillTurbulentData(turbulent, face, entity, MIPLEVELS);
	turbulent.created = cache->created;
	D_FillLightmap(turbulent, cache);
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	turbulent.yaw = entity->angles[YAW];
	turbulent.pitch = entity->angles[PITCH];
	turbulent.roll = entity->angles[ROLL];
}

void D_AddTurbulentRotatedColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_turbulent_rotated_colored_lights++;
	if (d_lists.last_turbulent_rotated_colored_lights >= d_lists.turbulent_rotated_colored_lights.size())
	{
		d_lists.turbulent_rotated_colored_lights.emplace_back();
	}
	auto& turbulent = d_lists.turbulent_rotated_colored_lights[d_lists.last_turbulent_rotated_colored_lights];
	D_FillTurbulentData(turbulent, face, entity, MIPLEVELS);
	turbulent.created = cache->created;
	D_FillColoredLightmap(turbulent, cache);
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	turbulent.yaw = entity->angles[YAW];
	turbulent.pitch = entity->angles[PITCH];
	turbulent.roll = entity->angles[ROLL];
}

void D_AddTurbulentRotatedRGBALitToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_turbulent_rotated_rgba_lit++;
	if (d_lists.last_turbulent_rotated_rgba_lit >= d_lists.turbulent_rotated_rgba_lit.size())
	{
		d_lists.turbulent_rotated_rgba_lit.emplace_back();
	}
	auto& turbulent = d_lists.turbulent_rotated_rgba_lit[d_lists.last_turbulent_rotated_rgba_lit];
	D_FillTurbulentRGBAData(turbulent, face, entity, MIPLEVELS);
	turbulent.created = cache->created;
	D_FillLightmap(turbulent, cache);
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	turbulent.yaw = entity->angles[YAW];
	turbulent.pitch = entity->angles[PITCH];
	turbulent.roll = entity->angles[ROLL];
}

void D_AddTurbulentRotatedRGBAColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity)
{
	if (face->numedges < 3 || cache->width <= 0 || cache->height <= 0)
	{
		return;
	}
	d_lists.last_turbulent_rotated_rgba_colored_lights++;
	if (d_lists.last_turbulent_rotated_rgba_colored_lights >= d_lists.turbulent_rotated_rgba_colored_lights.size())
	{
		d_lists.turbulent_rotated_rgba_colored_lights.emplace_back();
	}
	auto& turbulent = d_lists.turbulent_rotated_rgba_colored_lights[d_lists.last_turbulent_rotated_rgba_colored_lights];
	D_FillTurbulentRGBAData(turbulent, face, entity, MIPLEVELS);
	turbulent.created = cache->created;
	D_FillColoredLightmap(turbulent, cache);
	turbulent.origin_x = entity->origin[0];
	turbulent.origin_y = entity->origin[1];
	turbulent.origin_z = entity->origin[2];
	turbulent.yaw = entity->angles[YAW];
	turbulent.pitch = entity->angles[PITCH];
	turbulent.roll = entity->angles[ROLL];
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
	d_lists.textured_vertices[d_lists.last_textured_vertex] = y;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = z;
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
	d_lists.textured_vertices[d_lists.last_textured_vertex] = y;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = z;
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
	d_lists.textured_vertices[d_lists.last_textured_vertex] = y;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = z;
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
	d_lists.textured_vertices[d_lists.last_textured_vertex] = y;
	d_lists.last_textured_vertex++;
	d_lists.textured_vertices[d_lists.last_textured_vertex] = z;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = s;
	d_lists.last_textured_attribute++;
	d_lists.textured_attributes[d_lists.last_textured_attribute] = t;
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
		alias.colormap = nullptr;
	}
	else
	{
		alias.colormap = colormap;
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
	alias.count = mdl->numtris * 3;
}

void D_FillAliasLights (aliashdr_t* aliashdr, trivertx_t* apverts, mdl_t* mdl)
{
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
}

void D_AddAliasToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* apverts)
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
	D_FillAliasData(alias, aliashdr, mdl, skindesc, colormap, apverts);
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
	D_FillAliasLights(aliashdr, apverts, mdl);
}

void D_AddViewmodelToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* apverts)
{
	auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
	if (mdl->numtris <= 0)
	{
		return;
	}
	d_lists.last_viewmodel++;
	if (d_lists.last_viewmodel >= d_lists.viewmodels.size())
	{
		d_lists.viewmodels.emplace_back();
	}
	auto& viewmodel = d_lists.viewmodels[d_lists.last_viewmodel];
	D_FillAliasData(viewmodel, aliashdr, mdl, skindesc, colormap, apverts);
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
	t2matrix[0][3] = currententity->origin[0] + r_modelorg_delta[0];
	t2matrix[1][3] = currententity->origin[1] + r_modelorg_delta[1];
	t2matrix[2][3] = currententity->origin[2] + r_modelorg_delta[2];
	R_ConcatTransforms (t2matrix, tmatrix, viewmodel.transform);
	D_FillAliasLights(aliashdr, apverts, mdl);
}

void D_AddParticleToLists (particle_t* part)
{
	auto new_size = d_lists.last_particle_position + 1 + 3;
	if (d_lists.particle_positions.size() < new_size)
	{
		d_lists.particle_positions.resize(new_size);
	}
	auto x = part->org[0];
	auto y = part->org[1];
	auto z = part->org[2];
	d_lists.last_particle_position++;
	d_lists.particle_positions[d_lists.last_particle_position] = x;
	d_lists.last_particle_position++;
	d_lists.particle_positions[d_lists.last_particle_position] = y;
	d_lists.last_particle_position++;
	d_lists.particle_positions[d_lists.last_particle_position] = z;
	new_size = d_lists.last_particle_color + 1 + 1;
	if (d_lists.particle_colors.size() < new_size)
	{
		d_lists.particle_colors.resize(new_size);
	}
	d_lists.last_particle_color++;
	d_lists.particle_colors[d_lists.last_particle_color] = part->color;
}

void D_FillSkyData (dsky_t& sky, qboolean full_area)
{
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
	sky.width = 128;
	sky.height = 128;
	sky.size = sky.width * sky.height;
	sky.data = r_skysource;
	D_FillSkyData(sky, full_area);
}

void D_AddSkyRGBAToLists (qboolean full_area)
{
	if (d_lists.last_sky_rgba >= 0)
	{
		return;
	}
	d_lists.last_sky_rgba++;
	if (d_lists.last_sky_rgba >= d_lists.sky_rgba.size())
	{
		d_lists.sky_rgba.emplace_back();
	}
	auto& sky = d_lists.sky_rgba[d_lists.last_sky_rgba];
	sky.width = r_skyRGBAwidth;
	sky.height = r_skyRGBAheight;
	sky.size = sky.width * 2 * sky.height * sizeof(unsigned);
	sky.data = (byte*)r_skysourceRGBA;
	D_FillSkyData(sky, full_area);
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
	new_size = d_lists.last_colored_color + 1 + face->numedges;
	if (d_lists.colored_colors.size() < new_size)
	{
		d_lists.colored_colors.resize(new_size);
	}
	auto first_vertex = (d_lists.last_colored_vertex + 1) / 3;
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
		new_size = d_lists.last_colored_index8 + 1 + (face->numedges - 2) * 3;
		if (d_lists.colored_indices8.size() < new_size)
		{
			d_lists.colored_indices8.resize(new_size);
		}
		auto& vertex = entity->model->vertexes[index];
		auto x = vertex.position[0];
		auto y = vertex.position[1];
		auto z = vertex.position[2];
		auto target = d_lists.colored_vertices.data() + d_lists.last_colored_vertex + 1;
		*target++ = x;
		*target++ = y;
		*target = z;
		target = d_lists.colored_colors.data() + d_lists.last_colored_color + 1;
		*target = color;
		d_lists.last_colored_index8++;
		d_lists.colored_indices8[d_lists.last_colored_index8] = first_vertex;
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
			target = d_lists.colored_vertices.data() + d_lists.last_colored_vertex + 1 + current_index * 3;
			*target++ = x;
			*target++ = y;
			*target = z;
			target = d_lists.colored_colors.data() + d_lists.last_colored_color + 1 + current_index;
			*target = color;
			if (i >= 3)
			{
				if (use_back)
				{
					d_lists.last_colored_index8++;
					d_lists.colored_indices8[d_lists.last_colored_index8] = first_vertex + previous_index;
					d_lists.last_colored_index8++;
					d_lists.colored_indices8[d_lists.last_colored_index8] = first_vertex + before_previous_index;
				}
				else
				{
					d_lists.last_colored_index8++;
					d_lists.colored_indices8[d_lists.last_colored_index8] = first_vertex + before_previous_index;
					d_lists.last_colored_index8++;
					d_lists.colored_indices8[d_lists.last_colored_index8] = first_vertex + previous_index;
				}
			}
			d_lists.last_colored_index8++;
			d_lists.colored_indices8[d_lists.last_colored_index8] = first_vertex + current_index;
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
		*target++ = y;
		*target = z;
		target = d_lists.colored_colors.data() + d_lists.last_colored_color + 1;
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
			*target++ = y;
			*target = z;
			target = d_lists.colored_colors.data() + d_lists.last_colored_color + 1 + current_index;
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
		*target++ = y;
		*target = z;
		target = d_lists.colored_colors.data() + d_lists.last_colored_color + 1;
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
			*target++ = y;
			*target = z;
			target = d_lists.colored_colors.data() + d_lists.last_colored_color + 1 + current_index;
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
	d_lists.last_colored_color += face->numedges;
}
