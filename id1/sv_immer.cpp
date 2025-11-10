#include "quakedef.h"

struct immviewmodeldata_t
{
	vec3_t	rotate;
	vec3_t	offset;
	vec3_t	scale;
};

model_t *Mod_FindName (const char *name);

std::unordered_map<std::string, immviewmodeldata_t> imm_viewmodels;

void TrimText(std::string& text)
{
	while (!text.empty())
	{
		if (text.back() <= ' ')
		{
			text.pop_back();
		}
		else
			break;
	}
	while (!text.empty())
	{
		if (text.front() <= ' ')
		{
			text.erase(0, 1);
		}
		else
			break;
	}
}

void SV_LoadViewmodelsFromFile (std::vector<byte>& contents)
{
	std::vector<std::string> lines;

	std::string line;
	for (auto c : contents)
	{
		if (c == '\n' || c == '\r')
		{
			if (!line.empty())
			{
				auto p = line.rfind('#');
				if (p != std::string::npos)
				{
					line.resize(p);
				}
				TrimText (line);
				if (!line.empty())
				{
					lines.push_back(line);
					line.clear();
				}
			}
		}
		else
		{
			line += c;
		}
	}
	if (!line.empty())
	{
		auto p = line.rfind('#');
		if (p != std::string::npos)
		{
			line.resize(p);
		}
		TrimText (line);
		if (!line.empty())
		{
			lines.push_back(line);
		}
	}


	for (auto& entry : lines)
	{
		auto p = entry.find(':');

		std::string name;
		std::string data;

		if (p == std::string::npos)
			name = entry;
		else
		{
			name = entry.substr(0, p);
			if (p < entry.length() - 1)
				data = entry.substr(p + 1);
		}

		TrimText (name);
		if (name.empty()) continue;

		std::string crc;

		p = name.find(',');

		if (p != std::string::npos)
		{
			crc = name.substr(p + 1);
			name = name.substr(0, p);
			TrimText (crc);
			TrimText (name);
		}

		auto crc_valid = true;
		if (!crc.empty())
		{
			int val = 0;
			for (auto c : crc)
			{
				if (c >= '0' && c <= '9')
				{
					val = val * 10 + (c - '0');
				}
				else
				{
					crc_valid = false;
					break;
				}
			}

			if (crc_valid && val > 65535)
				crc_valid = false;
		}

		if (!crc_valid) continue;

		immviewmodeldata_t viewmodeldata { { }, { }, { 1, 1, 1 }};

		if (!data.empty())
		{
			std::vector<std::string> numbers;

			std::string number;
			for (auto c : data)
			{
				if (c == ',')
				{
					if (!number.empty())
					{
						TrimText (number);
						if (!number.empty())
						{
							numbers.push_back(number);
							number.clear();
						}
						else
						{
							numbers.emplace_back("0");
						}
					}
				}
				else
				{
					number += c;
				}
			}
			if (!number.empty())
			{
				TrimText (number);
				if (!number.empty())
				{
					numbers.push_back(number);
					number.clear();
				}
				else
				{
					numbers.emplace_back("0");
				}
			}

			int position = 0;
			for (auto& num_entry : numbers)
			{
				auto value = Q_atof(num_entry.c_str());
				switch (position)
				{
				case 0:
					viewmodeldata.rotate[0] = value;
					break;
				case 1:
					viewmodeldata.rotate[1] = value;
					break;
				case 2:
					viewmodeldata.rotate[2] = value;
					break;
				case 3:
					viewmodeldata.offset[0] = value;
					break;
				case 4:
					viewmodeldata.offset[1] = value;
					break;
				case 5:
					viewmodeldata.offset[2] = value;
					break;
				case 6:
					viewmodeldata.scale[0] = value;
					break;
				case 7:
					viewmodeldata.scale[1] = value;
					break;
				default:
					viewmodeldata.scale[2] = value;
				}

				position++;
				if (position >= 9)
					break;
			}
		}

		if (crc.empty())
			imm_viewmodels[name] = viewmodeldata;
		else
			imm_viewmodels[name + "," + crc] = viewmodeldata;
	}
}

