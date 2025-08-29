/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_tent.c -- client side temporary entities

#include "quakedef.h"

int			num_temp_entities;
std::vector<entity_t>	cl_temp_entities(MAX_TEMP_ENTITIES);
bool	cl_increase_temp_entities;

int	additional_beams;
std::vector<beam_t>		cl_beams(MAX_BEAMS);

sfx_t			*cl_sfx_wizhit;
sfx_t			*cl_sfx_knighthit;
sfx_t			*cl_sfx_tink1;
sfx_t			*cl_sfx_ric1;
sfx_t			*cl_sfx_ric2;
sfx_t			*cl_sfx_ric3;
sfx_t			*cl_sfx_r_exp3;
#ifdef QUAKE2
sfx_t			*cl_sfx_imp;
sfx_t			*cl_sfx_rail;
#endif

/*
=================
CL_InitTEnts
=================
*/
void CL_InitTEnts (void)
{
	cl_sfx_wizhit = S_PrecacheSound ("wizard/hit.wav");
	cl_sfx_knighthit = S_PrecacheSound ("hknight/hit.wav");
	cl_sfx_tink1 = S_PrecacheSound ("weapons/tink1.wav");
	cl_sfx_ric1 = S_PrecacheSound ("weapons/ric1.wav");
	cl_sfx_ric2 = S_PrecacheSound ("weapons/ric2.wav");
	cl_sfx_ric3 = S_PrecacheSound ("weapons/ric3.wav");
	cl_sfx_r_exp3 = S_PrecacheSound ("weapons/r_exp3.wav");
#ifdef QUAKE2
	cl_sfx_imp = S_PrecacheSound ("shambler/sattck1.wav");
	cl_sfx_rail = S_PrecacheSound ("weapons/lstart.wav");
#endif
	cl_increase_temp_entities = false;
}

/*
=================
CL_ParseBeam
=================
*/
void CL_ParseBeam (model_t *m)
{
	int		ent;
	vec3_t	start, end;
	beam_t	*b;
	int		i;

	if (cl_protocol_version_from_server == EXPANDED_PROTOCOL_VERSION)
	{
		ent = MSG_ReadLong ();
	
		start[0] = MSG_ReadFloat ();
		start[1] = MSG_ReadFloat ();
		start[2] = MSG_ReadFloat ();

		end[0] = MSG_ReadFloat ();
		end[1] = MSG_ReadFloat ();
		end[2] = MSG_ReadFloat ();
	}
	else
	{
		ent = MSG_ReadShort ();
	
		start[0] = MSG_ReadCoord ();
		start[1] = MSG_ReadCoord ();
		start[2] = MSG_ReadCoord ();

		end[0] = MSG_ReadCoord ();
		end[1] = MSG_ReadCoord ();
		end[2] = MSG_ReadCoord ();
	}

// override any beam with the same entity
	for (i=0, b=cl_beams.data() ; i< cl_beams.size() ; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = m;
			b->endtime = cl.time + 0.2;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			return;
		}

// find a free beam
	for (i=0, b=cl_beams.data() ; i< cl_beams.size() ; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = m;
			b->endtime = cl.time + 0.2;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			return;
		}
	}
	Con_Printf ("beam list overflow!\n");
	additional_beams++;
}

/*
=================
CL_ReadTEntPosition
=================
*/
void CL_ReadTEntPosition (vec3_t pos)
{
	if (cl_protocol_version_from_server == EXPANDED_PROTOCOL_VERSION)
	{
		for (auto i=0 ; i<3 ; i++)
			pos[i] = MSG_ReadFloat ();
	}
	else
	{
		for (auto i=0 ; i<3 ; i++)
			pos[i] = MSG_ReadCoord ();
	}
}

