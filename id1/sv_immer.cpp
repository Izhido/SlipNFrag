#include "quakedef.h"

struct immviewmodeldata_t
{
	vec3_t	angle_offset;
	vec3_t	scale_origin_offset;
};

model_t *Mod_FindName (const char *name);

std::unordered_map<std::string, immviewmodeldata_t> imm_viewmodels;

void SV_LoadImmersiveViewmodels (void)
{
	imm_viewmodels.clear();

	if (!pr_immersive_allowed) return;

	imm_viewmodels.insert({"progs/v_shot.mdl,51045",{ { 6, 0, 0 }, {-2, 0, 9}}});
	imm_viewmodels.insert({"progs/v_shot2.mdl,17450",{ { 8, 0, 0 }, {-7, 0, 8}}});
	imm_viewmodels.insert({"progs/v_nail.mdl,45157",{ { 0, 0, 0 }, {-16, 0, 16}}});
	imm_viewmodels.insert({"progs/v_nail2.mdl,45135",{ { 0, 0, 0 }, {-16, 0, 16}}});
	imm_viewmodels.insert({"progs/v_rock.mdl,31509",{ { -6, 0, 0 }, {-9 , 0, 12}}});
	imm_viewmodels.insert({"progs/v_rock2.mdl,1775",{ { 8, 0, 0 }, {-11, 0, 14}}});
	imm_viewmodels.insert({"progs/v_light.mdl,33213",{ { 3, 0, 0 }, {-7, 0, 14}}});
	imm_viewmodels.insert({"progs/v_prox.mdl,40768",{ { -6, 0, 0 }, {-9 , 0, 12}}});
	imm_viewmodels.insert({"progs/v_laserg.mdl,9376",{ { 3, 0, 0 }, {-7, 0, 14}}});
	imm_viewmodels.insert({"progs/v_lava.mdl,14853",{ { 0, 0, 0 }, {-16, 0, 16}}});
	imm_viewmodels.insert({"progs/v_lava2.mdl,53468",{ { 0, 0, 0 }, {-16, 0, 16}}});
	imm_viewmodels.insert({"progs/v_multi.mdl,55429",{ { -6, 0, 0 }, {-9 , 0, 12}}});
	imm_viewmodels.insert({"progs/v_multi2.mdl,45339",{ { 8, 0, 0 }, {-11, 0, 14}}});
	imm_viewmodels.insert({"progs/v_plasma.mdl,28062",{ { 3, 0, 0 }, {-7, 0, 14}}});
	imm_viewmodels.insert({"progs/v_shot3.mdl,24820",{ { 6, 0, 0 }, {-5, 0, 8}}});
}

qboolean SV_ValidImmersiveViewmodel (client_t* client)
{
	auto name = client->edict->v.weaponmodel;

	auto mdl = Mod_FindName (pr_strings + name);

	auto crc = mdl->alias_crc;

	std::string nameWithoutCRC = std::string(pr_strings + name);
	std::string nameWithCRC = nameWithoutCRC + "," + std::to_string(crc);

	auto entryWithCRC = imm_viewmodels.find(nameWithCRC);

	if (entryWithCRC != imm_viewmodels.end())
	{
		VectorCopy(entryWithCRC->second.angle_offset, host_client->immersive.viewmodel_angle_offset);
		VectorCopy(entryWithCRC->second.scale_origin_offset, host_client->immersive.viewmodel_scale_origin_offset);

		return true;
	}
	else
	{
		auto entryWithoutCRC = imm_viewmodels.find(nameWithoutCRC);
		if (entryWithoutCRC != imm_viewmodels.end())
		{
			VectorCopy(entryWithoutCRC->second.angle_offset, host_client->immersive.viewmodel_angle_offset);
			VectorCopy(entryWithoutCRC->second.scale_origin_offset, host_client->immersive.viewmodel_scale_origin_offset);

			return true;
		}
	}
	return false;
}
