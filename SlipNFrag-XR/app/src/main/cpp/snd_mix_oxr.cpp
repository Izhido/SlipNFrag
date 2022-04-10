#include "quakedef.h"

#define	PAINTBUFFER_SIZE	512
float	paintbuffer[2 * PAINTBUFFER_SIZE];

void S_TransferPaintBuffer(int endtime)
{
	int 	out_idx;
	int 	count;
	int 	out_mask;
	float 	*p;
	float	snd_vol;
	float	*pbuf;

	p = paintbuffer;
	count = (endtime - paintedtime) * shm->channels;
	out_mask = shm->samples - 1; 
	out_idx = paintedtime * shm->channels & out_mask;
	snd_vol = volume.value;

	while (count > 0)
	{
		int n;
		if (out_idx + count > shm->samples)
		{
			n = out_idx + count - shm->samples;
		}
		else
		{
			n = count;
		}
		pbuf = (float *)shm->buffer.data() + out_idx;
		for (auto i=0; i<n; i++)
		{
			*pbuf++ = (*p++ * snd_vol);
		}
		count -= n;
		out_idx = 0;
	}
}


/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

void SND_PaintChannel (channel_t *ch, sfxcache_t *sc, int endtime);

void S_PaintChannels(int endtime)
{
	int 	i;
	int 	end;
	channel_t *ch;
	sfxcache_t	*sc;
	int		ltime, count;

	while (paintedtime < endtime)
	{
	// if paintbuffer is smaller than DMA buffer
		end = endtime;
		if (endtime - paintedtime > PAINTBUFFER_SIZE)
			end = paintedtime + PAINTBUFFER_SIZE;

	// clear the paint buffer
		memset(paintbuffer, 0, (end - paintedtime) * 2 * sizeof(float));

	// paint in the channels.
		ch = channels.data();
		for (i=0; i<total_channels ; i++, ch++)
		{
			if (!ch->sfx)
				continue;
			if (!ch->leftvol && !ch->rightvol)
				continue;
			sc = S_LoadSound (ch->sfx);
			if (!sc)
				continue;

			ltime = paintedtime;

			while (ltime < end)
			{	// paint up to end
				if (ch->end < end)
					count = ch->end - ltime;
				else
					count = end - ltime;

				if (count > 0)
				{	
						SND_PaintChannel(ch, sc, count);

					ltime += count;
				}

			// if at end of loop, restart
				if (ltime >= ch->end)
				{
					if (sc->loopstart >= 0)
					{
						ch->pos = sc->loopstart;
						ch->end = ltime + sc->length - ch->pos;
					}
					else				
					{	// channel just stopped
						ch->sfx = NULL;
						break;
					}
				}
			}
															  
		}

		ch = s_static_channels.data();
		for (i=0; i<s_total_static_channels ; i++, ch++)
		{
			if (!ch->sfx)
				continue;
			if (!ch->leftvol && !ch->rightvol)
				continue;
			sc = S_LoadSound (ch->sfx);
			if (!sc)
				continue;

			ltime = paintedtime;

			while (ltime < end)
			{	// paint up to end
				if (ch->end < end)
					count = ch->end - ltime;
				else
					count = end - ltime;

				if (count > 0)
				{	
						SND_PaintChannel(ch, sc, count);

					ltime += count;
				}

			// if at end of loop, restart
				if (ltime >= ch->end)
				{
					if (sc->loopstart >= 0)
					{
						ch->pos = sc->loopstart;
						ch->end = ltime + sc->length - ch->pos;
					}
					else				
					{	// channel just stopped
						ch->sfx = NULL;
						break;
					}
				}
			}
															  
		}

	// transfer out according to DMA format
		S_TransferPaintBuffer(end);
		paintedtime = end;
	}
}

void SND_InitScaletable (void)
{
}


void SND_PaintChannel (channel_t *ch, sfxcache_t *sc, int count)
{
	float data;
	float left, right;
	float leftvol, rightvol;
	float *sfx;
	int	i;

	leftvol = (float)ch->leftvol / 256;
	rightvol = (float)ch->rightvol / 256;
	sfx = (float *)sc->data + ch->pos;

	auto p = 0;
	for (i=0 ; i<count ; i++)
	{
		data = sfx[i];
		left = data * leftvol;
		right = data * rightvol;
		paintbuffer[p++] += left;
		paintbuffer[p++] += right;
	}

	ch->pos += count;
}

