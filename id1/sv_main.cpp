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
// sv_main.c -- server main program

#include "quakedef.h"

server_t		sv;
server_static_t	svs;

int				sv_protocol_version;
int				sv_static_entity_count;

qboolean		sv_bump_protocol_version;
qboolean		sv_request_protocol_version_upgrade;

void server_t::Clear()
{
	active = false;
	paused = false;
	loadgame = false;
	time = 0;
	lastcheck = 0;
	lastchecktime = 0;
	name = 0;
	memset(modelname, 0, sizeof(modelname));
	worldmodel = nullptr;
	model_precache.clear();
	model_index.clear();
	models.clear();
	sound_precache.clear();
	lightstyles.clear();
	lightstyles.resize(MAX_LIGHTSTYLES);
	num_edicts = 0;
	SV_DeleteEdictLeafs(0, edicts.size());
	memset(edicts.data(), 0, edicts.size());
	state = ss_loading;
	datagram.Clear();
	datagram_expanded.Clear();
	reliable_datagram.Clear();
	reliable_datagram_expanded.Clear();
	signon.Clear();
}

void client_t::Clear()
{
	active = false;
	spawned = false;
	dropasap = false;
	sendsignon = false;
	last_message = 0;
	netconnection = nullptr;
	memset(&cmd, 0, sizeof(cmd));
	memset(wishdir, 0, sizeof(wishdir));
	message.Clear();
	edict = nullptr;
	name = 0;
	colors = 0;
	memset(ping_times, 0, sizeof(ping_times));
	num_pings = 0;
	memset(spawn_parms, 0, sizeof(spawn_parms));
	old_frags = 0;
	protocol_version = 0;
	serverinfo_protocol_offset = 0;
}

//============================================================================

/*
===============
SV_Init
===============
*/
void SV_Init (void)
{
	extern	cvar_t	sv_maxvelocity;
	extern	cvar_t	sv_gravity;
	extern	cvar_t	sv_nostep;
	extern	cvar_t	sv_friction;
	extern	cvar_t	sv_edgefriction;
	extern	cvar_t	sv_stopspeed;
	extern	cvar_t	sv_maxspeed;
	extern	cvar_t	sv_accelerate;
	extern	cvar_t	sv_idealpitchscale;
	extern	cvar_t	sv_aim;

	Cvar_RegisterVariable (&sv_maxvelocity);
	Cvar_RegisterVariable (&sv_gravity);
	Cvar_RegisterVariable (&sv_friction);
	Cvar_RegisterVariable (&sv_edgefriction);
	Cvar_RegisterVariable (&sv_stopspeed);
	Cvar_RegisterVariable (&sv_maxspeed);
	Cvar_RegisterVariable (&sv_accelerate);
	Cvar_RegisterVariable (&sv_idealpitchscale);
	Cvar_RegisterVariable (&sv_aim);
	Cvar_RegisterVariable (&sv_nostep);

	sv.lightstyles.resize(MAX_LIGHTSTYLES);
}

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/

/*  
==================
SV_StartParticle

Make sure the event gets sent to all clients
==================
*/
void SV_StartParticle (const vec3_t org, const vec3_t dir, int color, int count)
{
	int		i, v;

	auto write_non_expanded = (sv.datagram.maxsize == 0 || sv.datagram.cursize <= MAX_DATAGRAM-16);

	MSG_WriteByte (&sv.datagram_expanded, svc_expandedparticle);
	MSG_WriteFloat (&sv.datagram_expanded, org[0]);
	MSG_WriteFloat (&sv.datagram_expanded, org[1]);
	MSG_WriteFloat (&sv.datagram_expanded, org[2]);
	if (write_non_expanded)
	{
		MSG_WriteByte (&sv.datagram, svc_particle);
		MSG_WriteCoord (&sv.datagram, org[0]);
		MSG_WriteCoord (&sv.datagram, org[1]);
		MSG_WriteCoord (&sv.datagram, org[2]);
	}
	auto offset = sv.datagram_expanded.cursize;
	for (i=0 ; i<3 ; i++)
	{
		v = dir[i]*16;
		if (v > 127)
			v = 127;
		else if (v < -128)
			v = -128;
		MSG_WriteChar (&sv.datagram_expanded, v);
	}
	MSG_WriteByte (&sv.datagram_expanded, count);
	MSG_WriteByte (&sv.datagram_expanded, color);

	if (write_non_expanded)
		SZ_Write (&sv.datagram, sv.datagram_expanded.data.data() + offset, 3 + 2);
}

