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
// snd_mem.c: sound caching

#include "quakedef.h"

int			cache_full_cycle;

byte *S_Alloc (int size);

/*
================
ResampleSfx
================
*/
void ResampleSfx (sfx_t *sfx, int inrate, int inwidth, byte *data)
{
	int		outcount;
	int		srcsample;
    int     srcsamplefrac;
	float	stepscale;
	int		i;
	int		y0, y1, y2, y3, samplefrac, fracstep, sample;
    float   a0, a1, a2, a3, mu, mu2;
	sfxcache_t	*sc;
	
	sc = (sfxcache_t*)sfx->data;
	if (!sc)
		return;

	stepscale = (float)inrate / shm->speed;	// this is usually 0.5, 1, or 2

	outcount = sc->length / stepscale;
	sc->length = outcount;
	if (sc->loopstart != -1)
		sc->loopstart = sc->loopstart / stepscale;

	sc->speed = shm->speed;
	if (loadas8bit.value)
		sc->width = 1;
	else
		sc->width = inwidth;
	sc->stereo = 0;

// resample / decimate to the current source rate

	if (stepscale == 1 && inwidth == 1 && sc->width == 1)
	{
// fast special case
		for (i=0 ; i<outcount ; i++)
			((signed char *)sc->data)[i]
			= (int)( (unsigned char)(data[i]) - 128);
	}
	else
	{
// general case
		samplefrac = 0;
		fracstep = stepscale*256;
        for (i=0 ; i<outcount ; i++)
        {
            srcsample = samplefrac >> 8;
            srcsamplefrac = samplefrac - (srcsample << 8);
            samplefrac += fracstep;
            if (inwidth == 2)
                y1 = LittleShort ( ((short *)data)[srcsample] );
            else
                y1 = (int)( (unsigned char)(data[srcsample]) - 128) << 8;
            if (srcsample == 0)
            {
                y0 = y1;
            }
            else
            {
                if (inwidth == 2)
                    y0 = LittleShort ( ((short *)data)[srcsample - 1] );
                else
                    y0 = (int)( (unsigned char)(data[srcsample - 1]) - 128) << 8;
            }
            if (srcsample >= sc->length - 1)
            {
                y2 = y1;
            }
            else
            {
                if (inwidth == 2)
                    y2 = LittleShort ( ((short *)data)[srcsample + 1] );
                else
                    y2 = (int)( (unsigned char)(data[srcsample + 1]) - 128) << 8;
            }
            if (srcsample >= sc->length - 2)
            {
                y3 = y2;
            }
            else
            {
                if (inwidth == 2)
                    y3 = LittleShort ( ((short *)data)[srcsample + 2] );
                else
                    y3 = (int)( (unsigned char)(data[srcsample + 2]) - 128) << 8;
            }
            a0 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
            a1 = y0 - 2.5 * y1 + 2 * y2 - 0.5 * y3;
            a2 = -0.5 * y0 + 0.5 * y2;
            a3 = y1;
            mu = srcsamplefrac / 256;
            mu2 = mu * mu;
            sample = (int)(a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
            if (sc->width == 2)
                ((short *)sc->data)[i] = sample;
            else
                ((signed char *)sc->data)[i] = sample >> 8;
        }
	}
}

//=============================================================================

/*
==============
S_LoadSound
==============
*/
sfxcache_t *S_LoadSound (sfx_t *s)
{
    std::string namebuffer;
	byte	*data;
	wavinfo_t	info;
	int		len;
	float	stepscale;
	sfxcache_t	*sc;

// see if still in memory
    sc = (sfxcache_t*)s->data;
	if (sc)
		return sc;

//Con_Printf ("S_LoadSound: %x\n", (int)stackbuf);
// load it in
    namebuffer = "sound/" + s->name;

//	Con_Printf ("loading %s\n",namebuffer);

    data = nullptr;
    std::vector<byte> contents;
    int handle = -1;
    auto length = COM_OpenFile(namebuffer.c_str(), &handle);
    if (handle >= 0 && length > 0)
    {
        contents.resize(length + 1);
        if (Sys_FileRead(handle, contents.data(), length) == length)
        {
            data = contents.data();
            data[length] = 0;
        }
        COM_CloseFile(handle);
    }

	if (!data)
	{
		Con_Printf ("Couldn't load %s\n", namebuffer.c_str());
		return NULL;
	}

	info = GetWavinfo (s->name.c_str(), data, com_filesize);
	if (info.channels != 1)
	{
		Con_Printf ("%s is a stereo sample\n",s->name.c_str());
		return NULL;
	}
	if (info.width != 1 && info.width != 2)
	{
		Con_Printf ("%s is not either 8-bit or 16-bit\n",s->name.c_str());
		return NULL;
	}

	stepscale = (float)info.rate / shm->speed;	
	len = info.samples / stepscale;

	len = len * info.width * info.channels;

    s->data = new byte[len + sizeof(sfxcache_t)];
    sc = (sfxcache_t*)s->data;
	if (!sc)
		return NULL;
	
	sc->length = info.samples;
	sc->loopstart = info.loopstart;
	sc->speed = info.rate;
	sc->width = info.width;
	sc->stereo = info.channels;

	ResampleSfx (s, sc->speed, sc->width, data + info.dataofs);

	return sc;
}



/*
===============================================================================

WAV loading

===============================================================================
*/


byte	*data_p;
byte 	*iff_end;
byte 	*last_chunk;
byte 	*iff_data;
int 	iff_chunk_len;


short GetLittleShort(void)
{
	short val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	data_p += 2;
	return val;
}

int GetLittleLong(void)
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	val = val + (*(data_p+2)<<16);
	val = val + (*(data_p+3)<<24);
	data_p += 4;
	return val;
}

