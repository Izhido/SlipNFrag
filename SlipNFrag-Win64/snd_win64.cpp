#include "quakedef.h"
#include "AppState.h"
#include <mmeapi.h>
#include <mmiscapi.h>
#include <mmreg.h>
#include "Locks.h"

qboolean snd_forceclear;
WAVEFORMATEX snd_waveformat { };
HWAVEOUT snd_waveout = NULL;
HANDLE snd_wavebuffer1 = NULL;
HPSTR snd_wavedata1 = NULL;
HGLOBAL snd_waveheader1 = NULL;
LPWAVEHDR snd_waveheaderptr1 = NULL;
HANDLE snd_wavebuffer2 = NULL;
HPSTR snd_wavedata2 = NULL;
HGLOBAL snd_waveheader2 = NULL;
LPWAVEHDR snd_waveheaderptr2 = NULL;

void SNDDMA_ReleaseAll(void)
{
    if (snd_waveout != NULL)
    {
        waveOutReset(snd_waveout);
        if (snd_waveheader2 != NULL)
        {
            waveOutUnprepareHeader(snd_waveout, snd_waveheaderptr2, sizeof(WAVEHDR));
            GlobalUnlock(snd_waveheader2);
            GlobalFree(snd_waveheader2);
            snd_waveheader2 = NULL;
            snd_waveheaderptr2 = NULL;
        }
        if (snd_wavebuffer2 != NULL)
        {
            GlobalUnlock(snd_wavebuffer2);
            GlobalFree(snd_wavebuffer2);
            snd_wavebuffer2 = NULL;
            snd_wavedata2 = NULL;
        }
        if (snd_waveheader1 != NULL)
        {
            waveOutUnprepareHeader(snd_waveout, snd_waveheaderptr1, sizeof(WAVEHDR));
            GlobalUnlock(snd_waveheader1);
            GlobalFree(snd_waveheader1);
            snd_waveheader1 = NULL;
            snd_waveheaderptr1 = NULL;
        }
        if (snd_wavebuffer1 != NULL)
        {
            GlobalUnlock(snd_wavebuffer1);
            GlobalFree(snd_wavebuffer1);
            snd_wavebuffer1 = NULL;
            snd_wavedata1 = NULL;
        }
        waveOutClose(snd_waveout);
        snd_waveout = NULL;
    }
}

void SNDDMA_Callback(void* waveOut, void* waveHeader)
{
    std::lock_guard<std::mutex> lock(Locks::SoundMutex);

    if (shm == NULL || snd_waveout == NULL || snd_waveout != waveOut)
    {
        return;
    }

    if (snd_forceclear)
    {
        std::fill(shm->buffer.begin(), shm->buffer.end(), 0);
        snd_forceclear = false;
    }
    memcpy(((LPWAVEHDR)waveHeader)->lpData, shm->buffer.data() + (shm->samplepos << 2), shm->samples >> 1);
    shm->samplepos += (shm->samples >> 3);
    if (shm->samplepos >= shm->samples)
    {
        shm->samplepos = 0;
    }
    auto result = waveOutWrite(snd_waveout, ((LPWAVEHDR)waveHeader), sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR)
    {
        SNDDMA_ReleaseAll();
    }
}

