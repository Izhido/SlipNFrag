#include "quakedef.h"
#include <common/xr_dependencies.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmeapi.h>
#include <mmiscapi.h>
#include <mmreg.h>
#include <mmdeviceapi.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "Locks.h"

extern wchar_t snd_audio_output_device_id[XR_MAX_AUDIO_DEVICE_STR_SIZE_OCULUS];

qboolean snd_delaytoobig;

qboolean snd_forceclear;

extern int sound_started;

WAVEFORMATEX snd_waveformat{ };
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

void SNDDMA_Callback(void* waveHeader)
{
    std::lock_guard<std::mutex> lock(Locks::SoundMutex);

    if (snd_waveout == NULL || shm == NULL || !sound_started)
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

void SNDDMA_timeCallback(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    SNDDMA_Callback((void*)dwUser);
}

void CALLBACK SNDDMA_waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    if (uMsg == WOM_DONE)
    {
        UINT delay = 1;
        do
        {
            auto result = timeSetEvent(delay, 0, (LPTIMECALLBACK)SNDDMA_timeCallback, dwParam1, TIME_ONESHOT);
            if (result <= 0)
            {
                delay++;
            }
            else
            {
                break;
            }
        } while (delay < 50);
        if (delay == 50)
        {
            snd_delaytoobig = true;
        }
    }
}

qboolean SNDDMA_Init(void)
{
    shm = &sn;
    shm->splitbuffer = 0;
    shm->samplebits = 32;
    shm->speed = 44100;
    shm->channels = 2;
    shm->samples = 65536;
    shm->samplepos = 0;
    shm->soundalive = true;
    shm->gamealive = true;
    shm->submission_chunk = (shm->samples >> 3);
    shm->buffer.resize(shm->samples * shm->samplebits / 8);

    snd_waveformat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    snd_waveformat.nChannels = shm->channels;
    snd_waveformat.nSamplesPerSec = shm->speed;
    snd_waveformat.nAvgBytesPerSec = shm->speed * shm->channels * (shm->samplebits / 8);
    snd_waveformat.nBlockAlign = shm->channels * (shm->samplebits / 8);
    snd_waveformat.wBitsPerSample = shm->samplebits;

    UINT deviceId = WAVE_MAPPER;

    auto numDevs = waveOutGetNumDevs();
    for (UINT i = 0; i < numDevs; i++)
    {
        DWORD endpointIdSize = 0;
        auto result = waveOutMessage((HWAVEOUT)IntToPtr(i), DRV_QUERYFUNCTIONINSTANCEIDSIZE, (DWORD_PTR)&endpointIdSize, NULL);
        if (result != MMSYSERR_NOERROR)
        {
            continue;
        }

        std::vector<wchar_t> endpointId(endpointIdSize);
        result = waveOutMessage((HWAVEOUT)IntToPtr(i), DRV_QUERYFUNCTIONINSTANCEID, (DWORD_PTR)endpointId.data(), endpointIdSize);
        if (result != MMSYSERR_NOERROR)
        {
            continue;
        }

        if (wcscmp(endpointId.data(), snd_audio_output_device_id) == 0)
        {
            deviceId = i;
        }
    }

    auto result = waveOutOpen(&snd_waveout, deviceId, &snd_waveformat, (DWORD_PTR)SNDDMA_waveOutProc, 0, CALLBACK_FUNCTION);
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
    if (snd_delaytoobig)
    {
        std::lock_guard<std::mutex> lock(Locks::SoundMutex);

        Sys_Printf("SNDDMA_Submit: delay for audio callback reached 50ms. Shutting down audio.\n");
        SNDDMA_ReleaseAll();

        snd_delaytoobig = false;
    }
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
}
