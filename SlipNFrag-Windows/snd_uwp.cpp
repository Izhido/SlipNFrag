#include "quakedef.h"

int snd_current_sample_pos = 0;
qboolean snd_forceclear;

qboolean SNDDMA_Init(void)
{
    shm = new dma_t;
    shm->splitbuffer = 0;
    shm->samplebits = 16;
    shm->speed = 44100;
    shm->channels = 2;
    shm->samples = 65536;
    shm->samplepos = 0;
    shm->soundalive = true;
    shm->gamealive = true;
    shm->submission_chunk = (shm->samples >> 3);
    shm->buffer.resize(shm->samples * (shm->samplebits >> 3) * shm->channels);
    snd_current_sample_pos = shm->samples >> 1;
    return true;
}

int SNDDMA_GetDMAPos(void)
{
	return snd_current_sample_pos;
}

void SNDDMA_Submit(void)
{
}

void SNDDMA_Shutdown(void)
{
	delete shm;
	shm = nullptr;
}
