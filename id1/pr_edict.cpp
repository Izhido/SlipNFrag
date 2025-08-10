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
// sv_edict.c -- entity dictionary

#include "quakedef.h"

dprograms_t		*progs;
dfunction_t		*pr_functions;
char			*pr_strings;
ddef_t			*pr_fielddefs;
ddef_t			*pr_globaldefs;
dstatement_t	*pr_statements;
globalvars_t	*pr_global_struct;
float			*pr_globals;			// same as pr_global_struct
int				pr_edict_size;	// in bytes

unsigned short		pr_crc;

int 			pr_items2_ofs;
int 			pr_gravity_ofs;
int 			pr_alpha_ofs;
int 			pr_scale_ofs;

std::unordered_map<std::string_view, int> pr_fielddefs_index;

int		type_size[8] = {1,sizeof(string_t)/4,1,3,1,1,sizeof(func_t)/4,sizeof(void *)/4};

ddef_t *ED_FieldAtOfs (int ofs);
qboolean	ED_ParseEpair (void *base, ddef_t *key, const char *s);

cvar_t	nomonsters = {"nomonsters", "0"};
cvar_t	gamecfg = {"gamecfg", "0"};
cvar_t	scratch1 = {"scratch1", "0"};
cvar_t	scratch2 = {"scratch2", "0"};
cvar_t	scratch3 = {"scratch3", "0"};
cvar_t	scratch4 = {"scratch4", "0"};
cvar_t	savedgamecfg = {"savedgamecfg", "0", true};
cvar_t	saved1 = {"saved1", "0", true};
cvar_t	saved2 = {"saved2", "0", true};
cvar_t	saved3 = {"saved3", "0", true};
cvar_t	saved4 = {"saved4", "0", true};

/*
=================
ED_ClearEdict

Sets everything to NULL
=================
*/
void ED_ClearEdict (edict_t *e)
{
	memset (&e->v, 0, progs->entityfields * 4);
	e->free = false;
}

/*
=================
ED_Alloc

Either finds a free edict, or allocates a new one.
Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
edict_t *ED_Alloc (qboolean touch_triggers)
{
	int			i,j,k;
	edict_t		*e;
	client_t* c;

	for ( i=svs.maxclients+1 ; i<sv.num_edicts ; i++)
	{
		e = EDICT_NUM(i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (e->free && ( e->freetime < 2 || sv.time - e->freetime > 0.5 ) )
		{
			ED_ClearEdict (e);
			return e;
		}
	}
	
	sv.num_edicts++;
	if (sv.num_edicts * pr_edict_size >= sv.edicts.size())
	{
		auto sv_playerindex = 0;
		for (j = 0, c = svs.clients.data(); j < svs.maxclients; j++, c++)
		{
			if (sv_player == c->edict)
			{
				sv_playerindex = j + 1;
			}
		}
		for (j = 0, k = 0; j < sv.num_edicts; j++, k += pr_edict_size)
		{
			e = (edict_t*)(sv.edicts.data() + k);
			SV_UnlinkEdict(e);
		}
		SV_ResizeEdicts(sv.edicts.size() + MAX_EDICTS * pr_edict_size);
		sv.max_edicts += MAX_EDICTS;
		sv.edicts_reallocation_sequence++;
		for (j = 0, k = 0; j < sv.num_edicts; j++, k += pr_edict_size)
		{
			e = (edict_t*)(sv.edicts.data() + k);
			SV_LinkEdict(e, touch_triggers);
		}
		for (j = 0, c = svs.clients.data(); j < svs.maxclients ; j++, c++)
		{
			c->edict = EDICT_NUM(j + 1);
		}
		if (sv_playerindex > 0)
		{
			sv_player = EDICT_NUM(sv_playerindex);
		}
	}
	
	e = EDICT_NUM(i);
	ED_ClearEdict (e);

	return e;
}

/*
=================
ED_Free

Marks the edict as free
FIXME: walk all entities and NULL out references to this entity
=================
*/
void ED_Free (edict_t *ed)
{
	SV_UnlinkEdict (ed);		// unlink from world bsp

	ed->free = true;
	ed->v.model = 0;
	ed->v.takedamage = 0;
	ed->v.modelindex = 0;
	ed->v.colormap = 0;
	ed->v.skin = 0;
	ed->v.frame = 0;
	VectorCopy (vec3_origin, ed->v.origin);
	VectorCopy (vec3_origin, ed->v.angles);
	ed->v.nextthink = -1;
	ed->v.solid = 0;
	
	ed->freetime = sv.time;
}