/*  
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

==================
*/  
void SV_StartSound (edict_t *entity, int channel, const char *sample, int volume,
    float attenuation)
{       
    int         sound_num;
    int field_mask;
    int			i;
	int			ent;
	vec3_t		pos;
	
	if (volume < 0 || volume > 255)
		Sys_Error ("SV_StartSound: volume = %i", volume);

	if (attenuation < 0 || attenuation > 4)
		Sys_Error ("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 7)
		Sys_Error ("SV_StartSound: channel = %i", channel);

// find precache number for sound
    for (sound_num=1 ; sound_num<sv.sound_precache.size() ; sound_num++)
        if (!strcmp(sample, sv.sound_precache[sound_num].c_str()))
            break;
    
    if ( sound_num == sv.sound_precache.size())
    {
        Con_Printf ("SV_StartSound: %s not precacheed\n", sample);
        return;
    }
    
	ent = NUM_FOR_EDICT(entity);

	field_mask = 0;
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		field_mask |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		field_mask |= SND_ATTENUATION;

	for (i=0 ; i<3 ; i++)
		pos[i] = entity->v.origin[i]+0.5*(entity->v.mins[i]+entity->v.maxs[i]);

// directed messages go only to the entity the are targeted on
	if (!sv.active && !sv_bump_protocol_version && (ent > 8191 || channel > 7 || sound_num > 255 || pos[0] < -4096 || pos[0] >= 4096 || pos[1] < -4096 || pos[1] >= 4096 || pos[2] < -4096 || pos[2] >= 4096))
	{
		sv_bump_protocol_version = true;
	}

	auto write_non_expanded = (sv.datagram.maxsize == 0 || sv.datagram.cursize <= MAX_DATAGRAM-16);

	MSG_WriteByte (&sv.datagram_expanded, svc_expandedsound);
	if (write_non_expanded)
		MSG_WriteByte (&sv.datagram, svc_sound);

	auto offset = sv.datagram_expanded.cursize;
	MSG_WriteByte (&sv.datagram_expanded, field_mask);
	if (field_mask & SND_VOLUME)
		MSG_WriteByte (&sv.datagram_expanded, volume);
	if (field_mask & SND_ATTENUATION)
		MSG_WriteByte (&sv.datagram_expanded, attenuation*64);
	if (write_non_expanded)
	{
		auto length = sv.datagram_expanded.cursize - offset;
		SZ_Write (&sv.datagram, sv.datagram_expanded.data.data() + offset, length);
	}

	MSG_WriteLong (&sv.datagram_expanded, ent);
	MSG_WriteLong (&sv.datagram_expanded, channel);
	MSG_WriteLong (&sv.datagram_expanded, sound_num);
	for (i=0 ; i<3 ; i++)
		MSG_WriteFloat (&sv.datagram_expanded, entity->v.origin[i]+0.5*(entity->v.mins[i]+entity->v.maxs[i]));

	if (write_non_expanded)
	{
		channel = (ent<<3) | channel;

		MSG_WriteShort (&sv.datagram, channel);
		MSG_WriteByte (&sv.datagram, sound_num);
		for (i=0 ; i<3 ; i++)
			MSG_WriteCoord (&sv.datagram, entity->v.origin[i]+0.5*(entity->v.mins[i]+entity->v.maxs[i]));
	}
}

/*
==============================================================================

CLIENT SPAWNING

==============================================================================
*/

/*
================
SV_SendServerinfo

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_SendServerinfo (client_t *client)
{
	client->message.maxsize = 0;
	MSG_WriteByte (&client->message, svc_print);
	char version_message[64];
	sprintf (version_message, "%c\nVERSION %4.2f SERVER (%i CRC)", 2, VERSION, pr_crc);
	MSG_WriteString (&client->message,version_message);

	MSG_WriteByte (&client->message, svc_serverinfo);
	client->serverinfo_protocol_offset = client->message.cursize;
	MSG_WriteLong (&client->message, sv_protocol_version);
	MSG_WriteByte (&client->message, svs.maxclients);

	if (!coop.value && deathmatch.value)
		MSG_WriteByte (&client->message, GAME_DEATHMATCH);
	else
		MSG_WriteByte (&client->message, GAME_COOP);

	std::string message = pr_strings+((edict_t*)sv.edicts.data())->v.message;

	MSG_WriteString (&client->message,message.c_str());

	for (size_t i = 1; i < sv.model_precache.size(); i++)
		MSG_WriteString (&client->message, sv.model_precache[i].c_str());
	MSG_WriteByte (&client->message, 0);

	for (size_t i = 1; i < sv.sound_precache.size(); i++)
		MSG_WriteString (&client->message, sv.sound_precache[i].c_str());
	MSG_WriteByte (&client->message, 0);

// send music
	MSG_WriteByte (&client->message, svc_cdtrack);
	MSG_WriteByte (&client->message, ((edict_t*)sv.edicts.data())->v.sounds);
	MSG_WriteByte (&client->message, ((edict_t*)sv.edicts.data())->v.sounds);

// set view	
	MSG_WriteByte (&client->message, svc_setview);
	MSG_WriteShort (&client->message, NUM_FOR_EDICT(client->edict));

	MSG_WriteByte (&client->message, svc_signonnum);
	MSG_WriteByte (&client->message, 1);

	client->sendsignon = true;
	client->spawned = false;		// need prespawn, spawn, etc
}

/*
================
SV_ConnectClient

Initializes a client_t for a new net connection.  This will only be called
once for a player each game, not once for each level change.
================
*/
void SV_ConnectClient (int clientnum)
{
	edict_t			*ent;
	client_t		*client;
	int				edictnum;
	struct qsocket_s *netconnection;
	int				i;
	float			spawn_parms[NUM_SPAWN_PARMS];

	client = svs.clients.data() + clientnum;

	Con_DPrintf ("Client %s connected\n", client->netconnection->address.c_str());

	edictnum = clientnum+1;

	ent = EDICT_NUM(edictnum);
	
// set up the client_t
	netconnection = client->netconnection;
	
	if (sv.loadgame)
		memcpy (spawn_parms, client->spawn_parms, sizeof(spawn_parms));
	client->Clear();
	client->netconnection = netconnection;

	client->name = ED_NewString(12);
	strcpy (pr_strings + client->name, "unconnected");
	client->active = true;
	client->spawned = false;
	client->edict = ent;
	client->protocol_version = sv_protocol_version;
	client->message.maxsize = (sv_protocol_version == EXPANDED_PROTOCOL_VERSION ? 0 : MAX_MSGLEN);
	client->message.allowoverflow = true;		// we can catch it

#ifdef IDGODS
	client->privileged = IsID(&client->netconnection->addr);
#else	
	client->privileged = false;				
#endif

	if (sv.loadgame)
		memcpy (client->spawn_parms, spawn_parms, sizeof(spawn_parms));
	else
	{
	// call the progs to get default spawn parms for the new client
		PR_ExecuteProgram (pr_global_struct->SetNewParms);
		for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
			client->spawn_parms[i] = (&pr_global_struct->parm1)[i];
	}

	SV_SendServerinfo (client);
}


/*
===================
SV_CheckForNewClients

===================
*/
void SV_CheckForNewClients (void)
{
	struct qsocket_s	*ret;
	int				i;
		
//
// check for new connections
//
	while (1)
	{
		ret = NET_CheckNewConnections ();
		if (!ret)
			break;

	// 
	// init a new client structure
	//	
		for (i=0 ; i<svs.maxclients ; i++)
			if (!svs.clients[i].active)
				break;
		if (i == svs.maxclients)
			Sys_Error ("Host_CheckForNewClients: no free clients");
		
		svs.clients[i].netconnection = ret;
		SV_ConnectClient (i);	
	
		net_activeconnections++;
	}
}



/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

/*
==================
SV_ClearDatagram

==================
*/
void SV_ClearDatagram (void)
{
	SZ_Clear (&sv.datagram);
	SZ_Clear (&sv.datagram_expanded);
}

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

int		fatbytes;
std::vector<byte>	fatpvs;

void SV_AddToFatPVS (const vec3_t org, mnode_t *node)
{
	int		i;
	byte	*pvs;
	mplane_t	*plane;
	float	d;

	auto fatpvs64 = (uint64_t*)fatpvs.data();
	auto trailing = fatbytes - fatbytes % 8;
	while (1)
	{
	// if this is a leaf, accumulate the pvs bits
		if (node->contents < 0)
		{
			if (node->contents != CONTENTS_SOLID)
			{
				pvs = Mod_LeafPVS ( (mleaf_t *)node, sv.worldmodel);
				auto pvs64 = (uint64_t*)pvs;
				for (i=0 ; i<trailing ; i+=8)
					*fatpvs64++ |= *pvs64++;
				for (i=trailing ; i<fatbytes ; i++)
					fatpvs[i] |= pvs[i];
			}
			return;
		}
	
		plane = node->plane;
		d = DotProduct (org, plane->normal) - plane->dist;
		if (d > 8)
			node = node->children[0];
		else if (d < -8)
			node = node->children[1];
		else
		{	// go down both
			SV_AddToFatPVS (org, node->children[0]);
			node = node->children[1];
		}
	}
}

/*
=============
SV_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
static byte *SV_FatPVS (const vec3_t org)
{
	fatbytes = (sv.worldmodel->numleafs+31)>>3;
	if (fatbytes > (int)fatpvs.size())
	{
		fatpvs.resize(fatbytes);
	}
	Q_memset (fatpvs.data(), 0, fatbytes);
	SV_AddToFatPVS (org, sv.worldmodel->nodes);
	return fatpvs.data();
}

//=============================================================================


/*
=============
SV_WriteEntitiesToClient

=============
*/
void SV_WriteEntitiesToClient (edict_t	*clent, sizebuf_t *msg, int protocol_version)
{
	int		e, i;
	int		bits;
	byte	*pvs;
	vec3_t	org;
	float	miss;
	edict_t	*ent;

// find the client's PVS
	VectorAdd (clent->v.origin, clent->v.view_ofs, org);
	pvs = SV_FatPVS (org);

// send over all entities (excpet the client) that touch the pvs
	ent = NEXT_EDICT(sv.edicts.data());
	for (e=1 ; e<sv.num_edicts ; e++, ent = NEXT_EDICT(ent))
	{
#ifdef QUAKE2
		// don't send if flagged for NODRAW and there are no lighting effects
		if (ent->v.effects == EF_NODRAW)
			continue;
#endif

// ignore if not touching a PV leaf
		if (ent != clent)	// clent is ALLWAYS sent
		{
// ignore ents without visible models
			if (ent->leafnums == nullptr || !ent->v.modelindex || !pr_strings[ent->v.model])
				continue;

			for (i=0 ; i < ent->num_leafs ; i++)
				if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i]&7) ))
					break;

			if (i == ent->num_leafs && ent->num_leafs < ent->leaf_size)
				continue;		// not visible
		}

		if (msg->maxsize > 0 && protocol_version == PROTOCOL_VERSION && msg->maxsize - msg->cursize < 16)
		{
			Con_Printf ("packet overflow\n");
			sv_request_protocol_version_upgrade = true;
			return;
		}