void FindNextChunk(const char *name)
{
	while (1)
	{
		data_p=last_chunk;

		if (data_p + 8 >= iff_end)
		{	// didn't find the chunk
			data_p = NULL;
			return;
		}
		
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return;
		}
//		if (iff_chunk_len > 1024*1024)
//			Sys_Error ("FindNextChunk: %i length is past the 1 meg sanity limit", iff_chunk_len);
		data_p -= 8;
		last_chunk = data_p + 8 + ( (iff_chunk_len + 1) & ~1 );
		if (!Q_strncmp((char*)data_p, name, 4))
			return;
	}
}

void FindChunk(const char *name)
{
	last_chunk = iff_data;
	FindNextChunk (name);
}


void DumpChunks(void)
{
	char	str[5];
	
	str[4] = 0;
	data_p=iff_data;
	do
	{
		memcpy (str, data_p, 4);
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		Con_Printf ("0x%x : %s (%d)\n", (int)(data_p - (byte*)4), str, iff_chunk_len);
		data_p += (iff_chunk_len + 1) & ~1;
	} while (data_p < iff_end);
}

/*
============
GetWavinfo
============
*/
wavinfo_t GetWavinfo (const char *name, byte *wav, int wavlength)
{
	wavinfo_t	info;
	int     i;
	int     format;
	int		samples;

	memset (&info, 0, sizeof(info));

	if (!wav)
		return info;
		
	iff_data = wav;
	iff_end = wav + wavlength;

// find "RIFF" chunk
	FindChunk("RIFF");
	if (!(data_p && !Q_strncmp((char*)data_p+8, "WAVE", 4)))
	{
		Con_Printf("Missing RIFF/WAVE chunks\n");
		return info;
	}

// get "fmt " chunk
	iff_data = data_p + 12;
// DumpChunks ();

	FindChunk("fmt ");
	if (!data_p)
	{
		Con_Printf("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	format = GetLittleShort();
	if (format != 1)
	{
		Con_Printf("Microsoft PCM format only\n");
		return info;
	}

	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4+2;
	info.width = GetLittleShort() / 8;

// get cue chunk
	FindChunk("cue ");
	if (data_p)
	{
		data_p += 32;
		info.loopstart = GetLittleLong();
//		Con_Printf("loopstart=%d\n", sfx->loopstart);

	// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk ("LIST");
		if (data_p)
		{
			if (!strncmp ((char*)data_p + 28, "mark", 4))
			{	// this is not a proper parse, but it works with cooledit...
				data_p += 24;
				i = GetLittleLong ();	// samples in loop
				info.samples = info.loopstart + i;
//				Con_Printf("looped length: %i\n", i);
			}
		}
	}
	else
		info.loopstart = -1;

// find data chunk
	FindChunk("data");
	if (!data_p)
	{
		Con_Printf("Missing data chunk\n");
		return info;
	}

	data_p += 4;
	samples = GetLittleLong () / info.width;

	if (info.samples)
	{
		if (samples < info.samples)
			Sys_Error ("Sound %s has a bad loop length", name);
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;
	
	return info;
}

