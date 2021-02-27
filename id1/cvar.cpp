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
// cvar.c -- dynamic variable tracking

#include "quakedef.h"

cvar_t	*cvar_vars;

std::unordered_map<std::string, cvar_t*> cvar_index;

/*
============
Cvar_FindVar
============
*/
cvar_t *Cvar_FindVar (const char *var_name)
{
	static std::string key;
	key = var_name;
    auto entry = cvar_index.find(key);
    if (entry != cvar_index.end())
    {
        return entry->second;
    }
    return nullptr;
}

/*
============
Cvar_VariableValue
============
*/
float	Cvar_VariableValue (const char *var_name)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return Q_atof (var->string.c_str());
}


/*
============
Cvar_VariableString
============
*/
const char *Cvar_VariableString (const char *var_name)
{
	cvar_t *var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return "";
	return (char*)var->string.c_str();
}


/*
============
Cvar_CompleteVariable
============
*/
const char *Cvar_CompleteVariable (char *partial)
{
	cvar_t		*cvar;
	int			len;
	
	len = Q_strlen(partial);
	
	if (!len)
		return NULL;
		
// check functions
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!Q_strncmp (partial,cvar->name, len))
			return cvar->name;

	return NULL;
}


/*
============
Cvar_Set
============
*/
void Cvar_Set (const char *var_name, const char *value)
{
	cvar_t	*var;
	qboolean changed;
	
	var = Cvar_FindVar (var_name);
	if (!var)
	{	// there is an error in C code if this happens
		Con_Printf ("Cvar_Set: variable %s not found\n", var_name);
		return;
	}

	changed = Q_strcmp(var->string.c_str(), value);

    var->string = value;
	var->value = Q_atof (value);
	if (var->server && changed)
	{
		if (sv.active)
			SV_BroadcastPrintf ("\"%s\" changed to \"%s\"\n", var->name, value);
	}
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue (const char *var_name, float value)
{
	char	val[32];
	
	sprintf (val, "%f",value);
	Cvar_Set (var_name, val);
}


/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
void Cvar_RegisterVariable (cvar_t *variable)
{
// first check to see if it has allready been defined
	if (Cvar_FindVar (variable->name))
	{
		Con_Printf ("Can't register variable %s, allready defined\n", variable->name);
		return;
	}
	
// check for overlap with a command
	if (Cmd_Exists (variable->name))
	{
		Con_Printf ("Cvar_RegisterVariable: %s is a command\n", variable->name);
		return;
	}
		
    variable->string = variable->default_value;
	variable->value = Q_atof (variable->default_value);
	
// link the variable in
	variable->next = cvar_vars;
	cvar_vars = variable;
    cvar_index.insert({ std::string(variable->name), variable });
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean	Cvar_Command (void)
{
	cvar_t			*v;

// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v)
		return false;
		
// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		Con_Printf ("\"%s\" is \"%s\"\n", v->name, v->string.c_str());
		return true;
	}

	Cvar_Set (v->name, Cmd_Argv(1));
	return true;
}


/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (int f)
{
	cvar_t	*var;
	
	for (var = cvar_vars ; var ; var = var->next)
		if (var->archive)
        {
            auto to_write = std::string(var->name) + " \"" + var->string + "\"\n";
            Sys_FileWrite(f, (void*)to_write.c_str(), (int)to_write.length());
        }
}