// send an update
		bits = 0;
		
		for (i=0 ; i<3 ; i++)
		{
			miss = ent->v.origin[i] - ent->baseline.origin[i];
			if ( miss < -0.1 || miss > 0.1 )
				bits |= U_ORIGIN1<<i;
		}

		if ( ent->v.angles[0] != ent->baseline.angles[0] )
			bits |= U_ANGLE1;
			
		if ( ent->v.angles[1] != ent->baseline.angles[1] )
			bits |= U_ANGLE2;
			
		if ( ent->v.angles[2] != ent->baseline.angles[2] )
			bits |= U_ANGLE3;
			
		if (ent->v.movetype == MOVETYPE_STEP)
			bits |= U_NOLERP;	// don't mess up the step animation
	
		if (ent->baseline.colormap != ent->v.colormap)
			bits |= U_COLORMAP;
			
		if (ent->baseline.skin != ent->v.skin)
			bits |= U_SKIN;
			
		if (ent->baseline.frame != ent->v.frame)
			bits |= U_FRAME;
		
		if (ent->baseline.effects != ent->v.effects)
			bits |= U_EFFECTS;
		
		if (ent->baseline.modelindex != ent->v.modelindex)
			bits |= U_MODEL;

		if (pr_alpha_ofs >= 0)
			if (protocol_version == EXPANDED_PROTOCOL_VERSION)
				bits |= U_ALPHA;
			else
				sv_request_protocol_version_upgrade = true;

		if (pr_scale_ofs >= 0)
			if (protocol_version == EXPANDED_PROTOCOL_VERSION)
				bits |= U_SCALE;
			else
				sv_request_protocol_version_upgrade = true;

		if (e >= 256)
			bits |= U_LONGENTITY;
			
		if (bits >= 256)
			bits |= U_MOREBITS;

		if (protocol_version == EXPANDED_PROTOCOL_VERSION && bits >= 65536)
			bits |= U_EVENMOREBITS;

	//
	// write the message
	//
		if (protocol_version == EXPANDED_PROTOCOL_VERSION)
		{
			auto oldsize = msg->cursize;
			auto oldmaxsize = msg->maxsize;
			msg->maxsize = 0;
			
			MSG_WriteByte (msg,bits | U_SIGNAL);
			
			if (bits & U_MOREBITS)
				MSG_WriteByte (msg, bits>>8);
			if (bits & U_EVENMOREBITS)
				MSG_WriteByte (msg, bits>>16);
			MSG_WriteLong (msg, e);
			
			if (bits & U_MODEL)
				MSG_WriteLong (msg, ent->v.modelindex);
			if (bits & U_FRAME)
				MSG_WriteLong (msg, ent->v.frame);
			if (bits & U_COLORMAP)
				MSG_WriteByte (msg, ent->v.colormap);
			if (bits & U_SKIN)
				MSG_WriteByte (msg, ent->v.skin);
			if (bits & U_EFFECTS)
				MSG_WriteByte (msg, ent->v.effects);
			if (bits & U_ORIGIN1)
				MSG_WriteFloat (msg, ent->v.origin[0]);
			if (bits & U_ANGLE1)
				MSG_WriteFloat(msg, ent->v.angles[0]);
			if (bits & U_ORIGIN2)
				MSG_WriteFloat (msg, ent->v.origin[1]);
			if (bits & U_ANGLE2)
				MSG_WriteFloat(msg, ent->v.angles[1]);
			if (bits & U_ORIGIN3)
				MSG_WriteFloat (msg, ent->v.origin[2]);
			if (bits & U_ANGLE3)
				MSG_WriteFloat(msg, ent->v.angles[2]);
			if (bits & U_ALPHA)
			{
				auto alpha = ((eval_t *)((char *)&ent->v + pr_alpha_ofs*4))->_float;
				if (alpha != 0)
				{
					alpha = fmin(fmax(1.f + 254.f * alpha, 1.f), 255.f);
					MSG_WriteByte (msg, (int)floor(alpha + 0.5f));
				}
				else
				{
					MSG_WriteByte (msg, 0);
				}
			}
			if (bits & U_SCALE)
			{
				auto scale = ((eval_t *)((char *)&ent->v + pr_scale_ofs*4))->_float;
				if (scale != 0)
				{
					scale = fmin(fmax(1.f + 254.f * scale, 1.f), 255.f);
					MSG_WriteByte (msg, (int)floor(scale + 0.5f));
				}
				else
				{
					MSG_WriteByte (msg, 0);
				}
			}

			msg->maxsize = oldmaxsize;
			if (oldmaxsize > 0 && msg->cursize > oldmaxsize)
			{
				msg->data.resize(oldsize);
				Con_Printf ("packet overflow\n");
				return;
			}
			continue;
		}
		MSG_WriteByte (msg,bits | U_SIGNAL);
		
		if (bits & U_MOREBITS)
			MSG_WriteByte (msg, bits>>8);
		if (bits & U_LONGENTITY)
			MSG_WriteShort (msg,e);
		else
			MSG_WriteByte (msg,e);

		if (bits & U_MODEL)
			MSG_WriteByte (msg,	ent->v.modelindex);
		if (bits & U_FRAME)
			MSG_WriteByte (msg, ent->v.frame);
		if (bits & U_COLORMAP)
			MSG_WriteByte (msg, ent->v.colormap);
		if (bits & U_SKIN)
			MSG_WriteByte (msg, ent->v.skin);
		if (bits & U_EFFECTS)
			MSG_WriteByte (msg, ent->v.effects);
		if (bits & U_ORIGIN1)
			MSG_WriteCoord (msg, ent->v.origin[0]);		
		if (bits & U_ANGLE1)
			MSG_WriteAngle(msg, ent->v.angles[0]);
		if (bits & U_ORIGIN2)
			MSG_WriteCoord (msg, ent->v.origin[1]);
		if (bits & U_ANGLE2)
			MSG_WriteAngle(msg, ent->v.angles[1]);
		if (bits & U_ORIGIN3)
			MSG_WriteCoord (msg, ent->v.origin[2]);
		if (bits & U_ANGLE3)
			MSG_WriteAngle(msg, ent->v.angles[2]);
	}
}

