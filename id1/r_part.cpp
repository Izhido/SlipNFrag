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

#include "quakedef.h"
#include "r_local.h"
#include "d_lists.h"

#define MAX_PARTICLES			2048	// default max # of particles at one
										//  time
#define ABSOLUTE_MIN_PARTICLES	512		// no fewer than this no matter what's
										//  on the command line

int		ramp1[8] = {0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61};
int		ramp2[8] = {0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66};
int		ramp3[8] = {0x6d, 0x6b, 6, 5, 4, 3};

particle_t	*active_particles, *free_particles;

std::vector<particle_t>	particles;
int			r_numparticles;

vec3_t			r_pright, r_pup, r_ppn;
qboolean r_increaseparticles;

/*
===============
R_InitParticles
===============
*/
void R_InitParticles (void)
{
	int		i;

	i = COM_CheckParm ("-particles");

	if (i)
	{
		r_numparticles = (int)(Q_atoi(com_argv[i+1]));
		if (r_numparticles < ABSOLUTE_MIN_PARTICLES)
			r_numparticles = ABSOLUTE_MIN_PARTICLES;
	}
}

#ifdef QUAKE2
void R_DarkFieldParticles (entity_t *ent)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;
	vec3_t		org;

	org[0] = ent->origin[0];
	org[1] = ent->origin[1];
	org[2] = ent->origin[2];
	for (i=-16 ; i<16 ; i+=8)
		for (j=-16 ; j<16 ; j+=8)
			for (k=0 ; k<32 ; k+=8)
			{
				if (!free_particles)
					return;
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;
		
				p->die = cl.time + 0.2 + (rand()&7) * 0.02;
				p->color = 150 + rand()%6;
				p->type = pt_slowgrav;
				
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;
	
				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}
#endif


/*
===============
R_EntityParticles
===============
*/

#define NUMVERTEXNORMALS	162
extern	float	r_avertexnormals[NUMVERTEXNORMALS][3];
vec3_t	avelocities[NUMVERTEXNORMALS];
float	beamlength = 16;
vec3_t	avelocity = {23, 7, 3};
float	partstep = 0.01;
float	timescale = 0.01;

void R_EntityParticles (entity_t *ent)
{
	int			count;
	int			i;
	particle_t	*p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist;
	
	dist = 64;
	count = 50;

if (!avelocities[0][0])
{
for (i=0 ; i<NUMVERTEXNORMALS*3 ; i++)
avelocities[0][i] = (Sys_Random()&255) * 0.01;
}


	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		angle = cl.time * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = cl.time * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = cl.time * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);
	
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		if (!free_particles)
		{
			if (r_numparticles == 0)
				r_increaseparticles = true;
			return;
		}
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->die = cl.time + 0.01;
		p->color = 0x6f;
		p->type = pt_explode;
		
		p->org[0] = ent->origin[0] + r_avertexnormals[i][0]*dist + forward[0]*beamlength;			
		p->org[1] = ent->origin[1] + r_avertexnormals[i][1]*dist + forward[1]*beamlength;			
		p->org[2] = ent->origin[2] + r_avertexnormals[i][2]*dist + forward[2]*beamlength;			
	}
}


/*
===============
R_ClearParticles
===============
*/
void R_ClearParticles (void)
{
	int		i;
	
	if (particles.empty())
	{
		if (r_numparticles == 0)
			particles.resize(MAX_PARTICLES);
		else
			particles.resize(r_numparticles);
	}
	free_particles = &particles[0];
	active_particles = NULL;

	for (i=0 ;i<particles.size()-1 ; i++)
		particles[i].next = &particles[i+1];
	particles[particles.size()-1].next = NULL;
}

void R_IncreaseParticles()
{
	auto activeindex = -1;
	if (active_particles != nullptr)
	{
		activeindex = active_particles - particles.data();
	}
	auto freeindex = -1;
	if (free_particles != nullptr)
	{
		freeindex = free_particles - particles.data();
	}
	std::vector<int> nextindices(particles.size());
	for (size_t i = 0; i < particles.size(); i++)
	{
		if (particles[i].next == nullptr)
		{
			nextindices[i] = -1;
			continue;
		}
		nextindices[i] = particles[i].next - particles.data();
	}
	particles.resize(particles.size() + MAX_PARTICLES);
	auto lastparticle = -1;
	for (size_t i = 0; i < nextindices.size(); i++)
	{
		if (nextindices[i] < 0)
		{
			lastparticle = i;
			continue;
		}
		particles[i].next = &particles[nextindices[i]];
	}
	if (lastparticle < 0)
	{
		Sys_Error("lastparticle < 0");
	}
	particles[lastparticle].next = &particles[nextindices.size()];
	for (size_t i = nextindices.size(); i < particles.size() - 1; i++)
	{
		particles[i].next = &particles[i + 1];
	}
	particles[particles.size() - 1].next = nullptr;
	if (activeindex >= 0)
	{
		active_particles = &particles[activeindex];
	}
	if (freeindex >= 0)
	{
		free_particles = &particles[freeindex];
	}
}

