#include "r_shared.h"

#define UPPER_8BIT_LIMIT 240
#define UPPER_16BIT_LIMIT 65520

struct dturbulent_t
{
	void* face;
	void* entity;
	void* model;
	int width;
	int height;
	int size;
	int mips;
	unsigned char* data;
	int count;
};

struct dsurface_t : dturbulent_t
{
	int created;
	int lightmap_width;
	int lightmap_height;
	int lightmap_size;
	int lightmap_texels;
};

struct dsurfacewithglow_t : dturbulent_t
{
	unsigned char* glow_data;
	int created;
	int lightmap_width;
	int lightmap_height;
	int lightmap_size;
	int lightmap_texels;
};

struct dsurfacerotated_t : dsurface_t
{
	float origin_x;
	float origin_y;
	float origin_z;
	float yaw;
	float pitch;
	float roll;
};

struct dsurfacerotatedwithglow_t : dsurfacewithglow_t
{
	float origin_x;
	float origin_y;
	float origin_z;
	float yaw;
	float pitch;
	float roll;
};

struct dspritedata_t
{
	int width;
	int height;
	int size;
	unsigned char* data;
	int first_vertex;
	int count;
};

struct dalias_t
{
	void* aliashdr;
	int width;
	int height;
	int size;
	unsigned char* data;
	std::vector<unsigned char> colormap;
	qboolean is_host_colormap;
	trivertx_t* apverts;
	stvert_t* texture_coordinates;
	int vertex_count;
	int first_attribute;
	int count;
	float transform[3][4];
};

struct dsky_t
{
	int width;
	int height;
	int size;
	unsigned char* data;
	float top;
	float left;
	float right;
	float bottom;
	int first_vertex;
	int count;
};

struct dskybox_t
{
    mtexinfo_t* textures;
};

struct dlists_t
{
	int last_surface;
	int last_surface_rgba;
	int last_surface_rgba_no_glow;
	int last_surface_rotated;
	int last_surface_rotated_rgba;
	int last_surface_rotated_rgba_no_glow;
	int last_fence;
	int last_fence_rgba;
	int last_fence_rgba_no_glow;
	int last_fence_rotated;
	int last_fence_rotated_rgba;
	int last_fence_rotated_rgba_no_glow;
	int last_turbulent;
	int last_turbulent_rgba;
	int last_turbulent_lit;
	int last_turbulent_lit_rgba;
	int last_sprite;
	int last_alias;
	int last_viewmodel;
	int last_sky;
	int last_sky_rgba;
    int last_skybox;
	int last_textured_vertex;
	int last_textured_attribute;
	int last_colormapped_attribute;
	int last_particle_position;
	int last_particle_color;
	int last_colored_vertex;
	int last_colored_color;
	int last_colored_index8;
	int last_colored_index16;
	int last_colored_index32;
	int last_lightmap_texel;
	int clear_color;
	float vieworg0;
	float vieworg1;
	float vieworg2;
	float vpn0;
	float vpn1;
	float vpn2;
	float vright0;
	float vright1;
	float vright2;
	float vup0;
	float vup1;
	float vup2;
	std::vector<dsurface_t> surfaces;
	std::vector<dsurfacewithglow_t> surfaces_rgba;
	std::vector<dsurface_t> surfaces_rgba_no_glow;
	std::vector<dsurfacerotated_t> surfaces_rotated;
	std::vector<dsurfacerotatedwithglow_t> surfaces_rotated_rgba;
	std::vector<dsurfacerotated_t> surfaces_rotated_rgba_no_glow;
	std::vector<dsurface_t> fences;
	std::vector<dsurfacewithglow_t> fences_rgba;
	std::vector<dsurface_t> fences_rgba_no_glow;
	std::vector<dsurfacerotated_t> fences_rotated;
	std::vector<dsurfacerotatedwithglow_t> fences_rotated_rgba;
	std::vector<dsurfacerotated_t> fences_rotated_rgba_no_glow;
	std::vector<dturbulent_t> turbulent;
	std::vector<dturbulent_t> turbulent_rgba;
	std::vector<dsurface_t> turbulent_lit;
	std::vector<dsurface_t> turbulent_lit_rgba;
	std::vector<dspritedata_t> sprites;
	std::vector<dalias_t> alias;
	std::vector<dalias_t> viewmodels;
	std::vector<dsky_t> sky;
	std::vector<dsky_t> sky_rgba;
    std::vector<dskybox_t> skyboxes;
	std::vector<float> textured_vertices;
	std::vector<float> textured_attributes;
	std::vector<float> colormapped_attributes;
	std::vector<float> particle_positions;
	std::vector<float> particle_colors;
	std::vector<float> colored_vertices;
	std::vector<float> colored_colors;
	std::vector<unsigned char> colored_indices8;
	std::vector<uint16_t> colored_indices16;
	std::vector<uint32_t> colored_indices32;
	std::vector<uint16_t> lightmap_texels;
};

extern dlists_t d_lists;

extern qboolean d_uselists;

void D_ResetLists ();
void D_AddSurfaceToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddSurfaceRGBAToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddSurfaceRGBANoGlowToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddSurfaceRotatedToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddSurfaceRotatedRGBAToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddSurfaceRotatedRGBANoGlowToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddFenceToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddFenceRGBAToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddFenceRGBANoGlowToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddFenceRotatedToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddFenceRotatedRGBAToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddFenceRotatedRGBANoGlowToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddTurbulentToLists (msurface_t* face, entity_t* entity);
void D_AddTurbulentRGBAToLists (msurface_t* face, entity_t* entity);
void D_AddTurbulentLitToLists (msurface_t* face, surfcache_s* cache, entity_t* entity);
void D_AddTurbulentLitRGBAToLists (msurface_t* face, surfcache_s* cache, entity_t* entity);
void D_AddSpriteToLists (vec5_t* pverts, spritedesc_t* spritedesc);
void D_AddAliasToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* apverts);
void D_AddViewmodelToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* apverts);
void D_AddParticleToLists (particle_t* part);
void D_AddSkyToLists (qboolean full_area);
void D_AddSkyRGBAToLists (qboolean full_area);
void D_AddSkyboxToLists (mtexinfo_t* textures);
void D_AddColoredSurfaceToLists (msurface_t* face, entity_t* entity, int color);
