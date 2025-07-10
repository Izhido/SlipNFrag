#include "quakedef.h"
#include <cfloat>

#define PROTOCOL_FITZQUAKE	666
#define PROTOCOL_RMQ		999

#define U_EXTEND1		(1<<15)
#define U_ALPHA_FR		(1<<16)
#define U_FRAME2		(1<<17)
#define U_MODEL2		(1<<18)
#define U_LERPFINISH	(1<<19)
#define U_SCALE_FR		(1<<20)
#define U_EXTEND2		(1<<23)

#define U_TRANS			(1<<15)

#define SU_EXTEND1		(1<<15)
#define SU_WEAPON2		(1<<16)
#define SU_ARMOR2		(1<<17)
#define SU_AMMO2		(1<<18)
#define SU_SHELLS2		(1<<19)
#define SU_NAILS2		(1<<20)
#define SU_ROCKETS2		(1<<21)
#define SU_CELLS2		(1<<22)
#define SU_EXTEND2		(1<<23)
#define SU_WEAPONFRAME2	(1<<24)
#define SU_WEAPONALPHA	(1<<25)

#define PRFL_SHORTANGLE		(1 << 1)
#define PRFL_FLOATANGLE		(1 << 2)
#define PRFL_24BITCOORD		(1 << 3)
#define PRFL_FLOATCOORD		(1 << 4)
#define PRFL_EDICTSCALE		(1 << 5)
#define PRFL_INT32COORD		(1 << 7)

#define	SND_LARGEENTITY	(1<<3)
#define	SND_LARGESOUND	(1<<4)

#define B_LARGEMODEL	(1<<0)
#define B_LARGEFRAME	(1<<1)
#define B_ALPHA			(1<<2)
#define B_SCALE			(1<<3)

#define	svc_skybox				37
#define svc_bf					40
#define svc_fog					41
#define svc_spawnbaseline2		42
#define svc_spawnstatic2		43
#define	svc_spawnstaticsound2	44

int cl_protocol_version_from_demo;
int cl_protocol_flags_from_demo;

float CL_DemoReadCoord (void)
{
	if (cl_protocol_flags_from_demo & PRFL_FLOATCOORD)
		return MSG_ReadFloat ();
	else if (cl_protocol_flags_from_demo & PRFL_INT32COORD)
		return MSG_ReadLong () * (1.0 / 16.0);
	else if (cl_protocol_flags_from_demo & PRFL_24BITCOORD)
		return MSG_ReadShort() + MSG_ReadByte() * (1.0/255);
	return MSG_ReadShort() * (1.0/8);
}

float CL_DemoReadAngle (void)
{
	if (cl_protocol_flags_from_demo & PRFL_FLOATANGLE)
		return MSG_ReadFloat ();
	else if (cl_protocol_flags_from_demo & PRFL_SHORTANGLE)
		return MSG_ReadShort () * (360.0 / 65536);
	return MSG_ReadChar () * (360.0 / 256);
}

void CL_DemoReadTEntPosition (sizebuf_t& translated)
{
	if (cl_protocol_version_from_demo == PROTOCOL_FITZQUAKE || cl_protocol_version_from_demo == PROTOCOL_RMQ)
	{
		for (auto i=0 ; i<3 ; i++)
		{
			auto pos = CL_DemoReadCoord ();
			MSG_WriteFloat (&translated, pos);
		}
	}
	else if (cl_protocol_version_from_server == EXPANDED_PROTOCOL_VERSION)
	{
		for (auto i=0 ; i<3 ; i++)
		{
			auto pos = MSG_ReadFloat ();
			MSG_WriteFloat (&translated, pos);
		}
	}
	else
	{
		for (auto i=0 ; i<3 ; i++)
		{
			auto pos = MSG_ReadShort ();
			MSG_WriteShort (&translated, pos);
		}
	}
}

void CL_DemoParseBeam (sizebuf_t& translated)
{
	if (cl_protocol_version_from_demo == PROTOCOL_FITZQUAKE || cl_protocol_version_from_demo == PROTOCOL_RMQ)
	{
		auto ent = MSG_ReadShort ();
		MSG_WriteLong (&translated, ent);

		for (auto i=0 ; i<3 ; i++)
		{
			auto start = CL_DemoReadCoord ();
			MSG_WriteFloat (&translated, start);
		}

		for (auto i=0 ; i<3 ; i++)
		{
			auto end = CL_DemoReadCoord ();
			MSG_WriteFloat (&translated, end);
		}
	}
	else if (cl_protocol_version_from_server == EXPANDED_PROTOCOL_VERSION)
	{
		auto ent = MSG_ReadLong ();
		MSG_WriteLong (&translated, ent);
	
		for (auto i=0 ; i<3 ; i++)
		{
			auto start = MSG_ReadFloat ();
			MSG_WriteFloat (&translated, start);
		}

		for (auto i=0 ; i<3 ; i++)
		{
			auto end = MSG_ReadFloat ();
			MSG_WriteFloat (&translated, end);
		}
	}
	else
	{
		auto ent = MSG_ReadShort ();
		MSG_WriteShort (&translated, ent);
	
		for (auto i=0 ; i<3 ; i++)
		{
			auto start = MSG_ReadShort ();
			MSG_WriteShort (&translated, start);
		}
	
		for (auto i=0 ; i<3 ; i++)
		{
			auto end = MSG_ReadShort ();
			MSG_WriteShort (&translated, end);
		}
	}
}