void R_ReadPointFile_f (void)
{
	int	f;
	vec3_t	org;
	int		r;
	int		c;
	particle_t	*p;
	std::string	name;
	
	name = std::string("maps/") + (pr_strings + sv.name) + ".pts";

	COM_FOpenFile (name.c_str(), &f);
	if (f < 0)
	{
		Con_Printf ("couldn't open %s\n", name.c_str());
		return;
	}
	
	Con_Printf ("Reading %s...\n", name.c_str());
	c = 0;
	for ( ;; )
	{
		std::string to_read;
		char onechar;
		auto len = Sys_FileRead (cls.demofile, &onechar, 1);
		while (len == 1 && onechar != '\n')
		{
			to_read.push_back(onechar);
			len = Sys_FileRead (cls.demofile, &onechar, 1);
		}
		to_read.push_back('\n');
		
		r = sscanf (to_read.c_str(), "%f %f %f\n", &org[0], &org[1], &org[2]);
		if (r != 3)
			break;
		c++;
		
		if (!free_particles)
		{
			Con_Printf ("Not enough free particles\n");
			break;
		}
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		
		p->die = 99999;
		p->color = (-c)&15;
		p->type = pt_static;
		VectorCopy (vec3_origin, p->vel);
		VectorCopy (org, p->org);
	}

	Sys_FileClose (f);
	Con_Printf ("%i points read\n", c);
}

/*
===============
R_ParseParticleEffect

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect (void)
{
	vec3_t		org, dir;
	int			i, count, msgcount, color;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	for (i=0 ; i<3 ; i++)
		dir[i] = MSG_ReadChar () * (1.0/16);
	msgcount = MSG_ReadByte ();
	color = MSG_ReadByte ();

if (msgcount == 255)
	count = 1024;
else
	count = msgcount;
	
	R_RunParticleEffect (org, dir, color, count);
}
	
/*
===============
R_ParseExpandedParticleEffect

Parse an effect out of the server message
using the expanded protocol
===============
*/
void R_ParseExpandedParticleEffect(void)
{
	vec3_t		org, dir;
	int			i, count, msgcount, color;

	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadFloat ();
	for (i=0 ; i<3 ; i++)
		dir[i] = MSG_ReadChar () * (1.0 / 16);
	msgcount = MSG_ReadByte ();
	color = MSG_ReadByte ();

	if (msgcount == 255)
		count = 1024;
	else
		count = msgcount;

	R_RunParticleEffect(org, dir, color, count);
}

/*
===============
R_ParticleExplosion

===============
*/
void R_ParticleExplosion (const vec3_t org)
{
	int			i, j;
	particle_t	*p;
	
	for (i=0 ; i<1024 ; i++)
	{
		if (!free_particles)
		{
			if (r_numparticles == 0)
				r_increaseparticles = true;
			return;
		}
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->die = cl.time + 5;
		p->color = ramp1[0];
		p->ramp = Sys_Random()&3;
		if (i & 1)
		{
			p->type = pt_explode;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((Sys_Random()%32)-16);
				p->vel[j] = (Sys_Random()%512)-256;
			}
		}
		else
		{
			p->type = pt_explode2;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((Sys_Random()%32)-16);
				p->vel[j] = (Sys_Random()%512)-256;
			}
		}
	}
}

/*
===============
R_ParticleExplosion2

===============
*/
void R_ParticleExplosion2 (const vec3_t org, int colorStart, int colorLength)
{
	int			i, j;
	particle_t	*p;
	int			colorMod = 0;

	for (i=0; i<512; i++)
	{
		if (!free_particles)
		{
			if (r_numparticles == 0)
				r_increaseparticles = true;
			return;
		}
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->die = cl.time + 0.3;
		p->color = colorStart + (colorMod % colorLength);
		colorMod++;

		p->type = pt_blob;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((Sys_Random()%32)-16);
			p->vel[j] = (Sys_Random()%512)-256;
		}
	}
}

