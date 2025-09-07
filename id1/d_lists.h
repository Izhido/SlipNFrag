#include "r_shared.h"

#define UPPER_8BIT_LIMIT 240
#define UPPER_16BIT_LIMIT 65520

struct dturbulent_t
{
	void* face;
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
	int first_lightmap_texel;
};

struct dsurfacewithglow_t : dsurface_t
{
	unsigned char* glow_data;
};

struct dturbulentrotated_t : dturbulent_t
{
	float origin_x;
	float origin_y;
	float origin_z;
	float yaw;
	float pitch;
	float roll;
	unsigned char alpha;
};

struct dsurfacerotated_t : dsurface_t
{
	float origin_x;
	float origin_y;
	float origin_z;
	float yaw;
	float pitch;
	float roll;
	unsigned char alpha;
};

struct dsurfacerotatedwithglow_t : dsurfacewithglow_t
{
	float origin_x;
	float origin_y;
	float origin_z;
	float yaw;
	float pitch;
	float roll;
	unsigned char alpha;
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

struct daliascoloredlights_t
{
	void* aliashdr;
	int width;
	int height;
	int size;
	unsigned char* data;
	trivertx_t* apverts;
	stvert_t* texture_coordinates;
	int vertex_count;
	int first_attribute;
	int count;
	float transform[3][4];
};

struct dalias_t : daliascoloredlights_t
{
	unsigned char* colormap;
};

struct dviewmodelcoloredlights_t : daliascoloredlights_t
{
	float transform2[3][4];
};

struct dviewmodel_t : dviewmodelcoloredlights_t
{
	unsigned char* colormap;
};

struct dsky_t
{
	int width;
	int height;
	int size;
	unsigned char* data;
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
	int last_surface_colored_lights;
	int last_surface_rgba;
	int last_surface_rgba_colored_lights;
	int last_surface_rgba_no_glow;
	int last_surface_rgba_no_glow_colored_lights;
	int last_surface_rotated;
	int last_surface_rotated_colored_lights;
	int last_surface_rotated_rgba;
	int last_surface_rotated_rgba_colored_lights;
	int last_surface_rotated_rgba_no_glow;
	int last_surface_rotated_rgba_no_glow_colored_lights;
	int last_fence;
	int last_fence_colored_lights;
	int last_fence_rgba;
	int last_fence_rgba_colored_lights;
	int last_fence_rgba_no_glow;
	int last_fence_rgba_no_glow_colored_lights;
	int last_fence_rotated;
	int last_fence_rotated_colored_lights;
	int last_fence_rotated_rgba;
	int last_fence_rotated_rgba_colored_lights;
	int last_fence_rotated_rgba_no_glow;
	int last_fence_rotated_rgba_no_glow_colored_lights;
	int last_turbulent;
	int last_turbulent_rgba;
	int last_turbulent_lit;
	int last_turbulent_colored_lights;
	int last_turbulent_rgba_lit;
	int last_turbulent_rgba_colored_lights;
	int last_turbulent_rotated;
	int last_turbulent_rotated_rgba;
	int last_turbulent_rotated_lit;
	int last_turbulent_rotated_colored_lights;
	int last_turbulent_rotated_rgba_lit;
	int last_turbulent_rotated_rgba_colored_lights;
	int last_sprite;
	int last_alias;
	int last_alias_alpha;
	int last_alias_colored_lights;
	int last_alias_alpha_colored_lights;
	int last_alias_holey;
	int last_alias_holey_alpha;
	int last_alias_holey_colored_lights;
	int last_alias_holey_alpha_colored_lights;
	int last_viewmodel;
	int last_viewmodel_colored_lights;
	int last_viewmodel_holey;
	int last_viewmodel_holey_colored_lights;
	int last_sky;
	int last_sky_rgba;
    int last_skybox;
	int last_textured_vertex;
	int last_textured_attribute;
	int last_alias_attribute;
	int last_particle;
	int last_colored_vertex;
	int last_colored_color;
	int last_colored_index8;
	int last_colored_index16;
	int last_colored_index32;
	int last_cutout_vertex;
	int last_cutout_index8;
	int last_cutout_index16;
	int last_cutout_index32;
	int last_lightmap_texel;
	int clear_color;
	float vieworg0;
	float vieworg1;
	float vieworg2;
	float vright0;
	float vright1;
	float vright2;
	float vup0;
	float vup1;
	float vup2;
	double time;
	qboolean immersive_hands_enabled;
	qboolean dominant_hand_left;
	float viewmodel_angle_offset0;
	float viewmodel_angle_offset1;
	float viewmodel_angle_offset2;
	float viewmodel_scale_origin_offset0;
	float viewmodel_scale_origin_offset1;
	float viewmodel_scale_origin_offset2;
	std::vector<dsurface_t> surfaces;
	std::vector<dsurface_t> surfaces_colored_lights;
	std::vector<dsurfacewithglow_t> surfaces_rgba;
	std::vector<dsurfacewithglow_t> surfaces_rgba_colored_lights;
	std::vector<dsurface_t> surfaces_rgba_no_glow;
	std::vector<dsurface_t> surfaces_rgba_no_glow_colored_lights;
	std::vector<dsurfacerotated_t> surfaces_rotated;
	std::vector<dsurfacerotated_t> surfaces_rotated_colored_lights;
	std::vector<dsurfacerotatedwithglow_t> surfaces_rotated_rgba;
	std::vector<dsurfacerotatedwithglow_t> surfaces_rotated_rgba_colored_lights;
	std::vector<dsurfacerotated_t> surfaces_rotated_rgba_no_glow;
	std::vector<dsurfacerotated_t> surfaces_rotated_rgba_no_glow_colored_lights;
	std::vector<dsurface_t> fences;
	std::vector<dsurface_t> fences_colored_lights;
	std::vector<dsurfacewithglow_t> fences_rgba;
	std::vector<dsurfacewithglow_t> fences_rgba_colored_lights;
	std::vector<dsurface_t> fences_rgba_no_glow;
	std::vector<dsurface_t> fences_rgba_no_glow_colored_lights;
	std::vector<dsurfacerotated_t> fences_rotated;
	std::vector<dsurfacerotated_t> fences_rotated_colored_lights;
	std::vector<dsurfacerotatedwithglow_t> fences_rotated_rgba;
	std::vector<dsurfacerotatedwithglow_t> fences_rotated_rgba_colored_lights;
	std::vector<dsurfacerotated_t> fences_rotated_rgba_no_glow;
	std::vector<dsurfacerotated_t> fences_rotated_rgba_no_glow_colored_lights;
	std::vector<dturbulent_t> turbulent;
	std::vector<dturbulent_t> turbulent_rgba;
	std::vector<dsurface_t> turbulent_lit;
	std::vector<dsurface_t> turbulent_colored_lights;
	std::vector<dsurface_t> turbulent_rgba_lit;
	std::vector<dsurface_t> turbulent_rgba_colored_lights;
	std::vector<dturbulentrotated_t> turbulent_rotated;
	std::vector<dturbulentrotated_t> turbulent_rotated_rgba;
	std::vector<dsurfacerotated_t> turbulent_rotated_lit;
	std::vector<dsurfacerotated_t> turbulent_rotated_colored_lights;
	std::vector<dsurfacerotated_t> turbulent_rotated_rgba_lit;
	std::vector<dsurfacerotated_t> turbulent_rotated_rgba_colored_lights;
	std::vector<dspritedata_t> sprites;
	std::vector<dalias_t> alias;
	std::vector<dalias_t> alias_alpha;
	std::vector<daliascoloredlights_t> alias_colored_lights;
	std::vector<daliascoloredlights_t> alias_alpha_colored_lights;
	std::vector<dalias_t> alias_holey;
	std::vector<dalias_t> alias_holey_alpha;
	std::vector<daliascoloredlights_t> alias_holey_colored_lights;
	std::vector<daliascoloredlights_t> alias_holey_alpha_colored_lights;
	std::vector<dviewmodel_t> viewmodels;
	std::vector<dviewmodelcoloredlights_t> viewmodels_colored_lights;
	std::vector<dviewmodel_t> viewmodels_holey;
	std::vector<dviewmodelcoloredlights_t> viewmodels_holey_colored_lights;
	std::vector<dsky_t> sky;
	std::vector<dsky_t> sky_rgba;
    std::vector<dskybox_t> skyboxes;
	std::vector<float> textured_vertices;
	std::vector<float> textured_attributes;
	std::vector<float> alias_attributes;
	std::vector<float> particles;
	std::vector<float> colored_vertices;
	std::vector<float> colored_colors;
	std::vector<unsigned char> colored_indices8;
	std::vector<uint16_t> colored_indices16;
	std::vector<uint32_t> colored_indices32;
	std::vector<float> cutout_vertices;
	std::vector<unsigned char> cutout_indices8;
	std::vector<uint16_t> cutout_indices16;
	std::vector<uint32_t> cutout_indices32;
	std::vector<uint32_t> lightmap_texels;
};

extern dlists_t d_lists;

extern qboolean d_uselists;

void D_ResetLists ();
void D_AddSurfaceToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddSurfaceColoredLightsToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddSurfaceRGBAToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddSurfaceRGBAColoredLightsToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddSurfaceRGBANoGlowToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddSurfaceRGBANoGlowColoredLightsToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddSurfaceRotatedToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddSurfaceRotatedColoredLightsToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddSurfaceRotatedRGBAToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddSurfaceRotatedRGBAColoredLightsToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddSurfaceRotatedRGBANoGlowToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddSurfaceRotatedRGBANoGlowColoredLightsToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddFenceToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddFenceColoredLightsToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddFenceRGBAToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddFenceRGBAColoredLightsToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddFenceRGBANoGlowToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddFenceRGBANoGlowColoredLightsToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity);
void D_AddFenceRotatedToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddFenceRotatedColoredLightsToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddFenceRotatedRGBAToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddFenceRotatedRGBAColoredLightsToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddFenceRotatedRGBANoGlowToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddFenceRotatedRGBANoGlowColoredLightsToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddTurbulentToLists (msurface_t* face, entity_t* entity);
void D_AddTurbulentRGBAToLists (msurface_t* face, entity_t* entity);
void D_AddTurbulentLitToLists (msurface_t* face, surfcache_s* cache, entity_t* entity);
void D_AddTurbulentColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity);
void D_AddTurbulentRGBALitToLists (msurface_t* face, surfcache_s* cache, entity_t* entity);
void D_AddTurbulentRGBAColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity);
void D_AddTurbulentRotatedToLists (msurface_t* face, entity_t* entity, byte alpha);
void D_AddTurbulentRotatedRGBAToLists (msurface_t* face, entity_t* entity, byte alpha);
void D_AddTurbulentRotatedLitToLists (msurface_t* face, surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddTurbulentRotatedColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddTurbulentRotatedRGBALitToLists (msurface_t* face, surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddTurbulentRotatedRGBAColoredLightsToLists (msurface_t* face, surfcache_s* cache, entity_t* entity, byte alpha);
void D_AddSpriteToLists (vec5_t* pverts, spritedesc_t* spritedesc);
void D_AddAliasToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity);
void D_AddAliasAlphaToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity);
void D_AddAliasColoredLightsToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity);
void D_AddAliasAlphaColoredLightsToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity);
void D_AddAliasHoleyToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity);
void D_AddAliasHoleyAlphaToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity);
void D_AddAliasHoleyColoredLightsToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity);
void D_AddAliasHoleyAlphaColoredLightsToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity);
void D_AddViewmodelToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity);
void D_AddViewmodelColoredLightsToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity);
void D_AddViewmodelHoleyToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity);
void D_AddViewmodelHoleyColoredLightsToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, trivertx_t* apverts, entity_t* entity);
void D_AddParticleToLists (particle_t* part);
void D_AddSkyToLists ();
void D_AddSkyRGBAToLists ();
void D_AddSkyboxToLists (mtexinfo_t* textures);
void D_AddColoredSurfaceToLists (msurface_t* face, entity_t* entity, int color);
void D_AddCutoutSurfaceToLists (msurface_t* face, entity_t* entity);
