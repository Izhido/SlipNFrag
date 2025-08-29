#include "quakedef.h"

struct immviewmodeldata_t
{
	vec3_t	angle_offset;
	vec3_t	scale_origin_offset;
};

std::unordered_map<std::string, immviewmodeldata_t> imm_viewmodels;

void SV_LoadImmersiveViewmodels (void)
{
	imm_viewmodels.clear();

	if (!pr_immersive_allowed) return;

	imm_viewmodels.insert({"progs/v_shot.mdl",{ { 6, 0, 0 }, {-2, 0, 9}}});
	imm_viewmodels.insert({"progs/v_shot2.mdl",{ { 8, 0, 0 }, {-7, 0, 8}}});
	imm_viewmodels.insert({"progs/v_nail.mdl",{ { 0, 0, 0 }, {-16, 0, 16}}});
	imm_viewmodels.insert({"progs/v_nail2.mdl",{ { 0, 0, 0 }, {-16, 0, 16}}});
	imm_viewmodels.insert({"progs/v_rock.mdl",{ { -6, 0, 0 }, {-9 , 0, 12}}});
	imm_viewmodels.insert({"progs/v_rock2.mdl",{ { 8, 0, 0 }, {-11, 0, 14}}});
	imm_viewmodels.insert({"progs/v_light.mdl",{ { 3, 0, 0 }, {-7, 0, 14}}});
	imm_viewmodels.insert({"progs/v_prox.mdl",{ { -6, 0, 0 }, {-9 , 0, 12}}});
	imm_viewmodels.insert({"progs/v_laserg.mdl",{ { 3, 0, 0 }, {-7, 0, 14}}});
	imm_viewmodels.insert({"progs/v_lava.mdl",{ { 0, 0, 0 }, {-16, 0, 16}}});
	imm_viewmodels.insert({"progs/v_lava2.mdl",{ { 0, 0, 0 }, {-16, 0, 16}}});
	imm_viewmodels.insert({"progs/v_multi.mdl",{ { -6, 0, 0 }, {-9 , 0, 12}}});
	imm_viewmodels.insert({"progs/v_multi2.mdl",{ { 8, 0, 0 }, {-11, 0, 14}}});
	imm_viewmodels.insert({"progs/v_plasma.mdl",{ { 3, 0, 0 }, {-7, 0, 14}}});
	imm_viewmodels.insert({"progs/v_shot3.mdl",{ { 6, 0, 0 }, {-5, 0, 8}}});
}

qboolean SV_ValidImmersiveViewmodel (client_t* client)
{
	auto name = client->edict->v.weaponmodel;

	auto entry = imm_viewmodels.find(pr_strings + name);

	if (entry == imm_viewmodels.end())
		return false;

	VectorCopy(entry->second.angle_offset, host_client->immersive.viewmodel_angle_offset);
	VectorCopy(entry->second.scale_origin_offset, host_client->immersive.viewmodel_scale_origin_offset);

	return true;
}