int CL_DemoTranslate (void)
{
	static sizebuf_t translated;
	
	translated.Clear ();
	
//
// parse the message
//
	MSG_BeginReading ();
	
	while (1)
	{
		if (msg_badread)
			Host_Error ("CL_DemoTranslate: Bad server message in demo");

		auto cmd = MSG_ReadByte ();

		if (cmd == -1)
		{
			break;		// end of message
		}

	// if the high bit of the command byte is set, it is a fast update
		if (cmd & 128)
		{
			auto bits = cmd&127;

			if (bits & U_MOREBITS)
			{
				auto i = MSG_ReadByte ();
				bits |= (i<<8);
			}

			if (cl_protocol_version_from_demo == PROTOCOL_FITZQUAKE || cl_protocol_version_from_demo == PROTOCOL_RMQ)
			{
				if (bits & U_EXTEND1)
				{
					auto i = MSG_ReadByte ();
					bits |= (i<<16);
				}
				if (bits & U_EXTEND2)
				{
					auto i = MSG_ReadByte ();
					bits |= (i<<24);
				}
			}
			else if (cl_protocol_version_from_server == EXPANDED_PROTOCOL_VERSION)
			{
				if (bits & U_EVENMOREBITS)
				{
					auto i = MSG_ReadByte ();
					bits |= (i<<16);
				}
			}

			int num;
			auto modnum = -1;
			auto frame = -1;
			if (cl_protocol_version_from_demo == 0 && cl_protocol_version_from_server == EXPANDED_PROTOCOL_VERSION)
			{
				num = MSG_ReadLong ();
				
				if (bits & U_MODEL)
					modnum = MSG_ReadLong ();

				if (bits & U_FRAME)
					frame = MSG_ReadLong ();
			}
			else
			{
				if (bits & U_LONGENTITY)
					num = MSG_ReadShort ();
				else
					num = MSG_ReadByte ();

				if (bits & U_MODEL)
					modnum = MSG_ReadByte ();

				if (bits & U_FRAME)
					frame = MSG_ReadByte ();
			}

			auto colormap = -1;
			if (bits & U_COLORMAP)
				colormap = MSG_ReadByte();

			auto skinnum = -1;
			if (bits & U_SKIN)
				skinnum = MSG_ReadByte();

			auto effects = -1;
			if (bits & U_EFFECTS)
				effects = MSG_ReadByte();

			auto origin1_asfloat = FLT_MAX;
			auto origin1 = INT_MAX;
			auto angle1_asfloat = FLT_MAX;
			auto angle1 = INT_MAX;
			auto origin2_asfloat = FLT_MAX;
			auto origin2 = INT_MAX;
			auto angle2_asfloat = FLT_MAX;
			auto angle2 = INT_MAX;
			auto origin3_asfloat = FLT_MAX;
			auto origin3 = INT_MAX;
			auto angle3_asfloat = FLT_MAX;
			auto angle3 = INT_MAX;
			auto alpha = -1;
			auto scale = -1;
			if (cl_protocol_version_from_demo == PROTOCOL_FITZQUAKE || cl_protocol_version_from_demo == PROTOCOL_RMQ)
			{
				if (bits & U_ORIGIN1)
					origin1_asfloat = CL_DemoReadCoord ();
				if (bits & U_ANGLE1)
					angle1_asfloat = CL_DemoReadAngle ();

				if (bits & U_ORIGIN2)
					origin2_asfloat = CL_DemoReadCoord ();
				if (bits & U_ANGLE2)
					angle2_asfloat = CL_DemoReadAngle ();

				if (bits & U_ORIGIN3)
					origin3_asfloat = CL_DemoReadCoord ();
				if (bits & U_ANGLE3)
					angle3_asfloat = CL_DemoReadAngle ();

				if (bits & U_ALPHA_FR)
					alpha = MSG_ReadByte();
				
				if (bits & U_SCALE_FR)
					scale = MSG_ReadByte();

				if (bits & U_FRAME2)
					frame = (frame & 0x00FF) | (MSG_ReadByte() << 8);
				if (bits & U_MODEL2)
					modnum = (modnum & 0x00FF) | (MSG_ReadByte() << 8);
				if (bits & U_LERPFINISH)
					MSG_ReadByte ();
			}
			else if (cl_protocol_version_from_server == EXPANDED_PROTOCOL_VERSION)
			{
				if (bits & U_ORIGIN1)
					origin1_asfloat = MSG_ReadFloat ();
				if (bits & U_ANGLE1)
					angle1_asfloat = MSG_ReadFloat ();

				if (bits & U_ORIGIN2)
					origin2_asfloat = MSG_ReadFloat ();
				if (bits & U_ANGLE2)
					angle2_asfloat = MSG_ReadFloat ();

				if (bits & U_ORIGIN3)
					origin3_asfloat = MSG_ReadFloat ();
				if (bits & U_ANGLE3)
					angle3_asfloat = MSG_ReadFloat ();

				if (bits & U_ALPHA)
					alpha = MSG_ReadByte();
				
				if (bits & U_SCALE)
					scale = MSG_ReadByte();
			}
			else
			{
				if (bits & U_ORIGIN1)
					origin1 = MSG_ReadShort ();
				if (bits & U_ANGLE1)
					angle1 = MSG_ReadChar ();

				if (bits & U_ORIGIN2)
					origin2 = MSG_ReadShort ();
				if (bits & U_ANGLE2)
					angle2 = MSG_ReadChar ();

				if (bits & U_ORIGIN3)
					origin3 = MSG_ReadShort ();
				if (bits & U_ANGLE3)
					angle3 = MSG_ReadChar ();

				if (bits & U_TRANS)
				{
					float a = MSG_ReadFloat();
					MSG_ReadFloat(); //alpha
					if (a == 2)
						MSG_ReadFloat(); //fullbright (not using this yet)
				}
			}

			MSG_WriteByte (&translated, cmd);

			auto evenmorebits = 0;
			if (alpha != -1 || scale != -1)
			{
				bits |= U_EVENMOREBITS;

				if (alpha != -1)
					evenmorebits |= U_ALPHA;

				if (scale != -1)
					evenmorebits |= U_SCALE;
			}
			else
				bits &= (~U_EVENMOREBITS);
			
			if (bits & U_MOREBITS)
				MSG_WriteByte (&translated, bits>>8);
			
			if (evenmorebits)
				MSG_WriteByte (&translated, evenmorebits>>16);

			if (cl_protocol_version_from_server == EXPANDED_PROTOCOL_VERSION)
			{
				MSG_WriteLong (&translated, num);
				
				if (modnum != -1)
					MSG_WriteLong (&translated, modnum);

				if (frame != -1)
					MSG_WriteLong (&translated, frame);
			}
			else
			{
				if (bits & U_LONGENTITY)
					MSG_WriteShort (&translated, num);
				else
					MSG_WriteByte (&translated, num);

				if (modnum != -1)
					MSG_WriteByte (&translated, modnum);

				if (frame != -1)
					MSG_WriteByte (&translated, frame);
			}

			if (colormap != -1)
				MSG_WriteByte (&translated, colormap);

			if (skinnum != -1)
				MSG_WriteByte (&translated, skinnum);

			if (effects != -1)
				MSG_WriteByte (&translated, effects);

			if (origin1_asfloat != FLT_MAX)
				MSG_WriteFloat (&translated, origin1_asfloat);
			else if (origin1 != INT_MAX)
				MSG_WriteShort (&translated, origin1);
			if (angle1_asfloat != FLT_MAX)
				MSG_WriteFloat (&translated, angle1_asfloat);
			else if (angle1 != INT_MAX)
				MSG_WriteChar (&translated, angle1);

			if (origin2_asfloat != FLT_MAX)
				MSG_WriteFloat (&translated, origin2_asfloat);
			else if (origin2 != INT_MAX)
				MSG_WriteShort (&translated, origin2);
			if (angle2_asfloat != FLT_MAX)
				MSG_WriteFloat (&translated, angle2_asfloat);
			else if (angle2 != INT_MAX)
				MSG_WriteChar (&translated, angle2);

			if (origin3_asfloat != FLT_MAX)
				MSG_WriteFloat (&translated, origin3_asfloat);
			else if (origin3 != INT_MAX)
				MSG_WriteShort (&translated, origin3);
			if (angle3_asfloat != FLT_MAX)
				MSG_WriteFloat (&translated, angle3_asfloat);
			else if (angle3 != INT_MAX)
				MSG_WriteChar (&translated, angle3);

			if (alpha != -1)
				MSG_WriteByte (&translated, alpha);
			
			if (scale != -1)
				MSG_WriteByte (&translated, scale);

			continue;
		}

	// other commands
		switch (cmd)
		{
		default:
			Host_Error ("CL_DemoTranslate: Illegible server message in demo\n");
			break;
			
		case svc_nop:
		case svc_disconnect:
		case svc_killedmonster:
		case svc_foundsecret:
		case svc_intermission:
		case svc_sellscreen:
			MSG_WriteByte (&translated, cmd);
			break;

		case svc_time:
			{
				MSG_WriteByte (&translated, cmd);

				auto mtime = MSG_ReadFloat ();
				MSG_WriteFloat (&translated, mtime);
			}
			break;
			
		case svc_clientdata:
			{
				auto bits = MSG_ReadShort ();

				if (cl_protocol_version_from_demo == PROTOCOL_FITZQUAKE || cl_protocol_version_from_demo == PROTOCOL_RMQ)
				{
					if (bits & SU_EXTEND1)
					{
						auto i = MSG_ReadByte ();
						bits |= (i<<16);
					}
					if (bits & SU_EXTEND2)
					{
						auto i = MSG_ReadByte ();
						bits |= (i<<24);
					}
				}

				auto viewheight = INT_MAX;
				if (bits & SU_VIEWHEIGHT)
					viewheight = MSG_ReadChar ();

				auto idealpitch = INT_MAX;
				if (bits & SU_IDEALPITCH)
					idealpitch = MSG_ReadChar ();

				auto punchangle1 = INT_MAX;
				if (bits & (SU_PUNCH1) )
					punchangle1 = MSG_ReadChar();

				auto mvelocity1 = INT_MAX;
				if (bits & (SU_VELOCITY1) )
					mvelocity1 = MSG_ReadChar();

				auto punchangle2 = INT_MAX;
				if (bits & (SU_PUNCH2) )
					punchangle2 = MSG_ReadChar();

				auto mvelocity2 = INT_MAX;
				if (bits & (SU_VELOCITY2) )
					mvelocity2 = MSG_ReadChar();

				auto punchangle3 = INT_MAX;
				if (bits & (SU_PUNCH3) )
					punchangle3 = MSG_ReadChar();

				auto mvelocity3 = INT_MAX;
				if (bits & (SU_VELOCITY3) )
					mvelocity3 = MSG_ReadChar();

// [always sent]	if (bits & SU_ITEMS)
				auto items = MSG_ReadLong ();

				auto weaponframe = -1;
				if (bits & SU_WEAPONFRAME)
					weaponframe = MSG_ReadByte ();

				auto armor = -1;
				if (bits & SU_ARMOR)
					armor = MSG_ReadByte ();

				auto weapon = -1;
				if (bits & SU_WEAPON)
					weapon = MSG_ReadByte ();

				auto health = MSG_ReadShort ();

				auto ammo = MSG_ReadByte ();

				auto shells = MSG_ReadByte ();
				auto nails = MSG_ReadByte ();
				auto rockets = MSG_ReadByte ();
				auto cells = MSG_ReadByte ();

				auto activeweapon = MSG_ReadByte ();

				auto emitexpanded = false;

				if (cl_protocol_version_from_demo == PROTOCOL_FITZQUAKE || cl_protocol_version_from_demo == PROTOCOL_RMQ)
				{
					if (bits & SU_WEAPON2)
						weapon |= (MSG_ReadByte() << 8);
					if (bits & SU_ARMOR2)
						armor |= (MSG_ReadByte() << 8);
					if (bits & SU_AMMO2)
						ammo |= (MSG_ReadByte() << 8);
					if (bits & SU_SHELLS2)
						shells |= (MSG_ReadByte() << 8);
					if (bits & SU_NAILS2)
						nails |= (MSG_ReadByte() << 8);
					if (bits & SU_ROCKETS2)
						rockets |= (MSG_ReadByte() << 8);
					if (bits & SU_CELLS2)
						cells |= (MSG_ReadByte() << 8);
					if (bits & SU_WEAPONFRAME2)
						weaponframe |= (MSG_ReadByte() << 8);
					if (bits & SU_WEAPONALPHA)
						MSG_ReadByte();

					emitexpanded = true;

					MSG_WriteByte (&translated, svc_expandedclientdata);

					MSG_WriteLong (&translated, bits & 65535);
				}
				else
				{
					MSG_WriteByte (&translated, cmd);

					MSG_WriteShort (&translated, bits);
				}

				if (viewheight != INT_MAX)
					MSG_WriteChar (&translated, viewheight);

				if (idealpitch != INT_MAX)
					MSG_WriteChar (&translated, idealpitch);

				if (punchangle1 != INT_MAX)
					MSG_WriteChar (&translated, punchangle1);

				if (mvelocity1 != INT_MAX)
					MSG_WriteChar (&translated, mvelocity1);

				if (punchangle2 != INT_MAX)
					MSG_WriteChar (&translated, punchangle2);

				if (mvelocity2 != INT_MAX)
					MSG_WriteChar (&translated, mvelocity2);

				if (punchangle3 != INT_MAX)
					MSG_WriteChar (&translated, punchangle3);

				if (mvelocity3 != INT_MAX)
					MSG_WriteChar (&translated, mvelocity3);

				MSG_WriteLong (&translated, items);

				if (emitexpanded)
				{
					if (weaponframe != -1)
						MSG_WriteLong (&translated, weaponframe);

					if (armor != -1)
						MSG_WriteLong (&translated, armor);

					if (weapon != -1)
						MSG_WriteLong (&translated, weapon);

					MSG_WriteLong (&translated, health);

					MSG_WriteLong (&translated, ammo);

					MSG_WriteLong (&translated, shells);
					MSG_WriteLong (&translated, nails);
					MSG_WriteLong (&translated, rockets);
					MSG_WriteLong (&translated, cells);

					MSG_WriteLong (&translated, activeweapon);
				}
				else
				{
					if (weaponframe != -1)
						MSG_WriteByte (&translated, weaponframe);

					if (armor != -1)
						MSG_WriteByte (&translated, armor);

					if (weapon != -1)
						MSG_WriteByte (&translated, weapon);

					MSG_WriteShort (&translated, health);

					MSG_WriteByte (&translated, ammo);

					MSG_WriteByte (&translated, shells);
					MSG_WriteByte (&translated, nails);
					MSG_WriteByte (&translated, rockets);
					MSG_WriteByte (&translated, cells);

					MSG_WriteByte (&translated, activeweapon);
				}
			}
			break;
			
		case svc_expandedclientdata:
			{
				MSG_WriteByte (&translated, cmd);

				auto bits = MSG_ReadLong ();
				MSG_WriteLong (&translated, bits);

				if (bits & SU_VIEWHEIGHT)
				{
					auto viewheight = MSG_ReadChar ();
					MSG_WriteChar (&translated, viewheight);
				}

				if (bits & SU_IDEALPITCH)
				{
					auto idealpitch = MSG_ReadChar ();
					MSG_WriteChar (&translated, idealpitch);
				}

				for (auto i=0 ; i<3 ; i++)
				{
					if (bits & (SU_PUNCH1<<i) )
					{
						auto punchangle = MSG_ReadChar();
						MSG_WriteChar (&translated, punchangle);
					}
					if (bits & (SU_VELOCITY1<<i) )
					{
						auto mvelocity = MSG_ReadChar();
						MSG_WriteChar (&translated, mvelocity);
					}
				}

// [always sent]	if (bits & SU_ITEMS)
				auto items = MSG_ReadLong ();
				MSG_WriteLong (&translated, items);

				if (bits & SU_WEAPONFRAME)
				{
					auto weaponframe = MSG_ReadLong ();
					MSG_WriteLong (&translated, weaponframe);
				}

				if (bits & SU_ARMOR)
				{
					auto armor = MSG_ReadLong ();
					MSG_WriteLong (&translated, armor);
				}

				if (bits & SU_WEAPON)
				{
					auto weapon = MSG_ReadLong ();
					MSG_WriteLong (&translated, weapon);
				}

				auto health = MSG_ReadLong ();
				MSG_WriteLong (&translated, health);

				auto ammo = MSG_ReadLong ();
				MSG_WriteLong (&translated, ammo);

				for (auto i=0 ; i<4 ; i++)
				{
					auto j = MSG_ReadLong ();
					MSG_WriteLong (&translated, j);
				}

				auto activeweapon = MSG_ReadLong ();
				MSG_WriteLong (&translated, activeweapon);
			}
			break;
		
		case svc_version:
			{
				MSG_WriteByte (&translated, cmd);

				auto i = MSG_ReadLong ();

				if (i == PROTOCOL_FITZQUAKE || i == PROTOCOL_RMQ)
				{
					cl_protocol_version_from_demo = i;
					cl_protocol_version_from_server = EXPANDED_PROTOCOL_VERSION;
				}
				else if (i == EXPANDED_PROTOCOL_VERSION || i == PROTOCOL_VERSION)
				{
					cl_protocol_version_from_demo = 0;
					cl_protocol_version_from_server = i;
				}
				else
				{
					Host_Error ("CL_DemoTranslate: Demo is protocol %i instead of %i or %i or %i or 0x%.8x\n", i, PROTOCOL_VERSION, PROTOCOL_FITZQUAKE, PROTOCOL_RMQ, EXPANDED_PROTOCOL_VERSION);
				}

				MSG_WriteLong (&translated, cl_protocol_version_from_server);
			}
			break;

		case svc_print:
		case svc_centerprint:
		case svc_stufftext:
		case svc_finale:
		case svc_cutscene:
			{
				MSG_WriteByte (&translated, cmd);

				auto msg = MSG_ReadString ();
				MSG_WriteString (&translated, msg);
			}
			break;

		case svc_damage:
			{
				auto armor = MSG_ReadByte ();
				auto blood = MSG_ReadByte ();
				auto from1_asfloat = FLT_MAX;
				auto from1 = INT_MAX;
				auto from2_asfloat = FLT_MAX;
				auto from2 = INT_MAX;
				auto from3_asfloat = FLT_MAX;
				auto from3 = INT_MAX;
				auto emitexpanded = false;
				if (cl_protocol_version_from_demo == PROTOCOL_FITZQUAKE || cl_protocol_version_from_demo == PROTOCOL_RMQ)
				{
					from1_asfloat = CL_DemoReadCoord ();
					from2_asfloat = CL_DemoReadCoord ();
					from3_asfloat = CL_DemoReadCoord ();

					emitexpanded = true;

					MSG_WriteByte (&translated, svc_expandeddamage);
				}
				else
				{
					from1 = MSG_ReadShort ();
					from2 = MSG_ReadShort ();
					from3 = MSG_ReadShort ();

					MSG_WriteByte (&translated, cmd);
				}

				MSG_WriteByte (&translated, armor);
				MSG_WriteByte (&translated, blood);
				if (from1_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, from1_asfloat);
				else
					MSG_WriteShort (&translated, from1);
				if (from2_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, from2_asfloat);
				else
					MSG_WriteShort (&translated, from2);
				if (from3_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, from3_asfloat);
				else
					MSG_WriteShort (&translated, from3);
			}
			break;

		case svc_expandeddamage:
			{
				MSG_WriteByte (&translated, cmd);

				auto armor = MSG_ReadByte ();
				MSG_WriteByte (&translated, armor);
				auto blood = MSG_ReadByte ();
				MSG_WriteByte (&translated, blood);
				for (auto i=0 ; i<3 ; i++)
				{
					auto from = MSG_ReadFloat ();
					MSG_WriteFloat (&translated, from);
				}
			}
			break;

		case svc_serverinfo:
			{
				MSG_WriteByte (&translated, cmd);

				auto i = MSG_ReadLong ();

				if (i == PROTOCOL_FITZQUAKE || i == PROTOCOL_RMQ)
				{
					cl_protocol_version_from_demo = i;
					cl_protocol_version_from_server = EXPANDED_PROTOCOL_VERSION;
				}
				else if (i == EXPANDED_PROTOCOL_VERSION || i == PROTOCOL_VERSION)
				{
					cl_protocol_version_from_demo = 0;
					cl_protocol_version_from_server = i;
				}
				else
				{
					Host_Error ("CL_DemoTranslate: Demo returned version %i, not %i or %i or %i or 0x%.8x\n", i, PROTOCOL_VERSION, PROTOCOL_FITZQUAKE, PROTOCOL_RMQ, EXPANDED_PROTOCOL_VERSION);
				}

				MSG_WriteLong (&translated, cl_protocol_version_from_server);

				if (cl_protocol_version_from_demo == PROTOCOL_RMQ)
				{
					const unsigned int supportedflags = (PRFL_SHORTANGLE | PRFL_FLOATANGLE | PRFL_24BITCOORD | PRFL_FLOATCOORD | PRFL_EDICTSCALE | PRFL_INT32COORD);
					
					auto flags = (unsigned int) MSG_ReadLong ();
					
					if (0 != (flags & (~supportedflags)))
					{
						Con_Printf("PROTOCOL_RMQ protocolflags %i contains unsupported flags\n", flags);
					}

					cl_protocol_flags_from_demo = flags;
				}

				auto maxclients = MSG_ReadByte ();
				MSG_WriteByte (&translated, maxclients);

				auto gametype = MSG_ReadByte ();
				MSG_WriteByte (&translated, gametype);

				auto levelname = MSG_ReadString ();
				MSG_WriteString (&translated, levelname);

				for (auto nummodels=1 ; ; nummodels++)
				{
					auto str = MSG_ReadString ();
					MSG_WriteString (&translated, str);
					if (!str[0])
						break;
				}

				for (auto numsounds=1 ; ; numsounds++)
				{
					auto str = MSG_ReadString ();
					MSG_WriteString (&translated, str);
					if (!str[0])
						break;
				}
			}
			break;
			
		case svc_setangle:
			{
				MSG_WriteByte (&translated, cmd);

				if (cl_protocol_version_from_demo == PROTOCOL_FITZQUAKE || cl_protocol_version_from_demo == PROTOCOL_RMQ)
				{
					for (auto i=0 ; i<3 ; i++)
					{
						auto angle = CL_DemoReadAngle ();
						MSG_WriteAngle (&translated, angle);
					}
				}
				else
				{
					for (auto i=0 ; i<3 ; i++)
					{
						auto angle = MSG_ReadChar ();
						MSG_WriteByte (&translated, angle);
					}
				}
			}
			break;

		case svc_setview:
		case svc_stopsound:
			{
				MSG_WriteByte (&translated, cmd);

				auto i = MSG_ReadShort ();
				MSG_WriteShort (&translated, i);
			}
			break;
					
		case svc_lightstyle:
			{
				MSG_WriteByte (&translated, cmd);

				auto i = MSG_ReadByte ();
				MSG_WriteByte (&translated, i);
				auto map = MSG_ReadString();
				MSG_WriteString (&translated, map);
			}
			break;

		case svc_sound:
			{
    			auto field_mask = MSG_ReadByte();

				auto volume = -1;
				if (field_mask & SND_VOLUME)
					volume = MSG_ReadByte ();

				auto attenuation = -1;
				if (field_mask & SND_ATTENUATION)
					attenuation = MSG_ReadByte ();

				auto emitexpanded = false;

				int	channel, ent;
				if (field_mask & SND_LARGEENTITY)
				{
					ent = MSG_ReadShort ();
					channel = MSG_ReadByte ();

					emitexpanded = true;
				}
				else
				{
					channel = MSG_ReadShort ();
					ent = channel >> 3;
					channel &= 7;
				}

				int	sound_num;
				if (field_mask & SND_LARGESOUND)
				{
					sound_num = MSG_ReadShort ();

					emitexpanded = true;
				}
				else
					sound_num = MSG_ReadByte ();

				auto pos1_asfloat = FLT_MAX;
				auto pos1 = INT_MAX;
				auto pos2_asfloat = FLT_MAX;
				auto pos2 = INT_MAX;
				auto pos3_asfloat = FLT_MAX;
				auto pos3 = INT_MAX;
				if (cl_protocol_version_from_demo == PROTOCOL_FITZQUAKE || cl_protocol_version_from_demo == PROTOCOL_RMQ)
				{
					pos1_asfloat = CL_DemoReadCoord ();
					pos2_asfloat = CL_DemoReadCoord ();
					pos3_asfloat = CL_DemoReadCoord ();

					emitexpanded = true;

					MSG_WriteByte (&translated, svc_expandedsound);
				}
				else
				{
					pos1 = MSG_ReadShort ();
					pos2 = MSG_ReadShort ();
					pos3 = MSG_ReadShort ();

					MSG_WriteByte (&translated, cmd);
				}

				MSG_WriteByte (&translated, field_mask);

				if (volume != -1)
					MSG_WriteByte (&translated, volume);

				if (attenuation != -1)
					MSG_WriteByte (&translated, attenuation);

				if (emitexpanded)
				{
					MSG_WriteLong (&translated, ent);
					MSG_WriteLong (&translated, channel);
					MSG_WriteLong (&translated, sound_num);
				}
				else
				{
					channel = (ent<<3) | channel;

					MSG_WriteShort (&translated, channel);
					MSG_WriteByte (&translated, sound_num);
				}

				if (pos1_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, pos1_asfloat);
				else
					MSG_WriteShort (&translated, pos1);

				if (pos2_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, pos2_asfloat);
				else
					MSG_WriteShort (&translated, pos2);

				if (pos3_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, pos3_asfloat);
				else
					MSG_WriteShort (&translated, pos3);
			}
			break;
			
		case svc_expandedsound:
			{
				MSG_WriteByte (&translated, cmd);

				auto field_mask = MSG_ReadByte();
				MSG_WriteByte (&translated, field_mask);

				if (field_mask & SND_VOLUME)
				{
					auto volume = MSG_ReadByte ();
					MSG_WriteByte (&translated, volume);
				}
					
				if (field_mask & SND_ATTENUATION)
				{
					auto attenuation = MSG_ReadByte ();
					MSG_WriteByte (&translated, attenuation);
				}

				auto ent = MSG_ReadLong ();
				MSG_WriteLong (&translated, ent);
				auto channel = MSG_ReadLong ();
				MSG_WriteLong (&translated, channel);
				auto sound_num = MSG_ReadLong ();
				MSG_WriteLong (&translated, sound_num);

				for (auto i=0 ; i<3 ; i++)
				{
					auto pos = MSG_ReadFloat ();
					MSG_WriteFloat (&translated, pos);
				}
			}
			break;

		case svc_updatename:
			{
				MSG_WriteByte (&translated, cmd);

				auto i = MSG_ReadByte ();
				MSG_WriteByte (&translated, i);
				auto name = MSG_ReadString ();
				MSG_WriteString (&translated, name);
			}
			break;
			
		case svc_updatefrags:
			{
				MSG_WriteByte (&translated, cmd);

				auto i = MSG_ReadByte ();
				MSG_WriteByte (&translated, i);
				auto frags = MSG_ReadShort ();
				MSG_WriteShort (&translated, frags);
			}
			break;

		case svc_updatecolors:
		case svc_cdtrack:
			{
				MSG_WriteByte (&translated, cmd);

				auto i = MSG_ReadByte ();
				MSG_WriteByte (&translated, i);
				auto j = MSG_ReadByte ();
				MSG_WriteByte (&translated, j);
			}
			break;
			
		case svc_particle:
			{
				auto emitexpanded = false;

				auto org1_asfloat = FLT_MAX;
				auto org1 = INT_MAX;
				auto org2_asfloat = FLT_MAX;
				auto org2 = INT_MAX;
				auto org3_asfloat = FLT_MAX;
				auto org3 = INT_MAX;
				if (cl_protocol_version_from_demo == PROTOCOL_FITZQUAKE || cl_protocol_version_from_demo == PROTOCOL_RMQ)
				{
					org1_asfloat = CL_DemoReadCoord ();
					org2_asfloat = CL_DemoReadCoord ();
					org3_asfloat = CL_DemoReadCoord ();

					emitexpanded = true;
				}
				else
				{
					org1 = MSG_ReadShort ();
					org2 = MSG_ReadShort ();
					org3 = MSG_ReadShort ();
				}

				auto dir1 = MSG_ReadChar ();
				auto dir2 = MSG_ReadChar ();
				auto dir3 = MSG_ReadChar ();

				auto msgcount = MSG_ReadByte ();
				auto color = MSG_ReadByte ();

				if (emitexpanded)
					MSG_WriteByte (&translated, svc_expandedparticle);
				else
					MSG_WriteByte (&translated, cmd);

				if (org1_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, org1_asfloat);
				else
					MSG_WriteShort (&translated, org1);
				if (org2_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, org2_asfloat);
				else
					MSG_WriteShort (&translated, org2);
				if (org3_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, org3_asfloat);
				else
					MSG_WriteShort (&translated, org3);

				MSG_WriteChar (&translated, dir1);
				MSG_WriteChar (&translated, dir2);
				MSG_WriteChar (&translated, dir3);

				MSG_WriteByte (&translated, msgcount);
				MSG_WriteByte (&translated, color);
			}
			break;

		case svc_expandedparticle:
			{
				MSG_WriteByte (&translated, cmd);

				for (auto i=0 ; i<3 ; i++)
				{
					auto org = MSG_ReadFloat ();
					MSG_WriteFloat (&translated, org);
				}
				for (auto i=0 ; i<3 ; i++)
				{
					auto dir = MSG_ReadChar ();
					MSG_WriteChar (&translated, dir);
				}
				auto msgcount = MSG_ReadByte ();
				MSG_WriteByte (&translated, msgcount);
				auto color = MSG_ReadByte ();
				MSG_WriteByte (&translated, color);
			}
			break;

		case svc_spawnbaseline:
			{
				auto ent = MSG_ReadShort ();

				auto modelindex = MSG_ReadByte ();
				auto frame = MSG_ReadByte ();
				auto colormap = MSG_ReadByte();
				auto skin = MSG_ReadByte();

				auto emitexpanded = false;

				auto origin1_asfloat = FLT_MAX;
				auto origin1 = INT_MAX;
				auto angle1_asfloat = FLT_MAX;
				auto angle1 = INT_MAX;
				auto origin2_asfloat = FLT_MAX;
				auto origin2 = INT_MAX;
				auto angle2_asfloat = FLT_MAX;
				auto angle2 = INT_MAX;
				auto origin3_asfloat = FLT_MAX;
				auto origin3 = INT_MAX;
				auto angle3_asfloat = FLT_MAX;
				auto angle3 = INT_MAX;
				if (cl_protocol_version_from_demo == PROTOCOL_FITZQUAKE || cl_protocol_version_from_demo == PROTOCOL_RMQ)
				{
					origin1_asfloat = CL_DemoReadCoord ();
					angle1_asfloat = CL_DemoReadAngle ();
					origin2_asfloat = CL_DemoReadCoord ();
					angle2_asfloat = CL_DemoReadAngle ();
					origin3_asfloat = CL_DemoReadCoord ();
					angle3_asfloat = CL_DemoReadAngle ();

					emitexpanded = true;

					MSG_WriteByte (&translated, svc_expandedspawnbaseline);

					MSG_WriteLong (&translated, ent);

					MSG_WriteLong (&translated, modelindex);
					MSG_WriteLong (&translated, frame);
				}
				else
				{
					origin1 = MSG_ReadShort ();
					angle1 = MSG_ReadChar ();
					origin2 = MSG_ReadShort ();
					angle2 = MSG_ReadChar ();
					origin3 = MSG_ReadShort ();
					angle3 = MSG_ReadChar ();

					MSG_WriteByte (&translated, cmd);

					MSG_WriteShort (&translated, ent);

					MSG_WriteByte (&translated, modelindex);
					MSG_WriteByte (&translated, frame);
				}

				MSG_WriteByte (&translated, colormap);
				MSG_WriteByte (&translated, skin);

				if (origin1_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, origin1_asfloat);
				else
					MSG_WriteShort (&translated, origin1);
				if (angle1_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, angle1_asfloat);
				else
					MSG_WriteChar (&translated, angle1);

				if (origin2_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, origin2_asfloat);
				else
					MSG_WriteShort (&translated, origin2);
				if (angle2_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, angle2_asfloat);
				else
					MSG_WriteChar (&translated, angle2);

				if (origin3_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, origin3_asfloat);
				else
					MSG_WriteShort (&translated, origin3);
				if (angle3_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, angle3_asfloat);
				else
					MSG_WriteChar (&translated, angle3);

				if (emitexpanded)
				{
					MSG_WriteByte (&translated, 0);
					MSG_WriteByte (&translated, 0);
				}
			}
			break;

		case svc_expandedspawnbaseline:
			{
				MSG_WriteByte (&translated, cmd);

				auto ent = MSG_ReadLong ();
				MSG_WriteLong (&translated, ent);

				auto modelindex = MSG_ReadLong ();
				MSG_WriteLong (&translated, modelindex);
				auto frame = MSG_ReadLong ();
				MSG_WriteLong (&translated, frame);
				auto colormap = MSG_ReadByte();
				MSG_WriteByte (&translated, colormap);
				auto skin = MSG_ReadByte();
				MSG_WriteByte (&translated, skin);
				for (auto i=0 ; i<3 ; i++)
				{
					auto origin = MSG_ReadFloat ();
					MSG_WriteFloat (&translated, origin);
					auto angle = MSG_ReadFloat ();
					MSG_WriteFloat (&translated, angle);
				}
				auto alpha = MSG_ReadByte();
				MSG_WriteByte (&translated, alpha);
				auto scale = MSG_ReadByte();
				MSG_WriteByte (&translated, scale);
			}
			break;

		case svc_spawnstatic:
			{
				auto modelindex = MSG_ReadByte ();
				auto frame = MSG_ReadByte ();
				auto colormap = MSG_ReadByte();
				auto skin = MSG_ReadByte();

				auto emitexpanded = false;

				auto origin1_asfloat = FLT_MAX;
				auto origin1 = INT_MAX;
				auto angle1_asfloat = FLT_MAX;
				auto angle1 = INT_MAX;
				auto origin2_asfloat = FLT_MAX;
				auto origin2 = INT_MAX;
				auto angle2_asfloat = FLT_MAX;
				auto angle2 = INT_MAX;
				auto origin3_asfloat = FLT_MAX;
				auto origin3 = INT_MAX;
				auto angle3_asfloat = FLT_MAX;
				auto angle3 = INT_MAX;
				if (cl_protocol_version_from_demo == PROTOCOL_FITZQUAKE || cl_protocol_version_from_demo == PROTOCOL_RMQ)
				{
					origin1_asfloat = CL_DemoReadCoord ();
					angle1_asfloat = CL_DemoReadAngle ();
					origin2_asfloat = CL_DemoReadCoord ();
					angle2_asfloat = CL_DemoReadAngle ();
					origin3_asfloat = CL_DemoReadCoord ();
					angle3_asfloat = CL_DemoReadAngle ();

					emitexpanded = true;

					MSG_WriteByte (&translated, svc_expandedspawnstatic);

					MSG_WriteLong (&translated, modelindex);
					MSG_WriteLong (&translated, frame);
				}
				else
				{
					origin1 = MSG_ReadShort ();
					angle1 = MSG_ReadChar ();
					origin2 = MSG_ReadShort ();
					angle2 = MSG_ReadChar ();
					origin3 = MSG_ReadShort ();
					angle3 = MSG_ReadChar ();

					MSG_WriteByte (&translated, cmd);

					MSG_WriteByte (&translated, modelindex);
					MSG_WriteByte (&translated, frame);
				}

				MSG_WriteByte (&translated, colormap);
				MSG_WriteByte (&translated, skin);

				if (origin1_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, origin1_asfloat);
				else
					MSG_WriteShort (&translated, origin1);
				if (angle1_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, angle1_asfloat);
				else
					MSG_WriteChar (&translated, angle1);

				if (origin2_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, origin2_asfloat);
				else
					MSG_WriteShort (&translated, origin2);
				if (angle2_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, angle2_asfloat);
				else
					MSG_WriteChar (&translated, angle2);

				if (origin3_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, origin3_asfloat);
				else
					MSG_WriteShort (&translated, origin3);
				if (angle3_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, angle3_asfloat);
				else
					MSG_WriteChar (&translated, angle3);

				if (emitexpanded)
				{
					MSG_WriteByte (&translated, 0);
					MSG_WriteByte (&translated, 0);
				}
			}
			break;

		case svc_expandedspawnstatic:
			{
				MSG_WriteByte (&translated, cmd);

				auto modelindex = MSG_ReadLong ();
				MSG_WriteLong (&translated, modelindex);
				auto frame = MSG_ReadLong ();
				MSG_WriteLong (&translated, frame);
				auto colormap = MSG_ReadByte();
				MSG_WriteByte (&translated, colormap);
				auto skin = MSG_ReadByte();
				MSG_WriteByte (&translated, skin);
				for (auto i=0 ; i<3 ; i++)
				{
					auto origin = MSG_ReadFloat ();
					MSG_WriteFloat (&translated, origin);
					auto angle = MSG_ReadFloat ();
					MSG_WriteFloat (&translated, angle);
				}
				auto alpha = MSG_ReadByte();
				MSG_WriteByte (&translated, alpha);
				auto scale = MSG_ReadByte();
				MSG_WriteByte (&translated, scale);
			}
			break;

		case svc_temp_entity:
			{
				MSG_WriteByte (&translated, cmd);

				auto type = MSG_ReadByte ();
				MSG_WriteByte (&translated, type);
				switch (type)
				{
				case TE_WIZSPIKE:
				case TE_KNIGHTSPIKE:
				case TE_SPIKE:
				case TE_SUPERSPIKE:
				case TE_GUNSHOT:
				case TE_EXPLOSION:
				case TE_TAREXPLOSION:
				case TE_LAVASPLASH:
				case TE_TELEPORT:
					CL_DemoReadTEntPosition (translated);
					break;

				case TE_LIGHTNING1:
				case TE_LIGHTNING2:
				case TE_LIGHTNING3:
				case TE_BEAM:
					CL_DemoParseBeam (translated);
					break;
					
				case TE_EXPLOSION2:
					{
						CL_DemoReadTEntPosition (translated);
						auto colorStart = MSG_ReadByte ();
						MSG_WriteByte (&translated, colorStart);
						auto colorLength = MSG_ReadByte ();
						MSG_WriteByte (&translated, colorLength);
					}
					break;

				default:
					Host_Error ("CL_DemoTranslate: bad svc_temp_entity type");
				}
			}
			break;

		case svc_setpause:
		case svc_signonnum:
			{
				MSG_WriteByte (&translated, cmd);

				auto paused = MSG_ReadByte ();
				MSG_WriteByte (&translated, paused);
			}
			break;

		case svc_updatestat:
			{
				MSG_WriteByte (&translated, cmd);

				auto i = MSG_ReadByte ();
				MSG_WriteByte (&translated, i);
				auto stats = MSG_ReadLong ();
				MSG_WriteLong (&translated, stats);
			}
			break;
			
		case svc_spawnstaticsound:
			{
				auto emitexpanded = false;

				auto org1_asfloat = FLT_MAX;
				auto org1 = INT_MAX;
				auto org2_asfloat = FLT_MAX;
				auto org2 = INT_MAX;
				auto org3_asfloat = FLT_MAX;
				auto org3 = INT_MAX;
				if (cl_protocol_version_from_demo == PROTOCOL_FITZQUAKE || cl_protocol_version_from_demo == PROTOCOL_RMQ)
				{
					org1_asfloat = CL_DemoReadCoord ();
					org2_asfloat = CL_DemoReadCoord ();
					org3_asfloat = CL_DemoReadCoord ();

					emitexpanded = true;
				}
				else
				{
					org1 = MSG_ReadShort ();
					org2 = MSG_ReadShort ();
					org3 = MSG_ReadShort ();
				}
				auto sound_num = MSG_ReadByte ();
				auto vol = MSG_ReadByte ();
				auto atten = MSG_ReadByte ();

				if (emitexpanded)
					MSG_WriteByte (&translated, svc_expandedspawnstaticsound);
				else
					MSG_WriteByte (&translated, cmd);

				if (org1_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, org1_asfloat);
				else
					MSG_WriteShort (&translated, org1);
				if (org2_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, org2_asfloat);
				else
					MSG_WriteShort (&translated, org2);
				if (org3_asfloat != FLT_MAX)
					MSG_WriteFloat (&translated, org3_asfloat);
				else
					MSG_WriteShort (&translated, org3);
				if (emitexpanded)
					MSG_WriteLong (&translated, sound_num);
				else
					MSG_WriteByte (&translated, sound_num);
				MSG_WriteByte (&translated, vol);
				MSG_WriteByte (&translated, atten);
			}
			break;
			
		case svc_expandedspawnstaticsound:
			{
				MSG_WriteByte (&translated, cmd);

				for (auto i=0 ; i<3 ; i++)
				{
					auto org = MSG_ReadFloat ();
					MSG_WriteFloat (&translated, org);
				}
				auto sound_num = MSG_ReadLong ();
				MSG_WriteLong (&translated, sound_num);
				auto vol = MSG_ReadByte ();
				MSG_WriteByte (&translated, vol);
				auto atten = MSG_ReadByte ();
				MSG_WriteByte (&translated, atten);
			}
			break;

		case svc_skybox:
			MSG_ReadString();
			break;

		case svc_bf:
			// Unsupported, but valid command. Do nothing.
			break;

		case svc_fog:
			MSG_ReadByte();
			MSG_ReadByte();
			MSG_ReadByte();
			MSG_ReadByte();
			MSG_ReadShort();
			break;

		case svc_spawnbaseline2:
			{
				MSG_WriteByte (&translated, svc_expandedspawnbaseline);

				auto ent = MSG_ReadShort ();
				MSG_WriteLong (&translated, ent);

				auto bits = MSG_ReadByte ();

				int modelindex;
				if (bits & B_LARGEMODEL)
					modelindex = MSG_ReadShort ();
				else
					modelindex = MSG_ReadByte ();
				MSG_WriteLong (&translated, modelindex);

				int frame;
				if (bits & B_LARGEFRAME)
					frame = MSG_ReadShort ();
				else
					frame = MSG_ReadByte ();
				MSG_WriteLong (&translated, frame);

				auto colormap = MSG_ReadByte();
				MSG_WriteByte (&translated, colormap);
				auto skin = MSG_ReadByte();
				MSG_WriteByte (&translated, skin);

				for (auto i = 0; i < 3; i++)
				{
					auto origin = CL_DemoReadCoord ();
					MSG_WriteFloat (&translated, origin);
					auto angle = CL_DemoReadAngle ();
					MSG_WriteFloat (&translated, angle);
				}

				auto alpha = 0;
				if (bits & B_ALPHA)
					alpha = MSG_ReadByte();
				MSG_WriteByte (&translated, alpha);

				auto scale = 0;
				if (bits & B_SCALE)
					scale = MSG_ReadByte();
				MSG_WriteByte (&translated, scale);
			}
			break;

		case svc_spawnstatic2:
			{
				MSG_WriteByte (&translated, svc_expandedspawnstatic);

				auto bits = MSG_ReadByte ();

				int modelindex;
				if (bits & B_LARGEMODEL)
					modelindex = MSG_ReadShort ();
				else
					modelindex = MSG_ReadByte ();
				MSG_WriteLong (&translated, modelindex);

				int frame;
				if (bits & B_LARGEFRAME)
					frame = MSG_ReadShort ();
				else
					frame = MSG_ReadByte ();
				MSG_WriteLong (&translated, frame);

				auto colormap = MSG_ReadByte();
				MSG_WriteByte (&translated, colormap);
				auto skin = MSG_ReadByte();
				MSG_WriteByte (&translated, skin);

				for (auto i=0 ; i<3 ; i++)
				{
					auto origin = CL_DemoReadCoord ();
					MSG_WriteFloat (&translated, origin);
					auto angle = CL_DemoReadAngle ();
					MSG_WriteFloat (&translated, angle);
				}

				auto alpha = 0;
				if (bits & B_ALPHA)
					alpha = MSG_ReadByte ();
				MSG_WriteByte (&translated, alpha);

				auto scale = 0;
				if (bits & B_SCALE)
					alpha = MSG_ReadByte ();
				MSG_WriteByte (&translated, scale);
			}
			break;

		case svc_spawnstaticsound2:
			{
				MSG_WriteByte (&translated, svc_expandedspawnstaticsound);

				for (auto i=0 ; i<3 ; i++)
				{
					auto org = CL_DemoReadCoord ();
					MSG_WriteFloat (&translated, org);
				}

				auto sound_num = MSG_ReadShort ();
				MSG_WriteLong (&translated, sound_num);
				auto vol = MSG_ReadByte ();
				MSG_WriteByte (&translated, vol);
				auto atten = MSG_ReadByte ();
				MSG_WriteByte (&translated, atten);
			}
			break;
		}
	}

	net_message = translated;

	return 1;
}
