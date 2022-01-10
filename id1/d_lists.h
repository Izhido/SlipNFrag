#include "r_shared.h"

struct dsurface_t
{
	void* surface;
	void* entity;
	void* model;
	int created;
	int texture_width;
	int texture_height;
	int texture_size;
	unsigned char* texture;
	int lightmap_width;
	int lightmap_height;
	int lightmap_size;
	int lightmap_texels;
	int count;
};

struct dsurfacerotated_t
{
	void* surface;
	void* entity;
	void* model;
	int created;
	int texture_width;
	int texture_height;
	int texture_size;
	unsigned char* texture;
	int lightmap_width;
	int lightmap_height;
	int lightmap_size;
	int lightmap_texels;
	int count;
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

struct dturbulent_t
{
	void* surface;
	void* entity;
	void* model;
	void* texture;
	int width;
	int height;
	int size;
	unsigned char* data;
	int count;
};

struct dturbulentrotated_t
{
	void* surface;
	void* entity;
	void* model;
	void* texture;
	int width;
	int height;
	int size;
	unsigned char* data;
	int count;
	float origin_x;
	float origin_y;
	float origin_z;
	float yaw;
	float pitch;
	float roll;
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
	int last_surface_rotated16;
	int last_surface_rotated32;
	int last_fence16;
	int last_fence32;
	int last_fence_rotated16;
	int last_fence_rotated32;
	int last_sprite;
	int last_turbulent16;
	int last_turbulent32;
	int last_turbulent_rotated16;
	int last_turbulent_rotated32;
	int last_alias16;
	int last_alias32;
	int last_viewmodel16;
	int last_viewmodel32;
	int last_sky;
    int last_skybox;
	int last_textured_vertex;
	int last_textured_attribute;
	int last_colormapped_attribute;
	int last_colored_vertex;
	int last_colored_attribute;
	int last_colored_index16;
	int last_colored_index32;
	int last_lightmap_texel;
	int clear_color;
	std::vector<dsurface_t> surfaces;
	std::vector<dsurfacerotated_t> surfaces_rotated16;
	std::vector<dsurfacerotated_t> surfaces_rotated32;
	std::vector<dsurface_t> fences16;
	std::vector<dsurface_t> fences32;
	std::vector<dsurfacerotated_t> fences_rotated16;
	std::vector<dsurfacerotated_t> fences_rotated32;
	std::vector<dspritedata_t> sprites;
	std::vector<dturbulent_t> turbulent16;
	std::vector<dturbulent_t> turbulent32;
	std::vector<dturbulentrotated_t> turbulent_rotated16;
	std::vector<dturbulentrotated_t> turbulent_rotated32;
	std::vector<dalias_t> alias16;
	std::vector<dalias_t> alias32;
	std::vector<dalias_t> viewmodels16;
	std::vector<dalias_t> viewmodels32;
	std::vector<dsky_t> sky;
    std::vector<dskybox_t> skyboxes;
	std::vector<float> textured_vertices;
	std::vector<float> textured_attributes;
	std::vector<float> colormapped_attributes;
	std::vector<float> colored_vertices;
	std::vector<float> colored_attributes;
	std::vector<uint16_t> colored_indices16;
	std::vector<uint32_t> colored_indices32;
	std::vector<unsigned> lightmap_texels;
};

extern dlists_t d_lists;

extern qboolean d_uselists;
extern qboolean d_awayfromviewmodel;

void D_ResetLists ();
void D_AddSurfaceToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, int created);
void D_AddSurfaceRotatedToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, int created);
void D_AddFenceToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, int created);
void D_AddFenceRotatedToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, int created);
void D_AddSpriteToLists (vec5_t* pverts, spritedesc_t* spritedesc);
void D_AddTurbulentRotatedToLists (msurface_t* face, entity_t* entity);
void D_AddTurbulentToLists (msurface_t* face, entity_t* entity);
void D_AddAliasToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* apverts);
void D_AddViewmodelToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* apverts);
void D_AddParticleToLists (particle_t* part);
void D_AddSkyToLists (qboolean full_area);
void D_AddSkyboxToLists (mtexinfo_t* textures);
void D_AddColoredSurfaceToLists (msurface_t* face, entity_t* entity, int color);
