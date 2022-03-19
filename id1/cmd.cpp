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
// cmd.c -- Quake script command processing module

#include "quakedef.h"

void Cmd_ForwardToServer (void);

struct cmdalias_t
{
	cmdalias_t	*next;
    std::string name;
	std::string value;
};

cmdalias_t	*cmd_alias;

int trashtest;
int *trashspot;

qboolean	cmd_wait;

//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
void Cmd_Wait_f (void)
{
	cmd_wait = true;
}

/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

std::string cmd_text;

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
}


/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (const char *text)
{
    cmd_text += text;
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
============
*/
void Cbuf_InsertText (const char *text)
{
    cmd_text = std::string(text) + cmd_text;
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	int		i;
	const char	*text;
    std::string line;
	int		quotes;
	int		comment;
	int		commentpos;

	while (cmd_text.length() > 0)
	{
// find a \n or ; line break
        text = cmd_text.c_str();

		quotes = 0;
		comment = 0;
		commentpos = -1;
		for (i=0 ; i< cmd_text.length() ; i++)
		{
			if (text[i] == '"')
				quotes++;
			if ( !(quotes&1) )	// don't break if inside a quoted string
			{
				if (text[i] == '/')
				{
					comment++;
					if (comment == 2 && commentpos < 0)
						commentpos = i - 1;
				}
				else
				{
					comment = 0;
					if (text[i] == ';' && commentpos < 0)
						break;
				}
			}
			if (text[i] == '\n')
				break;
		}
			
        line = cmd_text.substr(0, (commentpos < 0 ? i : commentpos));
		
// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec, alias) can insert data at the
// beginning of the text buffer

        cmd_text.erase(0, i + 1);

// execute the command line
		Cmd_ExecuteString ((char*)line.c_str(), src_command);
		
		if (cmd_wait)
		{	// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait = false;
			break;
		}
	}
}

/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/

/*
===============
Cmd_StuffCmds_f

Adds command line parameters as script statements
Commands lead with a +, and continue until a - or another +
quake +prog jctest.qp +cmd amlev1
quake -nosound +cmd amlev1
===============
*/
void Cmd_StuffCmds_f (void)
{
	int		i, j;
	int		s;
	char	c;
		
	if (Cmd_Argc () != 1)
	{
		Con_Printf ("stuffcmds : execute command line parameters\n");
		return;
	}

// build the combined string to parse from
	s = 0;
	for (i=1 ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP nulls out -NXHost
		s += Q_strlen (com_argv[i]) + 1;
	}
	if (!s)
		return;
		
    std::string text;
	for (i=1 ; i<com_argc ; i++)
	{
		if (!com_argv[i])
			continue;		// NEXTSTEP nulls out -NXHost
		text += com_argv[i];
		if (i != com_argc-1)
			text += " ";
	}
	
// pull out the commands
    std::string build;
	
	for (i=0 ; i<s-1 ; i++)
	{
		if (text[i] == '+')
		{
			i++;

			for (j=i ; (text[j] != '+') && (text[j] != '-') && (text[j] != 0) ; j++)
				;

			c = text[j];
			text[j] = 0;
			
			build += text.c_str()+i;
			build += "\n";
			text[j] = c;
			i = j-1;
		}
	}
	
	if (build[0])
		Cbuf_InsertText (build.c_str());
}


/*
===============
Cmd_Exec_f
===============
*/
void Cmd_Exec_f (void)
{
	char	*f;

	if (Cmd_Argc () != 2)
	{
		Con_Printf ("exec <filename> : execute a script file\n");
		return;
	}

    std::vector<byte> contents;
    f = (char *)COM_LoadFile (Cmd_Argv(1), contents);
	if (!f)
	{
		Con_Printf ("couldn't exec %s\n",Cmd_Argv(1));
		return;
	}
	Con_Printf ("execing %s\n",Cmd_Argv(1));
	
	Cbuf_InsertText (f);
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void Cmd_Echo_f (void)
{
	int		i;
	
	for (i=1 ; i<Cmd_Argc() ; i++)
		Con_Printf ("%s ",Cmd_Argv(i));
	Con_Printf ("\n");
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/

void Cmd_Alias_f (void)
{
	cmdalias_t	*a;
	int			i, c;
	const char	*s;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Current alias commands:\n");
		for (a = cmd_alias ; a ; a=a->next)
			Con_Printf ("%s : %s\n", a->name.c_str(), a->value.c_str());
		return;
	}

	s = Cmd_Argv(1);

	// if the alias allready exists, reuse it
	for (a = cmd_alias ; a ; a=a->next)
	{
		if (!strcmp(s, a->name.c_str()))
		{
            a->value.clear();
			break;
		}
	}

	if (!a)
	{
		a = new cmdalias_t { cmd_alias };
		cmd_alias = a;
        a->name = s;
	}

// copy the rest of the command line
	c = Cmd_Argc();
	for (i=2 ; i< c ; i++)
	{
		a->value += Cmd_Argv(i);
		if (i != c)
			a->value += " ";
	}
	a->value += "\n";
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/



static	std::vector<std::string> cmd_argv;
static	const char	*cmd_null_string = "";
static	const char	*cmd_args = NULL;

cmd_source_t	cmd_source;


std::unordered_map<std::string, xcommand_t> cmd_functions;		// possible commands to execute

/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
//
// register our commands
//
	Cmd_AddCommand ("stuffcmds",Cmd_StuffCmds_f);
	Cmd_AddCommand ("exec",Cmd_Exec_f);
	Cmd_AddCommand ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("alias",Cmd_Alias_f);
	Cmd_AddCommand ("cmd", Cmd_ForwardToServer);
	Cmd_AddCommand ("wait", Cmd_Wait_f);
}

/*
============
Cmd_Argc
============
*/
int		Cmd_Argc (void)
{
	return cmd_argv.size();
}

/*
============
Cmd_Argv
============
*/
const char	*Cmd_Argv (int arg)
{
	if ( (unsigned)arg >= cmd_argv.size() )
		return cmd_null_string;
	return (char*)cmd_argv[arg].c_str();
}

/*
============
Cmd_Args
============
*/
const char	*Cmd_Args (void)
{
	return cmd_args;
}


/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
============
*/
void Cmd_TokenizeString (const char *text)
{
// clear the args from the last string
	cmd_argv.clear();
	cmd_args = NULL;
	
	while (1)
	{
// skip whitespace up to a /n
		while (*text && *text <= ' ' && *text != '\n')
		{
			text++;
		}
		
		if (*text == '\n')
		{	// a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			return;
	
		if (cmd_argv.size() == 1)
			 cmd_args = text;
			
		text = COM_Parse (text);
		if (!text)
			return;

        cmd_argv.emplace_back(com_token.c_str());
	}
	
}


/*
============
Cmd_AddCommand
============
*/
void	Cmd_AddCommand (const char *cmd_name, xcommand_t function)
{
// fail if the command is a variable name
	if (Cvar_VariableString(cmd_name)[0])
	{
		Con_Printf ("Cmd_AddCommand: %s already defined as a var\n", cmd_name);
		return;
	}
	
// fail if the command already exists
    std::string name(cmd_name);
    auto entry = cmd_functions.find(name);
    if (entry != cmd_functions.end())
	{
			Con_Printf ("Cmd_AddCommand: %s already defined\n", cmd_name);
			return;
	}

    cmd_functions.insert({ name, function });
}

/*
============
Cmd_Exists
============
*/
qboolean	Cmd_Exists (const char *cmd_name)
{
	std::string name(cmd_name);
    auto entry = cmd_functions.find(name);
    if (entry != cmd_functions.end())
			return true;

	return false;
}



/*
============
Cmd_CompleteCommand
============
*/
char *Cmd_CompleteCommand (char *partial)
{
	int				len;
	
	len = Q_strlen(partial);
	
	if (!len)
		return NULL;
		
// check functions
    for (auto& entry : cmd_functions)
		if (!Q_strncmp (partial,entry.first.c_str(), len))
			return (char*)entry.first.c_str();

	return NULL;
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void	Cmd_ExecuteString (const char *text, cmd_source_t src)
{	
	cmdalias_t		*a;

	cmd_source = src;
	Cmd_TokenizeString (text);
			
// execute the command line
	if (!Cmd_Argc())
		return;		// no tokens

// check functions
    std::string name(cmd_argv[0]);
    auto entry = cmd_functions.find(name);
    if (entry != cmd_functions.end())
    {
        entry->second();
        return;
    }

// check alias
	for (a=cmd_alias ; a ; a=a->next)
	{
		if (!Q_strcasecmp (cmd_argv[0].c_str(), a->name.c_str()))
		{
			Cbuf_InsertText (a->value.c_str());
			return;
		}
	}
	
// check cvars
	if (!Cvar_Command ())
		Con_Printf ("Unknown command \"%s\"\n", Cmd_Argv(0));
	
}


/*
===================
Cmd_ForwardToServer

Sends the entire command line over to the server
===================
*/
void Cmd_ForwardToServer (void)
{
	if (cls.state != ca_connected)
	{
		Con_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}
	
	if (cls.demoplayback)
		return;		// not really connected

	MSG_WriteByte (&cls.message, clc_stringcmd);
	if (Q_strcasecmp(Cmd_Argv(0), "cmd") != 0)
	{
		SZ_Print (&cls.message, Cmd_Argv(0));
		SZ_Print (&cls.message, " ");
	}
	if (Cmd_Argc() > 1)
		SZ_Print (&cls.message, Cmd_Args());
	else
		SZ_Print (&cls.message, "\n");
}