//===========================================================================

/*
============
ED_GlobalAtOfs
============
*/
ddef_t *ED_GlobalAtOfs (int ofs)
{
	ddef_t		*def;
	int			i;
	
	for (i=0 ; i<progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}

/*
============
ED_FieldAtOfs
============
*/
ddef_t *ED_FieldAtOfs (int ofs)
{
	ddef_t		*def;
	int			i;
	
	for (i=0 ; i<progs->numfielddefs ; i++)
	{
		def = &pr_fielddefs[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}

/*
============
ED_FindField
============
*/
ddef_t *ED_FindField (const char *name)
{
	auto entry = pr_fielddefs_index.find(name);
	if (entry != pr_fielddefs_index.end())
	{
		return &pr_fielddefs[entry->second];
	}
	return NULL;
}


/*
============
ED_FindGlobal
============
*/
ddef_t *ED_FindGlobal (const char *name)
{
	ddef_t		*def;
	int			i;
	
	for (i=0 ; i<progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs[i];
		if (!strcmp(pr_strings + def->s_name,name) )
			return def;
	}
	return NULL;
}


/*
============
ED_FindFunction
============
*/
dfunction_t *ED_FindFunction (const char *name)
{
	dfunction_t		*func;
	int				i;
	
	for (i=0 ; i<progs->numfunctions ; i++)
	{
		func = &pr_functions[i];
		if (!strcmp(pr_strings + func->s_name,name) )
			return func;
	}
	return NULL;
}


eval_t *GetEdictFieldValue(edict_t *ed, const char *field)
{
	ddef_t			*def = NULL;

	def = ED_FindField (field);

	if (!def)
		return NULL;

	return (eval_t *)((char *)&ed->v + def->ofs*4);
}


/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
const char *PR_ValueString (etype_t type, eval_t *val)
{
	static std::string	line;
	static char fline[16];
	ddef_t		*def;
	dfunction_t	*f;
	
	type = (etype_t)((int)type & ~DEF_SAVEGLOBAL);

	switch (type)
	{
	case ev_string:
		line = pr_strings + val->string;
		break;
	case ev_entity:
		line = std::string("entity ") + std::to_string(NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));
		break;
	case ev_function:
		f = pr_functions + val->function;
		line = std::string(pr_strings + f->s_name) + "()";
		break;
	case ev_field:
		def = ED_FieldAtOfs ( val->_int );
		line = std::string(".") + std::string(pr_strings + def->s_name);
		break;
	case ev_void:
		line = "void";
		break;
	case ev_float:
		sprintf (fline, "%5.1f", val->_float);
		line = fline;
		break;
	case ev_vector:
		sprintf (fline, "%5.1f", val->vector[0]);
		line = fline;
		sprintf (fline, " %5.1f", val->vector[1]);
		line += fline;
		sprintf (fline, " %5.1f", val->vector[2]);
		line += fline;
		break;
	case ev_pointer:
		line = "pointer";
		break;
	default:
		line = std::string("bad type ") + std::to_string(type);
		break;
	}
	
	return line.c_str();
}

/*
============
PR_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
const char *PR_UglyValueString (etype_t type, eval_t *val)
{
	static std::string	line;
	ddef_t		*def;
	dfunction_t	*f;
	
	type = (etype_t)((int)type & ~DEF_SAVEGLOBAL);

	switch (type)
	{
	case ev_string:
		line = pr_strings + val->string;
		break;
	case ev_entity:	
		line = std::to_string(NUM_FOR_EDICT(PROG_TO_EDICT(val->edict)));
		break;
	case ev_function:
		f = pr_functions + val->function;
		line = pr_strings + f->s_name;
		break;
	case ev_field:
		def = ED_FieldAtOfs ( val->_int );
		line = pr_strings + def->s_name;
		break;
	case ev_void:
		line = "void";
		break;
	case ev_float:
		line = std::to_string(val->_float);
		break;
	case ev_vector:
		line = std::to_string(val->vector[0]) + " " + std::to_string(val->vector[1]) + " " + std::to_string(val->vector[2]);
		break;
	default:
		line = std::string("bad type ") + std::to_string(type);
		break;
	}
	
	return line.c_str();
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
const char *PR_GlobalString (int ofs)
{
	ddef_t	*def;
	void	*val;
	static std::string line;
	
	val = (void *)&pr_globals[ofs];
	def = ED_GlobalAtOfs(ofs);
	if (!def)
		line = std::to_string(ofs) + "(???)";
	else
	{
		auto s = PR_ValueString ((etype_t)def->type, (eval_t*)val);
		line = std::to_string(ofs) + "(" + (pr_strings + def->s_name) + ")" + s;
	}
	
	while (line.length() < 20)
		line += ' ';
	line += ' ';
		
	return line.c_str();
}

const char *PR_GlobalStringNoContents (int ofs)
{
	ddef_t	*def;
	static std::string line;
	
	def = ED_GlobalAtOfs(ofs);
	if (!def)
		line = std::to_string(ofs) + "(???)";
	else
		line = std::to_string(ofs) + "(" + (pr_strings + def->s_name) + ")";
	
	while (line.length() < 20)
		line += ' ';
	line += ' ';
		
	return line.c_str();
}


/*
=============
ED_Print

For debugging
=============
*/
void ED_Print (edict_t *ed)
{
	int		l;
	ddef_t	*d;
	int		*v;
	int		i, j;
	char	*name;
	int		type;

	if (ed->free)
	{
		Con_Printf ("FREE\n");
		return;
	}

	Con_Printf("\nEDICT %i:\n", NUM_FOR_EDICT(ed));
	for (i=1 ; i<progs->numfielddefs ; i++)
	{
		d = &pr_fielddefs[i];
		name = pr_strings + d->s_name;
		if (name[strlen(name)-2] == '_')
			continue;	// skip _x, _y, _z vars
			
		v = (int *)((char *)&ed->v + d->ofs*4);

	// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;
		
		for (j=0 ; j<type_size[type] ; j++)
			if (v[j])
				break;
		if (j == type_size[type])
			continue;
	
		Con_Printf ("%s",name);
		l = strlen (name);
		while (l++ < 15)
			Con_Printf (" ");

		Con_Printf ("%s\n", PR_ValueString((etype_t)d->type, (eval_t *)v));		
	}
}

/*
=============
ED_Write

For savegames
=============
*/
void ED_Write (int f, edict_t *ed)
{
	ddef_t	*d;
	int		*v;
	int		i, j;
	char	*name;
	int		type;

	std::string to_write = "{\n";

	if (ed->free)
	{
		to_write += "}\n";
		Sys_FileWrite(f, (void*)to_write.c_str(), (int)to_write.length());
		return;
	}
	
	for (i=1 ; i<progs->numfielddefs ; i++)
	{
		d = &pr_fielddefs[i];
		name = pr_strings + d->s_name;
		if (name[strlen(name)-2] == '_')
			continue;	// skip _x, _y, _z vars
			
		v = (int *)((char *)&ed->v + d->ofs*4);

	// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;
		for (j=0 ; j<type_size[type] ; j++)
			if (v[j])
				break;
		if (j == type_size[type])
			continue;
	
		to_write += std::string("\"") + name + "\" ";
		to_write += std::string("\"") + PR_UglyValueString((etype_t)d->type, (eval_t *)v) + "\"\n";
	}

	to_write += "}\n";
	Sys_FileWrite(f, (void*)to_write.c_str(), (int)to_write.length());
}

void ED_PrintNum (int ent)
{
	ED_Print (EDICT_NUM(ent));
}

/*
=============
ED_PrintEdicts

For debugging, prints all the entities in the current server
=============
*/
void ED_PrintEdicts (void)
{
	int		i;
	
	Con_Printf ("%i entities\n", sv.num_edicts);
	for (i=0 ; i<sv.num_edicts ; i++)
		ED_PrintNum (i);
}

/*
=============
ED_PrintEdict_f

For debugging, prints a single edicy
=============
*/
void ED_PrintEdict_f (void)
{
	int		i;
	
	i = Q_atoi (Cmd_Argv(1));
	if (i >= sv.num_edicts)
	{
		Con_Printf("Bad edict number\n");
		return;
	}
	ED_PrintNum (i);
}

/*
=============
ED_Count

For debugging
=============
*/
void ED_Count (void)
{
	int		i;
	edict_t	*ent;
	int		active, models, solid, step;

	active = models = solid = step = 0;
	for (i=0 ; i<sv.num_edicts ; i++)
	{
		ent = EDICT_NUM(i);
		if (ent->free)
			continue;
		active++;
		if (ent->v.solid)
			solid++;
		if (ent->v.model)
			models++;
		if (ent->v.movetype == MOVETYPE_STEP)
			step++;
	}

	Con_Printf ("num_edicts:%3i\n", sv.num_edicts);
	Con_Printf ("active    :%3i\n", active);
	Con_Printf ("view      :%3i\n", models);
	Con_Printf ("touch     :%3i\n", solid);
	Con_Printf ("step      :%3i\n", step);

}

/*
==============================================================================

					ARCHIVING GLOBALS

FIXME: need to tag constants, doesn't really work
==============================================================================
*/

/*
=============
ED_WriteGlobals
=============
*/
void ED_WriteGlobals (int f)
{
	ddef_t		*def;
	int			i;
	char		*name;
	int			type;

	std::string to_write = "{\n";
	for (i=0 ; i<progs->numglobaldefs ; i++)
	{
		def = &pr_globaldefs[i];
		type = def->type;
		if ( !(def->type & DEF_SAVEGLOBAL) )
			continue;
		type &= ~DEF_SAVEGLOBAL;

		if (type != ev_string
		&& type != ev_float
		&& type != ev_entity)
			continue;

		name = pr_strings + def->s_name;		
		to_write += std::string("\"") + name + "\" ";
		to_write += std::string("\"") + PR_UglyValueString((etype_t)type, (eval_t *)&pr_globals[def->ofs]) + "\"\n";
	}
	to_write += "}\n";
	Sys_FileWrite (f, (void*)to_write.c_str(), (int)to_write.length());
}

/*
=============
ED_ParseGlobals
=============
*/
void ED_ParseGlobals (const char *data)
{
	std::string keyname;
	ddef_t	*key;

	while (1)
	{	
	// parse key
		data = COM_Parse (data);
		if (com_token[0] == '}')
			break;
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		keyname = com_token;

	// parse value	
		data = COM_Parse (data);
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			Sys_Error ("ED_ParseEntity: closing brace without data");

		key = ED_FindGlobal (keyname.c_str());
		if (!key)
		{
			Con_Printf ("'%s' is not a global\n", keyname.c_str());
			continue;
		}

		if (!ED_ParseEpair ((void *)pr_globals, key, com_token.c_str()))
			Host_Error ("ED_ParseGlobals: parse error");
	}
}

//============================================================================

std::vector<char> pr_string_block;
int pr_string_block_used;

/*
=============
ED_NewString
=============
*/
char *ED_NewString (const char *string)
{
	char	*new_string, *new_p;
	int		i,l;
	
	l = (int)strlen(string) + 1;
	i = ED_NewString(l);
	new_string = pr_string_block.data() + i;
	new_p = new_string;

	for (i=0 ; i< l ; i++)
	{
		if (string[i] == '\\' && i < l-1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}
	
	return new_string;
}

int ED_NewString(int size)
{
	size = (size + 15) & ~15;
	if (pr_string_block_used + size > pr_string_block.size())
	{
		int additional = size;
		if (progs != nullptr && progs->numstrings > size)
		{
			additional = (progs->numstrings + 15) & ~15;
		}
		pr_string_block.resize(pr_string_block.size() + additional);
		pr_strings = pr_string_block.data();
		if (pr_fielddefs != nullptr)
		{
			pr_fielddefs_index.clear();
			for (auto i = 0; i < progs->numfielddefs; i++)
			{
				pr_fielddefs_index.insert({ pr_strings + pr_fielddefs[i].s_name, i });
			}
		}
	}
	auto offset = pr_string_block_used;
	pr_string_block_used += size;
	return offset;
}

void PR_LoadStrings(char* source, int size)
{
	auto to_use = (size + 15) & ~15;
	auto to_allocate = to_use;
	if (pr_string_block_used + to_allocate > pr_string_block.size())
	{
		pr_string_block.resize(pr_string_block.size() + to_allocate);
		pr_strings = pr_string_block.data();
		if (pr_fielddefs != nullptr)
		{
			pr_fielddefs_index.clear();
			for (auto i = 0; i < progs->numfielddefs; i++)
			{
				pr_fielddefs_index.insert({ pr_strings + pr_fielddefs[i].s_name, i });
			}
		}
	}
	pr_string_block_used += to_use;
	Q_memcpy(pr_strings, source, size);
}

/*
=============
ED_ParseEval

Can parse either fields or globals
returns false if error
=============
*/
qboolean	ED_ParseEpair (void *base, ddef_t *key, const char *s)
{
	int		i;
	std::vector<char>	string(128);
	ddef_t	*def;
	char	*v, *w;
	void	*d;
	dfunction_t	*func;
	size_t len;
	char* end;

	d = (void *)((int *)base + key->ofs);
	
	switch (key->type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
		*(string_t *)d = ED_NewString (s) - pr_strings;
		break;
		
	case ev_float:
		*(float *)d = atof (s);
		break;
		
	case ev_vector:
		len = strlen(s);
		if (string.size() < len + 1)
		{
			string.resize(len + 1);
		}
		memcpy (string.data(), s, len);
		v = string.data();
		w = string.data();
		end = string.data() + len;
		for (i=0 ; i<3 && (w <= end) ; i++)
		{
			while (*v && *v != ' ')
				v++;
			*v = 0;
			((float *)d)[i] = atof (w);
			w = v = v+1;
		}
		if (i < 3)
		{
			Con_Printf ("Vector %s missing %i components\n", s, 3 - i);
			for (; i < 3; i++)
			{
				((float *)d)[i] = 0;
			}
		}
		break;
		
	case ev_entity:
		*(int *)d = EDICT_TO_PROG(EDICT_NUM(atoi (s)));
		break;
		
	case ev_field:
		def = ED_FindField (s);
		if (!def)
		{
			Con_Printf ("Can't find field %s\n", s);
			return false;
		}
		*(int *)d = G_INT(def->ofs);
		break;
	
	case ev_function:
		func = ED_FindFunction (s);
		if (!func)
		{
			Con_Printf ("Can't find function %s\n", s);
			return false;
		}
		*(func_t *)d = func - pr_functions;
		break;
		
	default:
		break;
	}
	return true;
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
const char *ED_ParseEdict (const char *data, edict_t *ent)
{
	ddef_t		*key;
	qboolean	anglehack;
	qboolean	init;
	std::string		keyname;

	init = false;

// clear it
	if (ent != (edict_t*)sv.edicts.data())	// hack
		memset (&ent->v, 0, progs->entityfields * 4);

// go through all the dictionary pairs
	while (1)
	{	
	// parse key
		data = COM_Parse (data);
		if (com_token[0] == '}')
			break;
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");
		
// anglehack is to allow QuakeEd to write single scalar angles
// and allow them to be turned into vectors. (FIXME...)
if (com_token == "angle")
{
	com_token += "s";
	anglehack = true;
}
else
	anglehack = false;

// FIXME: change light to _light to get rid of this hack
if (com_token == "light")
	com_token += "_lev";	// hack for single light def

		keyname = com_token;

		// another hack to fix heynames with trailing spaces
		while (keyname.length() > 0 && keyname.back() == ' ')
		{
			keyname.pop_back();
		}

	// parse value	
		data = COM_Parse (data);
		if (!data)
			Sys_Error ("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			Sys_Error ("ED_ParseEntity: closing brace without data");

		init = true;	

// keynames with a leading underscore are used for utility comments,
// and are immediately discarded by quake
		if (keyname[0] == '_')
			continue;
		
		key = ED_FindField (keyname.c_str());
		if (!key)
		{
			Con_Printf ("'%s' is not a field\n", keyname.c_str());
			continue;
		}

if (anglehack)
{
com_token = std::string("0 ") + com_token + " 0";
}

		if (!ED_ParseEpair ((void *)&ent->v, key, com_token.c_str()))
			Host_Error ("ED_ParseEdict: parse error");
	}

	if (!init)
		ent->free = true;

	return data;
}


/*
================
ED_LoadFromFile

The entities are directly placed in the array, rather than allocated with
ED_Alloc, because otherwise an error loading the map would have entity
number references out of order.

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.

Used for both fresh maps and savegame loads.  A fresh map would also need
to call ED_CallSpawnFunctions () to let the objects initialize themselves.
================
*/
void ED_LoadFromFile (const char *data)
{	
	edict_t		*ent;
	int			inhibit;
	dfunction_t	*func;
	
	ent = NULL;
	inhibit = 0;
	pr_global_struct->time = sv.time;
	
// parse ents
	while (1)
	{
// parse the opening brace	
		data = COM_Parse (data);
		if (!data)
			break;
		if (com_token[0] != '{')
			Sys_Error ("ED_LoadFromFile: found %s when expecting {",com_token.c_str());

		if (!ent)
			ent = EDICT_NUM(0);
		else
			ent = ED_Alloc (true);
		data = ED_ParseEdict (data, ent);

// remove things from different skill levels or deathmatch
		if (deathmatch.value)
		{
			if (((int)ent->v.spawnflags & SPAWNFLAG_NOT_DEATHMATCH))
			{
				ED_Free (ent);	
				inhibit++;
				continue;
			}
		}
		else if ((current_skill == 0 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_EASY))
				|| (current_skill == 1 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_MEDIUM))
				|| (current_skill >= 2 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_HARD)) )
		{
			ED_Free (ent);	
			inhibit++;
			continue;
		}

//
// immediately call spawn function
//
		if (!ent->v.classname)
		{
			Con_Printf ("No classname for:\n");
			ED_Print (ent);
			ED_Free (ent);
			continue;
		}

	// look for the spawn function
		func = ED_FindFunction ( pr_strings + ent->v.classname );

		if (!func)
		{
			Con_Printf ("No spawn function for:\n");
			ED_Print (ent);
			ED_Free (ent);
			continue;
		}

		pr_global_struct->self = EDICT_TO_PROG(ent);
		PR_ExecuteProgram (func - pr_functions);
	}	

	Con_DPrintf ("%i entities inhibited\n", inhibit);
}


/*
===============
PR_LoadProgs
===============
*/
void PR_LoadProgs (void)
{
	int		i;

// cleanup cached entity field offsets:
	pr_items2_ofs = -1;
	pr_gravity_ofs = -1;
	pr_alpha_ofs = -1;
	pr_scale_ofs = -1;

	CRC_Init (&pr_crc);

	static std::vector<byte> progs_block;
	progs = (dprograms_t *)COM_LoadFile ("progs.dat", progs_block);
	if (!progs)
		Sys_Error ("PR_LoadProgs: couldn't load progs.dat");
	Con_DPrintf ("Programs occupy %iK.\n", com_filesize/1024);

	for (i=0 ; i<com_filesize ; i++)
		CRC_ProcessByte (&pr_crc, ((byte *)progs)[i]);

// byte swap the header
	for (i=0 ; i<sizeof(*progs)/4 ; i++)
		((int *)progs)[i] = LittleLong ( ((int *)progs)[i] );		

	if (progs->version != PROG_VERSION)
		Sys_Error ("progs.dat has wrong version number (%i should be %i)", progs->version, PROG_VERSION);
	if (progs->crc != PROGHEADER_CRC)
		Sys_Error ("progs.dat system vars have been modified, progdefs.h is out of date");

	pr_functions = (dfunction_t *)((byte *)progs + progs->ofs_functions);
	PR_LoadStrings((char*)progs + progs->ofs_strings, progs->numstrings);
	pr_globaldefs = (ddef_t *)((byte *)progs + progs->ofs_globaldefs);
	pr_fielddefs = (ddef_t *)((byte *)progs + progs->ofs_fielddefs);
	pr_statements = (dstatement_t *)((byte *)progs + progs->ofs_statements);

	pr_global_struct = (globalvars_t *)((byte *)progs + progs->ofs_globals);
	pr_globals = (float *)pr_global_struct;
	
	pr_edict_size = progs->entityfields * 4 + sizeof (edict_t) - sizeof(entvars_t);
	
// byte swap the lumps
	for (i=0 ; i<progs->numstatements ; i++)
	{
		pr_statements[i].op = LittleShort(pr_statements[i].op);
		pr_statements[i].a = LittleShort(pr_statements[i].a);
		pr_statements[i].b = LittleShort(pr_statements[i].b);
		pr_statements[i].c = LittleShort(pr_statements[i].c);
	}

	for (i=0 ; i<progs->numfunctions; i++)
	{
	pr_functions[i].first_statement = LittleLong (pr_functions[i].first_statement);
	pr_functions[i].parm_start = LittleLong (pr_functions[i].parm_start);
	pr_functions[i].s_name = LittleLong (pr_functions[i].s_name);
	pr_functions[i].s_file = LittleLong (pr_functions[i].s_file);
	pr_functions[i].numparms = LittleLong (pr_functions[i].numparms);
	pr_functions[i].locals = LittleLong (pr_functions[i].locals);
	}	

	for (i=0 ; i<progs->numglobaldefs ; i++)
	{
		pr_globaldefs[i].type = LittleShort (pr_globaldefs[i].type);
		pr_globaldefs[i].ofs = LittleShort (pr_globaldefs[i].ofs);
		pr_globaldefs[i].s_name = LittleLong (pr_globaldefs[i].s_name);
	}

	pr_fielddefs_index.clear();
	for (i=0 ; i<progs->numfielddefs ; i++)
	{
		pr_fielddefs[i].type = LittleShort (pr_fielddefs[i].type);
		if (pr_fielddefs[i].type & DEF_SAVEGLOBAL)
			Sys_Error ("PR_LoadProgs: pr_fielddefs[i].type & DEF_SAVEGLOBAL");
		pr_fielddefs[i].ofs = LittleShort (pr_fielddefs[i].ofs);
		pr_fielddefs[i].s_name = LittleLong (pr_fielddefs[i].s_name);
		const char* name = pr_strings + pr_fielddefs[i].s_name;
		pr_fielddefs_index.insert({ name, i });
		if (Q_strcmp(name, "items2") == 0)
		{
			pr_items2_ofs = pr_fielddefs[i].ofs;
		}
		else if (Q_strcmp(name, "gravity") == 0)
		{
			pr_gravity_ofs = pr_fielddefs[i].ofs;
		}
		else if (Q_strcmp(name, "alpha") == 0)
		{
			pr_alpha_ofs = pr_fielddefs[i].ofs;
		}
		else if (Q_strcmp(name, "scale") == 0)
		{
			pr_scale_ofs = pr_fielddefs[i].ofs;
		}
	}

	for (i=0 ; i<progs->numglobals ; i++)
		((int *)pr_globals)[i] = LittleLong (((int *)pr_globals)[i]);
}


/*
===============
PR_Init
===============
*/
void PR_Init (void)
{
	Cmd_AddCommand ("edict", ED_PrintEdict_f);
	Cmd_AddCommand ("edicts", ED_PrintEdicts);
	Cmd_AddCommand ("edictcount", ED_Count);
	Cmd_AddCommand ("profile", PR_Profile_f);
	Cvar_RegisterVariable (&nomonsters);
	Cvar_RegisterVariable (&gamecfg);
	Cvar_RegisterVariable (&scratch1);
	Cvar_RegisterVariable (&scratch2);
	Cvar_RegisterVariable (&scratch3);
	Cvar_RegisterVariable (&scratch4);
	Cvar_RegisterVariable (&savedgamecfg);
	Cvar_RegisterVariable (&saved1);
	Cvar_RegisterVariable (&saved2);
	Cvar_RegisterVariable (&saved3);
	Cvar_RegisterVariable (&saved4);
}



edict_t *EDICT_NUM(int n)
{
	if (n < 0 || n >= sv.max_edicts)
		Sys_Error ("EDICT_NUM: bad number %i", n);
	return (edict_t *)((byte *)sv.edicts.data()+ (n)*pr_edict_size);
}

int NUM_FOR_EDICT(edict_t *e)
{
	intptr_t		b;
	
	b = (byte *)e - (byte *)sv.edicts.data();
	b = b / pr_edict_size;
	
	if (b < 0 || b >= sv.num_edicts)
		Sys_Error ("NUM_FOR_EDICT: bad pointer");
	return (int)b;
}