void SV_LoadImmersiveViewmodels (void)
{
	imm_viewmodels.clear();

	if (!pr_immersive_allowed) return;

// model[,CRC16]: [rotation in degrees], [offset from origin], [scale]

// id1
	imm_viewmodels.insert({"progs/v_shot.mdl,51045",{{ 6, 0, 0 }, { -2, 0, 9 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_shot2.mdl,17450",{{ 8, 0, 0 }, { -7, 0, 8 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_nail.mdl,45157",{{ 0, 0, 0 }, { -16, 0, 16 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_nail2.mdl,45135",{{ 0, 0, 0 }, { -16, 0, 16 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_rock.mdl,31509",{{ -6, 0, 0 }, { -9 , 0, 12 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_rock2.mdl,1775",{{ 8, 0, 0 }, { -11, 0, 14 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_light.mdl,33213",{{ 3, 0, 0 }, { -7, 0, 14 }, { 1, 1, 1 }}});

// hipnotic
	imm_viewmodels.insert({"progs/v_prox.mdl,40768",{{ -6, 0, 0 }, { -9 , 0, 12 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_laserg.mdl,9376",{{ 3, 0, 0 }, { -7, 0, 14 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_lava.mdl,14853",{{ 0, 0, 0 }, { -16, 0, 16 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_lava2.mdl,53468",{{ 0, 0, 0 }, { -16, 0, 16 }, { 1, 1, 1 }}});

// rogue
	imm_viewmodels.insert({"progs/v_multi.mdl,55429",{{ -6, 0, 0 }, { -9 , 0, 12 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_multi2.mdl,45339",{{ 8, 0, 0 }, { -11, 0, 14 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_plasma.mdl,28062",{{ 3, 0, 0 }, { -7, 0, 14 }, { 1, 1, 1 }}});

// ArcaneDimensions (1.80p1)
	imm_viewmodels.insert({"progs/v_shot.mdl,64893",{{ 5, 0, 0 }, { -6, 0, 13 }, { 0.7, 0.7, 0.7 }}});
	imm_viewmodels.insert({"progs/v_shot2.mdl,53147",{{ 8, 0, 0 }, { -7, 0, 8 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_shot3.mdl,24820",{{ 5, 0, 0 }, { -6, 0, 8 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_nail.mdl,27843",{{ 0, 0, 0 }, { -16, 0, 16 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_nail2.mdl,38486",{{ 0, 0, 0 }, { -16, 0, 16 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_rock.mdl,55259",{{ -4, 0, 0 }, { -9 , 0, 12 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_rock2.mdl,30105",{{ 8, 0, 0 }, { -11, 0, 14 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_light.mdl,47549",{{ 1, 0, 0 }, { -7, 0, 14 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_plasma.mdl,10007",{{ -13, 0, 0 }, { -16, 0, 20 }, { 2, 1, 1 }}});

// alkaline (1.2)
	imm_viewmodels.insert({"progs/v_shot40fps.mdl,22201",{{ 5, 0, 0 }, { -6, 0, 13 }, { 0.7, 0.7, 0.7 }}});
	imm_viewmodels.insert({"progs/v_shot2_40fps.mdl,40908",{{ 8, 0, 0 }, { -13, 0, 12 }, { 0.7, 0.7, 0.7 }}});
	imm_viewmodels.insert({"progs/v_nail_alk40fps.mdl,57447",{{ 0, 0, 0 }, { -13, 0, 16 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_nail3.mdl,28732",{{ 0, 0, 0 }, { -12, 0, 16 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_rock_40fps.mdl,32950",{{ -6, 0, 0 }, { -9 , 0, 12 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_mine_40fps.mdl,45911",{{ -6, 0, 0 }, { -9 , 0, 12 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_rock2_40fps.mdl,27002",{{ 8, 0, 0 }, { -11, 0, 14 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_laserg40fps.mdl,18484",{{ 5, 0, 0 }, { -7, 0, 14 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_plasma.mdl,25190",{{ 1, 0, 0 }, { 9, 0, 14 }, { 1, 1, 1 }}});

// LibreQuake
	imm_viewmodels.insert({"progs/v_shot.mdl,17894",{{ 0, 0, 0 }, { -3, 0, 10 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_shot2.mdl,9073",{{ 0, 0, 0 }, { -1, 0, 10 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_nail.mdl,53996",{{ 0, 0, 0 }, { -14, 0, 18 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_nail2.mdl,2792",{{ 0, 0, 0 }, { 1, 0, 21 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_rock.mdl,4650",{{ 0, 0, 0 }, { -6, 0, 16 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_rock2.mdl,8174",{{ -2, 0, 0 }, { -6, 0, 24 }, { 1, 1, 1 }}});
	imm_viewmodels.insert({"progs/v_light.mdl,25844",{{ 0, 0, 0 }, { 12, 0, 14 }, { 1, 1, 1 }}});

	const char* viewmodels_file = "viewmodels.txt";

	std::vector<byte> contents;
	auto buffer = COM_LoadFile (viewmodels_file, false, contents);
	if (buffer)
	{
		if (contents.size() > 1)
			SV_LoadViewmodelsFromFile (contents);
		contents.clear();
	}

	int f;
	auto len = Sys_FileOpenRead ((com_basedir + "/" + viewmodels_file).c_str(), &f);
	if (f > 0)
	{
		if (len > 0)
		{
			contents.resize (len+1);
			contents[len] = 0;
			Sys_FileRead (f, contents.data(), len);
		}
		Sys_FileClose (f);
		if (contents.size() > 1)
			SV_LoadViewmodelsFromFile (contents);
	}
}

qboolean SV_ValidImmersiveViewmodel (client_t* client)
{
	auto name = client->edict->v.weaponmodel;
	if (name == 0) return false;

	auto mod_name = pr_strings + name;
	if (mod_name[0] == 0) return false;

	auto mdl = Mod_FindName (pr_strings + name);

	auto crc = mdl->alias_crc;

	std::string nameWithoutCRC = std::string(pr_strings + name);
	std::string nameWithCRC = nameWithoutCRC + "," + std::to_string(crc);

	auto entryWithCRC = imm_viewmodels.find(nameWithCRC);

	if (entryWithCRC != imm_viewmodels.end())
	{
		VectorCopy(entryWithCRC->second.rotate, host_client->immersive.viewmodel_rotate);
		VectorCopy(entryWithCRC->second.offset, host_client->immersive.viewmodel_offset);
		VectorCopy(entryWithCRC->second.scale, host_client->immersive.viewmodel_scale);

		return true;
	}
	else
	{
		auto entryWithoutCRC = imm_viewmodels.find(nameWithoutCRC);
		if (entryWithoutCRC != imm_viewmodels.end())
		{
			VectorCopy(entryWithoutCRC->second.rotate, host_client->immersive.viewmodel_rotate);
			VectorCopy(entryWithoutCRC->second.offset, host_client->immersive.viewmodel_offset);
			VectorCopy(entryWithoutCRC->second.scale, host_client->immersive.viewmodel_scale);

			return true;
		}
	}
	return false;
}