/*
=============
SV_CleanupEnts

=============
*/
void SV_CleanupEnts (void)
{
	int		e;
	edict_t	*ent;
	
	ent = NEXT_EDICT(sv.edicts.data());
	for (e=1 ; e<sv.num_edicts ; e++, ent = NEXT_EDICT(ent))
	{
		ent->v.effects = (int)ent->v.effects & ~EF_MUZZLEFLASH;
	}

}

/*
==================
SV_WriteClientdataToMessage

==================
*/
void SV_WriteClientdataToMessage (edict_t *ent, sizebuf_t *msg, int protocol_version)
{
	int		bits;
	int		i;
	edict_t	*other;
	int		items;
#ifndef QUAKE2
	eval_t	*val;
#endif

//
// send a damage message
//
	if (ent->v.dmg_take || ent->v.dmg_save)
	{
		other = PROG_TO_EDICT(ent->v.dmg_inflictor);
		if (protocol_version == EXPANDED_PROTOCOL_VERSION)
		{
			MSG_WriteByte(msg, svc_expandeddamage);
		}
		else
		{
			MSG_WriteByte (msg, svc_damage);
		}
		MSG_WriteByte (msg, ent->v.dmg_save);
		MSG_WriteByte (msg, ent->v.dmg_take);
		if (protocol_version == EXPANDED_PROTOCOL_VERSION)
		{
			for (i=0 ; i<3 ; i++)
				MSG_WriteFloat (msg, other->v.origin[i] + 0.5*(other->v.mins[i] + other->v.maxs[i]));
		}
		else
		{
			for (i=0 ; i<3 ; i++)
				MSG_WriteCoord (msg, other->v.origin[i] + 0.5*(other->v.mins[i] + other->v.maxs[i]));
		}
	
		ent->v.dmg_take = 0;
		ent->v.dmg_save = 0;
	}

//
// send the current viewpos offset from the view entity
//
	SV_SetIdealPitch ();		// how much to look up / down ideally

// a fixangle might get lost in a dropped packet.  Oh well.
	if ( ent->v.fixangle )
	{
		MSG_WriteByte (msg, svc_setangle);
		for (i=0 ; i < 3 ; i++)
			MSG_WriteAngle (msg, ent->v.angles[i] );
		ent->v.fixangle = 0;
	}

	bits = 0;
	
	if (ent->v.view_ofs[2] != DEFAULT_VIEWHEIGHT)
		bits |= SU_VIEWHEIGHT;
		
	if (ent->v.idealpitch)
		bits |= SU_IDEALPITCH;

// stuff the sigil bits into the high bits of items for sbar, or else
// mix in items2
#ifdef QUAKE2
	items = (int)ent->v.items | ((int)ent->v.items2 << 23);
#else
	if (pr_items2_ofs >= 0)
		items = (int)ent->v.items | ((int)((eval_t *)((char *)&ent->v + pr_items2_ofs*4))->_float << 23);
	else
		items = (int)ent->v.items | ((int)pr_global_struct->serverflags << 28);
#endif

	bits |= SU_ITEMS;
	
	if ( (int)ent->v.flags & FL_ONGROUND)
		bits |= SU_ONGROUND;
	
	if ( ent->v.waterlevel >= 2)
		bits |= SU_INWATER;
	
	for (i=0 ; i<3 ; i++)
	{
		if (ent->v.punchangle[i])
			bits |= (SU_PUNCH1<<i);
		if (ent->v.velocity[i])
			bits |= (SU_VELOCITY1<<i);
	}
	
	if (ent->v.weaponframe)
		bits |= SU_WEAPONFRAME;

	if (ent->v.armorvalue)
		bits |= SU_ARMOR;

//	if (ent->v.weapon)
		bits |= SU_WEAPON;

// send the data
	if (protocol_version == EXPANDED_PROTOCOL_VERSION)
	{
		MSG_WriteByte (msg, svc_expandedclientdata);
		MSG_WriteLong (msg, bits);
		
		if (bits & SU_VIEWHEIGHT)
			MSG_WriteChar (msg, ent->v.view_ofs[2]);
		
		if (bits & SU_IDEALPITCH)
			MSG_WriteChar (msg, ent->v.idealpitch);
		
		for (i=0 ; i<3 ; i++)
		{
			if (bits & (SU_PUNCH1<<i))
				MSG_WriteChar (msg, ent->v.punchangle[i]);
			if (bits & (SU_VELOCITY1<<i))
				MSG_WriteChar (msg, ent->v.velocity[i]/16);
		}
		
		MSG_WriteLong (msg, items);

		if (bits & SU_WEAPONFRAME)
			MSG_WriteLong (msg, ent->v.weaponframe);
		if (bits & SU_ARMOR)
			MSG_WriteLong (msg, ent->v.armorvalue);
		if (bits & SU_WEAPON)
			MSG_WriteLong (msg, SV_ModelIndex(pr_strings+ent->v.weaponmodel));
		
		MSG_WriteLong (msg, ent->v.health);
		MSG_WriteLong (msg, ent->v.currentammo);
		MSG_WriteLong (msg, ent->v.ammo_shells);
		MSG_WriteLong (msg, ent->v.ammo_nails);
		MSG_WriteLong (msg, ent->v.ammo_rockets);
		MSG_WriteLong (msg, ent->v.ammo_cells);
		
		if (standard_quake)
		{
			MSG_WriteLong (msg, ent->v.weapon);
		}
		else
		{
			for(i=0;i<32;i++)
			{
				if ( ((int)ent->v.weapon) & (1<<i) )
				{
					MSG_WriteLong (msg, i);
					break;
				}
			}
		}
		return;
	}

	if (sv_request_protocol_version_upgrade)
		bits |= SU_REQEXPPROTO;

	MSG_WriteByte (msg, svc_clientdata);
	MSG_WriteShort (msg, bits);

	if (bits & SU_VIEWHEIGHT)
		MSG_WriteChar (msg, ent->v.view_ofs[2]);

	if (bits & SU_IDEALPITCH)
		MSG_WriteChar (msg, ent->v.idealpitch);

	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i))
			MSG_WriteChar (msg, ent->v.punchangle[i]);
		if (bits & (SU_VELOCITY1<<i))
			MSG_WriteChar (msg, ent->v.velocity[i]/16);
	}