qboolean SNDDMA_Init(void)
{
    shm = new dma_t;
    shm->splitbuffer = 0;
    shm->samplebits = 32;
    shm->speed = 44100;
    shm->channels = 2;
    shm->samples = 65536;
    shm->samplepos = 0;
    shm->soundalive = true;
    shm->gamealive = true;
    shm->submission_chunk = (shm->samples >> 3);
    shm->buffer.resize(shm->samples * shm->samplebits/8);

    snd_waveformat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    snd_waveformat.nChannels = shm->channels;
    snd_waveformat.nSamplesPerSec = shm->speed;
    snd_waveformat.nAvgBytesPerSec = shm->speed * shm->channels * (shm->samplebits / 8);
    snd_waveformat.nBlockAlign = shm->channels * (shm->samplebits / 8);
    snd_waveformat.wBitsPerSample = shm->samplebits;

    auto result = waveOutOpen(&snd_waveout, WAVE_MAPPER, &snd_waveformat, (DWORD_PTR)appState.hWnd, 0, CALLBACK_WINDOW);
    if (result != MMSYSERR_NOERROR)
    {
        SNDDMA_ReleaseAll();
        return false;
    }

    snd_wavebuffer1 = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, shm->samples >> 1);
    if (snd_wavebuffer1 == NULL)
    {
        SNDDMA_ReleaseAll();
        return false;
    }
    snd_wavedata1 = (LPSTR)GlobalLock(snd_wavebuffer1);
    if (snd_wavebuffer1 == NULL)
    {
        SNDDMA_ReleaseAll();
        return false;
    }
    snd_waveheader1 = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (DWORD)sizeof(WAVEHDR));
    if (snd_waveheader1 == NULL)
    {
        SNDDMA_ReleaseAll();
        return false;
    }
    snd_waveheaderptr1 = (LPWAVEHDR)GlobalLock(snd_waveheader1);
    if (snd_waveheaderptr1 == NULL)
    {
        SNDDMA_ReleaseAll();
        return false;
    }
    snd_waveheaderptr1->lpData = snd_wavedata1;
    snd_waveheaderptr1->dwBufferLength = (shm->samples >> 1);
    snd_waveheaderptr1->dwLoops = 0;
    snd_waveheaderptr1->dwFlags = 0;
    result = waveOutPrepareHeader(snd_waveout, snd_waveheaderptr1, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR)
    {
        SNDDMA_ReleaseAll();
        return false;
    }

    result = waveOutWrite(snd_waveout, snd_waveheaderptr1, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR)
    {
        SNDDMA_ReleaseAll();
        return false;
    }

    snd_wavebuffer2 = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, shm->samples >> 1);
    if (snd_wavebuffer2 == NULL)
    {
        SNDDMA_ReleaseAll();
        return false;
    }
    snd_wavedata2 = (LPSTR)GlobalLock(snd_wavebuffer2);
    if (snd_wavebuffer2 == NULL)
    {
        SNDDMA_ReleaseAll();
        return false;
    }
    snd_waveheader2 = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (DWORD)sizeof(WAVEHDR));
    if (snd_waveheader2 == NULL)
    {
        SNDDMA_ReleaseAll();
        return false;
    }
    snd_waveheaderptr2 = (LPWAVEHDR)GlobalLock(snd_waveheader2);
    if (snd_waveheaderptr2 == NULL)
    {
        SNDDMA_ReleaseAll();
        return false;
    }
    snd_waveheaderptr2->lpData = snd_wavedata2;
    snd_waveheaderptr2->dwBufferLength = (shm->samples >> 1);
    snd_waveheaderptr2->dwLoops = 0;
    snd_waveheaderptr2->dwFlags = 0;
    result = waveOutPrepareHeader(snd_waveout, snd_waveheaderptr2, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR)
    {
        SNDDMA_ReleaseAll();
        return false;
    }

    result = waveOutWrite(snd_waveout, snd_waveheaderptr2, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR)
    {
        SNDDMA_ReleaseAll();
        return false;
    }

    return true;
}

int SNDDMA_GetDMAPos(void)
{
    return shm->samplepos;
}

void SNDDMA_Submit(void)
{
}

void SNDDMA_ClearBuffer(void)
{
    std::lock_guard<std::mutex> lock(Locks::SoundMutex);

    std::fill(shm->buffer.begin(), shm->buffer.end(), 0);
}

void SNDDMA_Shutdown(void)
{
    std::lock_guard<std::mutex> lock(Locks::SoundMutex);

    SNDDMA_ReleaseAll();
	delete shm;
	shm = nullptr;
}