/*
===============
R_BlobExplosion

===============
*/
void R_BlobExplosion (const vec3_t org)
{
	int			i, j;
	particle_t	*p;
	
	for (i=0 ; i<1024 ; i++)
	{
		if (!free_particles)
		{
			if (r_numparticles == 0)
				r_increaseparticles = true;
			return;
		}
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->die = cl.time + 1 + (Sys_Random()&8)*0.05;

		if (i & 1)
		{
			p->type = pt_blob;
			p->color = 66 + Sys_Random()%6;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((Sys_Random()%32)-16);
				p->vel[j] = (Sys_Random()%512)-256;
			}
		}
		else
		{
			p->type = pt_blob2;
			p->color = 150 + Sys_Random()%6;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((Sys_Random()%32)-16);
				p->vel[j] = (Sys_Random()%512)-256;
			}
		}
	}
}

/*
===============
R_RunParticleEffect

===============
*/
void R_RunParticleEffect (const vec3_t org, const vec3_t dir, int color, int count)
{
	int			i, j;
	particle_t	*p;
	
	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
		{
			if (r_numparticles == 0)
				r_increaseparticles = true;
			return;
		}
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		if (count == 1024)
		{	// rocket explosion
			p->die = cl.time + 5;
			p->color = ramp1[0];
			p->ramp = Sys_Random()&3;
			if (i & 1)
			{
				p->type = pt_explode;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = org[j] + ((Sys_Random()%32)-16);
					p->vel[j] = (Sys_Random()%512)-256;
				}
			}
			else
			{
				p->type = pt_explode2;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = org[j] + ((Sys_Random()%32)-16);
					p->vel[j] = (Sys_Random()%512)-256;
				}
			}
		}
		else
		{
			p->die = cl.time + 0.1*(Sys_Random()%5);
			p->color = (color&~7) + (Sys_Random()&7);
			p->type = pt_slowgrav;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((Sys_Random()&15)-8);
				p->vel[j] = dir[j]*15;// + (Sys_Random()%300)-150;
			}
		}
	}
}


/*
===============
R_LavaSplash

===============
*/
void R_LavaSplash (const vec3_t org)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i++)
		for (j=-16 ; j<16 ; j++)
			for (k=0 ; k<1 ; k++)
			{
				if (!free_particles)
				{
					if (r_numparticles == 0)
						r_increaseparticles = true;
					return;
				}
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;
		
				p->die = cl.time + 2 + (Sys_Random()&31) * 0.02;
				p->color = 224 + (Sys_Random()&7);
				p->type = pt_slowgrav;
				
				dir[0] = j*8 + (Sys_Random()&7);
				dir[1] = i*8 + (Sys_Random()&7);
				dir[2] = 256;
	
				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (Sys_Random()&63);
	
				VectorNormalize (dir);						
				vel = 50 + (Sys_Random()&63);
				VectorScale (dir, vel, p->vel);
			}
}

/*
===============
R_TeleportSplash

===============
*/
void R_TeleportSplash (const vec3_t org)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i+=4)
		for (j=-16 ; j<16 ; j+=4)
			for (k=-24 ; k<32 ; k+=4)
			{
				if (!free_particles)
				{
					if (r_numparticles == 0)
						r_increaseparticles = true;
					return;
				}
				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;
		
				p->die = cl.time + 0.2 + (Sys_Random()&7) * 0.02;
				p->color = 7 + (Sys_Random()&7);
				p->type = pt_slowgrav;
				
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;
	
				p->org[0] = org[0] + i + (Sys_Random()&3);
				p->org[1] = org[1] + j + (Sys_Random()&3);
				p->org[2] = org[2] + k + (Sys_Random()&3);
	
				VectorNormalize (dir);						
				vel = 50 + (Sys_Random()&63);
				VectorScale (dir, vel, p->vel);
			}
}