// [always sent]	if (bits & SU_ITEMS)
	MSG_WriteLong (msg, items);

	if (bits & SU_WEAPONFRAME)
		MSG_WriteByte (msg, ent->v.weaponframe);
	if (bits & SU_ARMOR)
		MSG_WriteByte (msg, ent->v.armorvalue);
	if (bits & SU_WEAPON)
		MSG_WriteByte (msg, SV_ModelIndex(pr_strings+ent->v.weaponmodel));
	
	MSG_WriteShort (msg, ent->v.health);
	MSG_WriteByte (msg, ent->v.currentammo);
	MSG_WriteByte (msg, ent->v.ammo_shells);
	MSG_WriteByte (msg, ent->v.ammo_nails);
	MSG_WriteByte (msg, ent->v.ammo_rockets);
	MSG_WriteByte (msg, ent->v.ammo_cells);

	if (standard_quake)
	{
		MSG_WriteByte (msg, ent->v.weapon);
	}
	else
	{
		for(i=0;i<32;i++)
		{
			if ( ((int)ent->v.weapon) & (1<<i) )
			{
				MSG_WriteByte (msg, i);
				break;
			}
		}
	}
}

/*
=======================
SV_SendClientDatagram
=======================
*/
qboolean SV_SendClientDatagram (client_t *client)
{
	static sizebuf_t	msg;
	
	msg.maxsize = MAX_DATAGRAM;
	msg.cursize = 0;
	if (client->protocol_version == EXPANDED_PROTOCOL_VERSION)
	{
		int size = NET_MaxUnreliableMessageSize(client->netconnection);
		if (size >= 0)
			msg.maxsize = size;
	}

	MSG_WriteByte (&msg, svc_time);
	MSG_WriteFloat (&msg, sv.time);

// add the client specific data to the datagram
	SV_WriteClientdataToMessage (client->edict, &msg, client->protocol_version);

	SV_WriteEntitiesToClient (client->edict, &msg, client->protocol_version);

// copy the server datagram if there is space
	sizebuf_t* source;
	if (client->protocol_version == EXPANDED_PROTOCOL_VERSION)
		source = &sv.datagram_expanded;
	else
		source = &sv.datagram;
	if (msg.maxsize == 0 || msg.cursize + source->cursize < msg.maxsize)
		SZ_Write (&msg, (void*)source->data.data(), source->cursize);

// send the datagram
	if (NET_SendUnreliableMessage (client->netconnection, &msg) == -1)
	{
		SV_DropClient (true);// if the message couldn't send, kick off
		return false;
	}
	
	return true;
}

