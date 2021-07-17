#include "r_shared.h"

struct dsurface_t
{
	void* surface;
	void* entity;
	void* vertexes;
	int vertex_count;
	int created;
	int texture_width;
	int texture_height;
	int texture_size;
	unsigned char* texture;
	int lightmap_width;
	int lightmap_height;
	int lightmap_size;
	float* lightmap;
	int first_index;
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
	void* vertexes;
	int vertex_count;
	void* texture;
	int width;
	int height;
	int size;
	unsigned char* data;
	int first_index;
	int count;
	float origin_x;
	float origin_y;
	float origin_z;
	float yaw;
	float pitch;
	float roll;
	float vecs[2][4];
};

struct dalias_t
{
	int width;
	int height;
	int size;
	unsigned char* data;
	std::vector<unsigned char> colormap;
	qboolean is_host_colormap;
	trivertx_t* vertices;
	stvert_t* texture_coordinates;
	int vertex_count;
	int first_attribute;
	int first_index;
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
	int last_surface16;
	int last_surface32;
	int last_fence16;
	int last_fence32;
	int last_sprite;
	int last_turbulent16;
	int last_turbulent32;
	int last_alias16;
	int last_alias32;
	int last_viewmodel16;
	int last_viewmodel32;
	int last_sky;
    int last_skybox;
	int last_surface_index16;
	int last_surface_index32;
	int last_textured_vertex;
	int last_textured_attribute;
	int last_colormapped_attribute;
	int last_colormapped_index16;
	int last_colormapped_index32;
	int last_colored_vertex;
	int last_colored_attribute;
	int last_colored_index16;
	int last_colored_index32;
	int last_lightmap_texel;
	int clear_color;
	std::vector<dsurface_t> surfaces16;
	std::vector<dsurface_t> surfaces32;
	std::vector<dsurface_t> fences16;
	std::vector<dsurface_t> fences32;
	std::vector<dspritedata_t> sprites;
	std::vector<dturbulent_t> turbulent16;
	std::vector<dturbulent_t> turbulent32;
	std::vector<dalias_t> alias16;
	std::vector<dalias_t> alias32;
	std::vector<dalias_t> viewmodels16;
	std::vector<dalias_t> viewmodels32;
	std::vector<dsky_t> sky;
    std::vector<dskybox_t> skyboxes;
	std::vector<uint16_t> surface_indices16;
	std::vector<uint32_t> surface_indices32;
	std::vector<float> textured_vertices;
	std::vector<float> textured_attributes;
	std::vector<float> colormapped_attributes;
	std::vector<uint16_t> colormapped_indices16;
	std::vector<uint32_t> colormapped_indices32;
	std::vector<float> colored_vertices;
	std::vector<float> colored_attributes;
	std::vector<uint16_t> colored_indices16;
	std::vector<uint32_t> colored_indices32;
	std::vector<float> lightmap_texels;
};

extern dlists_t d_lists;

extern qboolean d_uselists;
extern qboolean d_awayfromviewmodel;

void D_ResetLists ();
void D_AddSurfaceToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, qboolean created);
void D_AddFenceToLists (msurface_t* face, struct surfcache_s* cache, entity_t* entity, qboolean created);
void D_AddSpriteToLists (vec5_t* pverts, spritedesc_t* spritedesc);
void D_AddTurbulentToLists (msurface_t* face, entity_t* entity);
void D_AddAliasToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* vertices);
void D_AddViewModelToLists (aliashdr_t* aliashdr, maliasskindesc_t* skindesc, byte* colormap, trivertx_t* vertices);
void D_AddParticleToLists (particle_t* part);
void D_AddSkyToLists (surf_t* surf);
void D_AddSkyboxToLists (mtexinfo_t* textures);
void D_AddColoredSurfaceToLists (msurface_t* face, entity_t* entity, int color);