/*
=================
CL_ParseTEnt
=================
*/
void CL_ParseTEnt (void)
{
	int		type;
	vec3_t	pos;
#ifdef QUAKE2
	vec3_t	endpos;
#endif
	dlight_t	*dl;
	int		rnd;
	int		colorStart, colorLength;

	type = MSG_ReadByte ();
	switch (type)
	{
	case TE_WIZSPIKE:			// spike hitting wall
		CL_ReadTEntPosition (pos);
		R_RunParticleEffect (pos, vec3_origin, 20, 30);
		S_StartSound (-1, 0, cl_sfx_wizhit, pos, 1, 1);
		break;
		
	case TE_KNIGHTSPIKE:			// spike hitting wall
		CL_ReadTEntPosition (pos);
		R_RunParticleEffect (pos, vec3_origin, 226, 20);
		S_StartSound (-1, 0, cl_sfx_knighthit, pos, 1, 1);
		break;
		
	case TE_SPIKE:			// spike hitting wall
		CL_ReadTEntPosition (pos);
#ifdef GLTEST
		Test_Spawn (pos);
#else
		R_RunParticleEffect (pos, vec3_origin, 0, 10);
#endif
		if ( Sys_Random() % 5 )
			S_StartSound (-1, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = Sys_Random() & 3;
			if (rnd == 1)
				S_StartSound (-1, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-1, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-1, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;
	case TE_SUPERSPIKE:			// super spike hitting wall
		CL_ReadTEntPosition (pos);
		R_RunParticleEffect (pos, vec3_origin, 0, 20);

		if ( Sys_Random() % 5 )
			S_StartSound (-1, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = Sys_Random() & 3;
			if (rnd == 1)
				S_StartSound (-1, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-1, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-1, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;
		
	case TE_GUNSHOT:			// bullet hitting wall
		CL_ReadTEntPosition (pos);
		R_RunParticleEffect (pos, vec3_origin, 0, 20);
		break;
		
	case TE_EXPLOSION:			// rocket explosion
		CL_ReadTEntPosition (pos);
		R_ParticleExplosion (pos);
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 350;
		dl->die = cl.time + 0.5;
		dl->decay = 300;
		S_StartSound (-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;
		
	case TE_TAREXPLOSION:			// tarbaby explosion
		CL_ReadTEntPosition (pos);
		R_BlobExplosion (pos);

		S_StartSound (-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;

	case TE_LIGHTNING1:				// lightning bolts
		CL_ParseBeam (Mod_ForName("progs/bolt.mdl", true));
		break;
	
	case TE_LIGHTNING2:				// lightning bolts
		CL_ParseBeam (Mod_ForName("progs/bolt2.mdl", true));
		break;
	
	case TE_LIGHTNING3:				// lightning bolts
		CL_ParseBeam (Mod_ForName("progs/bolt3.mdl", true));
		break;
	
// PGM 01/21/97 
	case TE_BEAM:				// grappling hook beam
		CL_ParseBeam (Mod_ForName("progs/beam.mdl", true));
		break;
// PGM 01/21/97

	case TE_LAVASPLASH:	
		CL_ReadTEntPosition (pos);
		R_LavaSplash (pos);
		break;
	
	case TE_TELEPORT:
		CL_ReadTEntPosition (pos);
		R_TeleportSplash (pos);
		break;
		
	case TE_EXPLOSION2:				// color mapped explosion
		CL_ReadTEntPosition (pos);
		colorStart = MSG_ReadByte ();
		colorLength = MSG_ReadByte ();
		R_ParticleExplosion2 (pos, colorStart, colorLength);
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 350;
		dl->die = cl.time + 0.5;
		dl->decay = 300;
		S_StartSound (-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;
		
#ifdef QUAKE2
	case TE_IMPLOSION:
		CL_ReadTEntPosition (pos);
		S_StartSound (-1, 0, cl_sfx_imp, pos, 1, 1);
		break;

	case TE_RAILTRAIL:
		CL_ReadTEntPosition (pos);
		CL_ReadTEntPosition (endpos);
		S_StartSound (-1, 0, cl_sfx_rail, pos, 1, 1);
		S_StartSound (-1, 1, cl_sfx_r_exp3, endpos, 1, 1);
		R_RocketTrail (pos, endpos, 0+128);
		R_ParticleExplosion (endpos);
		dl = CL_AllocDlight (-1);
		VectorCopy (endpos, dl->origin);
		dl->radius = 350;
		dl->die = cl.time + 0.5;
		dl->decay = 300;
		break;
#endif

	default:
		Sys_Error ("CL_ParseTEnt: bad type");
	}
}


/*
=================
CL_NewTempEntity
=================
*/
entity_t *CL_NewTempEntity (void)
{
	entity_t	*ent;

	if (num_temp_entities == cl_temp_entities.size())
	{
		cl_increase_temp_entities = true;
		return NULL;
	}
	ent = &cl_temp_entities[num_temp_entities];
	memset (ent, 0, sizeof(*ent));
	num_temp_entities++;
	if (cl_numvisedicts >= (int)cl_visedicts.size())
	{
		cl_visedicts.resize(cl_visedicts.size() + MAX_VISEDICTS);
	}
	cl_visedicts[cl_numvisedicts] = ent;
	cl_numvisedicts++;

	ent->colormap = vid.colormap;
	return ent;
}


/*
=================
CL_UpdateTEnts
=================
*/
void CL_UpdateTEnts (void)
{
	int			i;
	beam_t		*b;
	vec3_t		dist, org;
	float		d;
	entity_t	*ent;
	float		yaw, pitch;
	float		forward;

	if (cl_increase_temp_entities)
	{
		cl_temp_entities.resize(cl_temp_entities.size() + MAX_TEMP_ENTITIES);
		cl_increase_temp_entities = false;
	}
	num_temp_entities = 0;

	if (additional_beams > 0)
	{
		additional_beams = MAX_BEAMS * (int)ceil((float)additional_beams / MAX_BEAMS);
		cl_beams.resize(cl_beams.size() + additional_beams);
		additional_beams = 0;
	}

// update lightning
	for (i=0, b=cl_beams.data() ; i< cl_beams.size() ; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
			continue;

	// if coming from the player, update the start position
		if (b->entity == cl.viewentity)
		{
			VectorCopy (cl_entities[cl.viewentity].origin, b->start);
			if (cl.immersive_enabled)
			{
				if (cl.immersive_hands_enabled)
				{
					auto hand = Cvar_VariableString ("dominant_hand");

					if (Q_strncmp(hand, "right", 5) == 0)
					{
						VectorAdd (b->start, cl_immersive_right_hand_delta, b->start);
					}
					else
					{
						VectorAdd (b->start, cl_immersive_left_hand_delta, b->start);
					}
				}
				else
				{
					VectorAdd (b->start, cl_immersive_origin_delta, b->start);
				}
			}
		}

	// calculate pitch and yaw
		VectorSubtract (b->end, b->start, dist);

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			yaw = (int) (atan2(dist[1], dist[0]) * 180 / M_PI);
			if (yaw < 0)
				yaw += 360;
	
			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = (int) (atan2(dist[2], forward) * 180 / M_PI);
			if (pitch < 0)
				pitch += 360;
		}

	// add new entities for the lightning
		VectorCopy (b->start, org);
		d = VectorNormalize(dist);
		while (d > 0)
		{
			ent = CL_NewTempEntity ();
			if (!ent)
				return;
			VectorCopy (org, ent->origin);
			ent->model = b->model;
			ent->angles[0] = pitch;
			ent->angles[1] = yaw;
			ent->angles[2] = Sys_Random()%360;

			for (i=0 ; i<3 ; i++)
				org[i] += dist[i]*30;
			d -= 30;
		}
	}
	
}