/*
=======================
SV_UpdateToReliableMessages
=======================
*/
void SV_UpdateToReliableMessages (void)
{
	int			i, j;
	client_t *client;

// check for changes to be sent over the reliable streams
	for (i=0, host_client = svs.clients.data() ; i<svs.maxclients ; i++, host_client++)
	{
		if (host_client->old_frags != host_client->edict->v.frags)
		{
			for (j=0, client = svs.clients.data() ; j<svs.maxclients ; j++, client++)
			{
				if (!client->active)
					continue;
				MSG_WriteByte (&client->message, svc_updatefrags);
				MSG_WriteByte (&client->message, i);
				MSG_WriteShort (&client->message, host_client->edict->v.frags);
			}

			host_client->old_frags = host_client->edict->v.frags;
		}
	}
	
	for (j=0, client = svs.clients.data() ; j<svs.maxclients ; j++, client++)
	{
		if (!client->active)
			continue;
		sizebuf_t* source;
		if (client->protocol_version == EXPANDED_PROTOCOL_VERSION)
			source = &sv.reliable_datagram_expanded;
		else
			source = &sv.reliable_datagram;
		SZ_Write (&client->message, (void*)source->data.data(), source->cursize);
	}

	SZ_Clear (&sv.reliable_datagram);
	SZ_Clear (&sv.reliable_datagram_expanded);
}