void R_RocketTrail (vec3_t start, const vec3_t end, int type)
{
	vec3_t		vec;
	float		len;
	int			j;
	particle_t	*p;
	int			dec;
	static int	tracercount;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	if (type < 128)
		dec = 3;
	else
	{
		dec = 1;
		type -= 128;
	}

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
		{
			if (r_numparticles == 0)
				r_increaseparticles = true;
			return;
		}
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		
		VectorCopy (vec3_origin, p->vel);
		p->die = cl.time + 2;

		switch (type)
		{
			case 0:	// rocket trail
				p->ramp = (Sys_Random()&3);
				p->color = ramp3[(int)p->ramp];
				p->type = pt_fire;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((Sys_Random()%6)-3);
				break;

			case 1:	// smoke smoke
				p->ramp = (Sys_Random()&3) + 2;
				p->color = ramp3[(int)p->ramp];
				p->type = pt_fire;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((Sys_Random()%6)-3);
				break;

			case 2:	// blood
				p->type = pt_grav;
				p->color = 67 + (Sys_Random()&3);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((Sys_Random()%6)-3);
				break;

			case 3:
			case 5:	// tracer
				p->die = cl.time + 0.5;
				p->type = pt_static;
				if (type == 3)
					p->color = 52 + ((tracercount&4)<<1);
				else
					p->color = 230 + ((tracercount&4)<<1);
			
				tracercount++;

				VectorCopy (start, p->org);
				if (tracercount & 1)
				{
					p->vel[0] = 30*vec[1];
					p->vel[1] = 30*-vec[0];
				}
				else
				{
					p->vel[0] = 30*-vec[1];
					p->vel[1] = 30*vec[0];
				}
				break;

			case 4:	// slight blood
				p->type = pt_grav;
				p->color = 67 + (Sys_Random()&3);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((Sys_Random()%6)-3);
				len -= 3;
				break;

			case 6:	// voor trail
				p->color = 9*16 + 8 + (Sys_Random()&3);
				p->type = pt_static;
				p->die = cl.time + 0.3;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((Sys_Random()&15)-8);
				break;
		}
		

		VectorAdd (start, vec, start);
	}
}


/*
===============
R_MoveParticles
===============
*/
extern	cvar_t	sv_gravity;

void R_MoveParticles (void)
{
	particle_t		*p, *kill;
	float			grav;
	int				i;
	float			time2, time3;
	float			time1;
	float			dvel;
	float			frametime;
	
	if (r_increaseparticles)
	{
		R_IncreaseParticles();
		r_increaseparticles = false;
	}

	frametime = cl.time - cl.oldtime;
	time3 = frametime * 15;
	time2 = frametime * 10; // 15;
	time1 = frametime * 5;
	grav = frametime * sv_gravity.value * 0.05;
	dvel = 4*frametime;
	
	for ( ;; ) 
	{
		kill = active_particles;
		if (kill && kill->die < cl.time)
		{
			active_particles = kill->next;
			kill->next = free_particles;
			free_particles = kill;
			continue;
		}
		break;
	}

	for (p=active_particles ; p ; p=p->next)
	{
		for ( ;; )
		{
			kill = p->next;
			if (kill && kill->die < cl.time)
			{
				p->next = kill->next;
				kill->next = free_particles;
				free_particles = kill;
				continue;
			}
			break;
		}

		p->org[0] += p->vel[0]*frametime;
		p->org[1] += p->vel[1]*frametime;
		p->org[2] += p->vel[2]*frametime;
		
		switch (p->type)
		{
		case pt_static:
			break;
		case pt_fire:
			p->ramp += time1;
			if (p->ramp >= 6)
				p->die = -1;
			else
				p->color = ramp3[(int)p->ramp];
			p->vel[2] += grav;
			break;

		case pt_explode:
			p->ramp += time2;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->color = ramp1[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_explode2:
			p->ramp += time3;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->color = ramp2[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] -= p->vel[i]*frametime;
			p->vel[2] -= grav;
			break;

		case pt_blob:
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_blob2:
			for (i=0 ; i<2 ; i++)
				p->vel[i] -= p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_grav:
#ifdef QUAKE2
			p->vel[2] -= grav * 20;
			break;
#endif
		case pt_slowgrav:
			p->vel[2] -= grav;
			break;
		}
	}
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
	particle_t		*p;

	D_StartParticles ();

	if (d_uselists)
	{
		for (p=active_particles ; p ; p=p->next)
		{
			D_AddParticleToLists(p);
		}
	}
	else
	{
		VectorScale (vright, xscaleshrink, r_pright);
		VectorScale (vup, yscaleshrink, r_pup);
		VectorCopy (vpn, r_ppn);

		for (p=active_particles ; p ; p=p->next)
		{
			D_DrawParticle (p);
		}
	}

	D_EndParticles ();
}