/*
=======================
SV_SendNop

Send a nop message without trashing or sending the accumulated client
message buffer
=======================
*/
void SV_SendNop (client_t *client)
{
	static sizebuf_t	msg;
	
	msg.maxsize = 4;
	msg.cursize = 0;

	MSG_WriteChar (&msg, svc_nop);

	if (NET_SendUnreliableMessage (client->netconnection, &msg) == -1)
		SV_DropClient (true);	// if the message couldn't send, kick off
	client->last_message = realtime;
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages (void)
{
	int			i;
	
// update frags, names, etc
	SV_UpdateToReliableMessages ();

// build individual updates
	for (i=0, host_client = svs.clients.data() ; i<svs.maxclients ; i++, host_client++)
	{
		if (!host_client->active)
			continue;

		if (host_client->spawned)
		{
			if (!SV_SendClientDatagram (host_client))
				continue;
		}
		else
		{
		// the player isn't totally in the game yet
		// send small keepalive messages if too much time has passed
		// send a full message when the next signon stage has been requested
		// some other message data (name changes, etc) may accumulate 
		// between signon stages
			if (!host_client->sendsignon)
			{
				if (realtime - host_client->last_message > 5)
					SV_SendNop (host_client);
				continue;	// don't send out non-signon messages
			}
		}

		// check for an overflowed message.  Should only happen
		// on a very fucked up connection that backs up a lot, then
		// changes level
		if (host_client->message.overflowed)
		{
			SV_DropClient (true);
			host_client->message.overflowed = false;
			continue;
		}
			
		if (host_client->message.cursize || host_client->dropasap)
		{
			if (!NET_CanSendMessage (host_client->netconnection))
			{
//				I_Printf ("can't write\n");
				continue;
			}

			if (host_client->dropasap)
				SV_DropClient (false);	// went to another level
			else
			{
				if (NET_SendMessage (host_client->netconnection
				, &host_client->message) == -1)
					SV_DropClient (true);	// if the message couldn't send, kick off
				SZ_Clear (&host_client->message);
				host_client->last_message = realtime;
				host_client->sendsignon = false;
			}
		}
	}
	
	
// clear muzzle flashes
	SV_CleanupEnts ();
}


/*
==============================================================================

SERVER SPAWNING

==============================================================================
*/

/*
================
SV_ModelIndex

================
*/
int SV_ModelIndex (const char *name)
{
	int		i;
	
	if (!name || !name[0])
		return 0;

	i = -1;
	auto entry = sv.model_index.find(name);
	if (entry != sv.model_index.end())
		i = entry->second;
	else
		Sys_Error ("SV_ModelIndex: model %s not precached", name);
	return i;
}

/*
================
SV_CreateBaseline

================
*/
void SV_CreateBaseline (void)
{
	int			i;
	edict_t			*svent;
	int				entnum;	
		
	for (entnum = 0; entnum < sv.num_edicts ; entnum++)
	{
	// get the current server version
		svent = EDICT_NUM(entnum);
		if (svent->free)
			continue;
		if (entnum > svs.maxclients && !svent->v.modelindex)
			continue;

	//
	// create entity baseline
	//
		VectorCopy (svent->v.origin, svent->baseline.origin);
		VectorCopy (svent->v.angles, svent->baseline.angles);
		svent->baseline.frame = svent->v.frame;
		svent->baseline.skin = svent->v.skin;
		if (entnum > 0 && entnum <= svs.maxclients)
		{
			svent->baseline.colormap = entnum;
			svent->baseline.modelindex = SV_ModelIndex("progs/player.mdl");
		}
		else
		{
			svent->baseline.colormap = 0;
			svent->baseline.modelindex =
				SV_ModelIndex(pr_strings + svent->v.model);
		}
		if (pr_alpha_ofs >= 0)
		{
			auto alpha = ((eval_t *)((char *)&svent->v + pr_alpha_ofs*4))->_float;
			if (alpha != 0)
			{
				alpha = fmin(fmax(1.f + 254.f * alpha, 1.f), 255.f);
				svent->baseline.alpha = (int)floor(alpha + 0.5f);
			}
			else
			{
				svent->baseline.alpha = 0;
			}
		}
		else
		{
			svent->baseline.alpha = 0;
		}
		if (pr_scale_ofs >= 0)
		{
			auto scale = ((eval_t *)((char *)&svent->v + pr_scale_ofs*4))->_float;
			if (scale != 0)
			{
				scale = fmin(fmax(1.f + 254.f * scale, 1.f), 255.f);
				svent->baseline.scale = (int)floor(scale + 0.5f);
			}
			else
			{
				svent->baseline.scale = 0;
			}
		}
		else
		{
			svent->baseline.scale = 0;
		}
		
	//
	// add to the message
	//
		if (sv_protocol_version == EXPANDED_PROTOCOL_VERSION)
		{
			MSG_WriteByte (&sv.signon,svc_expandedspawnbaseline);
			MSG_WriteLong (&sv.signon,entnum);
			
			MSG_WriteLong (&sv.signon, svent->baseline.modelindex);
			MSG_WriteLong (&sv.signon, svent->baseline.frame);
		}
		else
		{
			MSG_WriteByte (&sv.signon,svc_spawnbaseline);
			MSG_WriteShort (&sv.signon,entnum);

			MSG_WriteByte (&sv.signon, svent->baseline.modelindex);
			MSG_WriteByte (&sv.signon, svent->baseline.frame);
		}
		MSG_WriteByte (&sv.signon, svent->baseline.colormap);
		MSG_WriteByte (&sv.signon, svent->baseline.skin);
		if (sv_protocol_version == EXPANDED_PROTOCOL_VERSION)
		{ 
			for (i=0 ; i<3 ; i++)
			{
				MSG_WriteFloat(&sv.signon, svent->baseline.origin[i]);
				MSG_WriteFloat(&sv.signon, svent->baseline.angles[i]);
			}
			MSG_WriteByte(&sv.signon, svent->baseline.alpha);
			MSG_WriteByte(&sv.signon, svent->baseline.scale);
		}
		else
		{
			for (i=0 ; i<3 ; i++)
			{
				MSG_WriteCoord(&sv.signon, svent->baseline.origin[i]);
				MSG_WriteAngle(&sv.signon, svent->baseline.angles[i]);
			}
		}
	}
}


/*
================
SV_SendReconnect

Tell all the clients that the server is changing levels
================
*/
void SV_SendReconnect (void)
{
	static sizebuf_t	msg;

	msg.maxsize = 128;
	msg.cursize = 0;

	MSG_WriteChar (&msg, svc_stufftext);
	MSG_WriteString (&msg, "reconnect\n");
	NET_SendToAll (&msg, 5);
	
	if (cls.state != ca_dedicated)
#ifdef QUAKE2
		Cbuf_InsertText ("reconnect\n");
#else
		Cmd_ExecuteString ("reconnect\n", src_command);
#endif
}


/*
================
SV_SaveSpawnparms

Grabs the current state of each client for saving across the
transition to another level
================
*/
void SV_SaveSpawnparms (void)
{
	int		i, j;

	svs.serverflags = pr_global_struct->serverflags;

	for (i=0, host_client = svs.clients.data() ; i<svs.maxclients ; i++, host_client++)
	{
		if (!host_client->active)
			continue;

	// call the progs to get default spawn parms for the new client
		pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
		PR_ExecuteProgram (pr_global_struct->SetChangeParms);
		for (j=0 ; j<NUM_SPAWN_PARMS ; j++)
			host_client->spawn_parms[j] = (&pr_global_struct->parm1)[j];
	}
}


/*
================
SV_SpawnServer

This is called at the start of each level
================
*/
extern float		scr_centertime_off;
extern qboolean		snd_forceclear;

#ifdef QUAKE2
void SV_SpawnServer (char *server, char *startspot)
#else
void SV_SpawnServer (char *server)
#endif
{
	edict_t		*ent;
	int			i;

	// let's not have any servers with no name
	if (hostname.string[0] == 0)
		Cvar_Set ("hostname", "UNNAMED");
	scr_centertime_off = 0;
	snd_forceclear = true;

	Con_DPrintf ("SpawnServer: %s\n",server);
	svs.changelevel_issued = false;		// now safe to issue another

//
// tell all connected clients that we are going to a new level
//
	if (sv.active)
	{
		SV_SendReconnect ();
	}

//
// make cvars consistant
//
	if (coop.value)
		Cvar_SetValue ("deathmatch", 0);
	current_skill = (int)(skill.value + 0.5);
	if (current_skill < 0)
		current_skill = 0;
	if (current_skill > 3)
		current_skill = 3;

	Cvar_SetValue ("skill", (float)current_skill);
	
//
// set up the new server
//
	Host_ClearMemory ();

	sv.Clear();

	auto name_len = (int)strlen(server);
	sv.name = ED_NewString(name_len + 1);
	Q_strncpy(pr_strings + sv.name, server, name_len);
#ifdef QUAKE2
	if (startspot)
		strcpy(sv.startspot, startspot);
#endif

// load progs to get entity field count
	PR_LoadProgs ();

// allocate server memory
	sv.max_edicts = MAX_EDICTS;
	if (max_edicts.value > 0)
	{
		sv.max_edicts = (int)max_edicts.value;
	}

	SV_ResizeEdicts(sv.max_edicts * pr_edict_size);
	sv.edicts_reallocation_sequence++;

	sv.datagram.maxsize = MAX_DATAGRAM;
	sv.datagram.cursize = 0;

	sv.datagram_expanded.maxsize = 0;
	sv.datagram_expanded.cursize = 0;

	sv.reliable_datagram.maxsize = MAX_DATAGRAM;
	sv.reliable_datagram.cursize = 0;
	
	sv.reliable_datagram_expanded.maxsize = 0;
	sv.reliable_datagram_expanded.cursize = 0;
	
	sv.signon.maxsize = 0;
	sv.signon.cursize = 0;
	
	pr_string_temp = ED_NewString(128);
	
// leave slots at start for clients only
	sv.num_edicts = svs.maxclients+1;
	for (i=0 ; i<svs.maxclients ; i++)
	{
		ent = EDICT_NUM(i+1);
		svs.clients[i].edict = ent;
	}
	
	sv.state = ss_loading;
	sv.paused = false;

	sv.time = 1.0;
	
	name_len = (int)strlen(server);
	sv.name = ED_NewString(name_len + 1);
	Q_strncpy(pr_strings + sv.name, server, name_len);
	sprintf (sv.modelname,"maps/%s.bsp", server);
	sv.worldmodel = Mod_ForName (sv.modelname, false);
	if (!sv.worldmodel)
	{
		Con_Printf ("Couldn't spawn server %s\n", sv.modelname);
		sv.active = false;
		return;
	}
	if (sv.models.empty())
	{
		sv.models.push_back(nullptr);
	}
	sv.models.push_back(sv.worldmodel);
	
//
// clear world interaction links
//
	SV_ClearWorld ();
	
	sv.sound_precache.emplace_back(pr_strings);

	sv.model_precache.emplace_back(pr_strings);
	sv.model_index.insert({ pr_strings, 0 });
	sv.model_precache.emplace_back(sv.modelname);
	sv.model_index.insert({ sv.modelname, 1 });
	for (i=1 ; i<sv.worldmodel->numsubmodels ; i++)
	{
		std::string name("*");
		name += std::to_string(i);
		sv.model_precache.push_back(name);
		sv.model_index.insert({ name, 1+i});
		sv.models.push_back(Mod_ForName(name.c_str(), false));
	}

//
// load the rest of the entities
//	
	ent = EDICT_NUM(0);
	memset (&ent->v, 0, progs->entityfields * 4);
	ent->free = false;
	name_len = (int)sv.worldmodel->name.length();
	ent->v.model = ED_NewString(name_len + 1);
	Q_strncpy(pr_strings + ent->v.model, sv.worldmodel->name.c_str(), name_len);
	ent->v.modelindex = 1;		// world model
	ent->v.solid = SOLID_BSP;
	ent->v.movetype = MOVETYPE_PUSH;

	if (coop.value)
		pr_global_struct->coop = coop.value;
	else
		pr_global_struct->deathmatch = deathmatch.value;

	pr_global_struct->mapname = sv.name;
#ifdef QUAKE2
	pr_global_struct->startspot = sv.startspot - pr_strings;
#endif

// serverflags are for cross level information (sigils)
	pr_global_struct->serverflags = svs.serverflags;
	sv_static_entity_count = 0;
	sv_bump_protocol_version = false;
	sv_request_protocol_version_upgrade = false;

// set the protocol version to the default value
	sv_protocol_version = PROTOCOL_VERSION;
	
	ED_LoadFromFile (sv.worldmodel->entities);

	sv.active = true;

// perform first protocol version adjustment
	if (sv_bump_protocol_version ||
		sv.models.size() >= MAX_MODELS ||
		sv.sound_precache.size() >= MAX_SOUNDS ||
		sv.num_edicts >= MAX_EDICTS ||
		sv_static_entity_count >= MAX_STATIC_ENTITIES)
	{
		sv_protocol_version = EXPANDED_PROTOCOL_VERSION;
	}
	else
	{
		auto beyond = false;
		for (auto& model : sv.models)
		{
			if (model == nullptr)
			{
				continue;
			}
			if (model->mins[0] < -4096 ||
				model->mins[1] < -4096 ||
				model->mins[2] < -4096 ||
				model->maxs[0] >= 4096 ||
				model->maxs[1] >= 4096 ||
				model->maxs[2] >= 4096)
			{
				beyond = true;
				break;
			}
		}
		if (beyond)
		{
			sv_protocol_version = EXPANDED_PROTOCOL_VERSION;
		}
	}
	sv_bump_protocol_version = false;

// all setup is completed, any further precache statements are errors
	sv.state = ss_active;
	
// run two frames to allow everything to settle
	host_frametime = 0.1;
	SV_Physics ();
	SV_Physics ();

// create a baseline for more efficient communications
	SV_CreateBaseline ();

// use signon message for second protocol version adjustment
	if (sv_protocol_version == PROTOCOL_VERSION && sv.signon.cursize > NET_MAXMESSAGE)
	{
		sv_protocol_version = EXPANDED_PROTOCOL_VERSION;
	}

// send serverinfo to all connected clients
	for (i=0,host_client = svs.clients.data() ; i<svs.maxclients ; i++, host_client++)
		if (host_client->active)
		{
			SV_SendServerinfo (host_client);
			if (sv_protocol_version == PROTOCOL_VERSION && host_client->message.cursize >= MAX_MSGLEN - 64 && !sv_bump_protocol_version)
			{
				sv_bump_protocol_version = true;
			}
		}
	
// use bump flag for the third and final protocol version adjustment
	if (sv_bump_protocol_version)
	{
		sv_protocol_version = EXPANDED_PROTOCOL_VERSION;
		sv_bump_protocol_version = false;
	}

	for (i=0,host_client = svs.clients.data() ; i<svs.maxclients ; i++, host_client++)
		if (host_client->active && host_client->protocol_version != sv_protocol_version)
		{
			host_client->protocol_version = sv_protocol_version;
			MSG_WriteLong (host_client->message.data.data() + host_client->serverinfo_protocol_offset, sv_protocol_version);
		}

// if the protocol was not adjusted at this point, reset the signon limits to their expected values
	if (sv_protocol_version == PROTOCOL_VERSION)
	{
		sv.signon.maxsize = NET_MAXMESSAGE;
	}

	Con_DPrintf ("Server spawned.\n");
}

void SV_DeleteEdictLeafs(size_t start, size_t end)
{
	size_t index = start;
	auto data = sv.edicts.data();
	while (index < end)
	{
		auto e = (edict_t*)(data + index);
		delete[] e->leafnums;
		index += pr_edict_size;
	}
}

void SV_ResizeEdicts(size_t newsize)
{
	size_t oldsize = sv.edicts.size();
	if (oldsize > newsize)
	{
		SV_DeleteEdictLeafs(newsize, oldsize);
		sv.edicts.resize(newsize);
	}
	else if (oldsize < newsize)
	{
		sv.edicts.resize(newsize);
	}
}
