#include "quakedef.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "Locks.h"

SLObjectItf audioEngineObject;
SLEngineItf audioEngine;
SLObjectItf audioOutputMixObject;
SLObjectItf audioPlayerObject;
SLPlayItf audioPlayer;
SLAndroidSimpleBufferQueueItf audioBufferQueue;

qboolean snd_forceclear;

extern int sound_started;

void SNDDMA_DisposeBuffers();

void SNDDMA_Callback(SLAndroidSimpleBufferQueueItf /*bufferQueue*/, void* /*context*/)
{
    std::lock_guard<std::mutex> lock(Locks::SoundMutex);

    if (audioPlayer == nullptr)
    {
        return;
    }
    if (snd_forceclear)
    {
        S_ClearBuffer();
        snd_forceclear = false;
    }
    auto result = (*audioBufferQueue)->Enqueue(audioBufferQueue, shm->buffer.data() + (shm->samplepos << 2), shm->samples >> 1);
    if (result != SL_RESULT_SUCCESS)
    {
        SNDDMA_DisposeBuffers();
        Con_Printf("SNDDMA_Callback failed.\n");
        sound_started = 0;
    }
    shm->samplepos += (shm->samples >> 3);
    if(shm->samplepos >= shm->samples)
    {
        shm->samplepos = 0;
    }
}

void SNDDMA_DisposeBuffers()
{
    if (audioPlayerObject != nullptr)
    {
        (*audioPlayerObject)->Destroy(audioPlayerObject);
        audioPlayerObject = nullptr;
    }
    if (audioOutputMixObject != nullptr)
    {
        (*audioOutputMixObject)->Destroy(audioOutputMixObject);
        audioOutputMixObject = nullptr;
    }
    if (audioEngineObject != nullptr)
    {
        (*audioEngineObject)->Destroy(audioEngineObject);
        audioEngineObject = nullptr;
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
    shm->samplepos = (shm->samples >> 3);
    shm->soundalive = true;
    shm->gamealive = true;
    shm->submission_chunk = 1;
    shm->buffer.resize(shm->samples * sizeof(float));
    auto result = slCreateEngine(&audioEngineObject, 0, nullptr, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS)
    {
        return false;
    }
    result = (*audioEngineObject)->Realize(audioEngineObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS)
    {
        SNDDMA_DisposeBuffers();
        return false;
    }
    result = (*audioEngineObject)->GetInterface(audioEngineObject, SL_IID_ENGINE, &audioEngine);
    if (result != SL_RESULT_SUCCESS)
    {
        SNDDMA_DisposeBuffers();
        return false;
    }
    result = (*audioEngine)->CreateOutputMix(audioEngine, &audioOutputMixObject, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS)
    {
        SNDDMA_DisposeBuffers();
        return false;
    }
    result = (*audioOutputMixObject)->Realize(audioOutputMixObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS)
    {
        SNDDMA_DisposeBuffers();
        return false;
    }
    SLDataLocator_AndroidSimpleBufferQueue bufferQueueLocator;
    bufferQueueLocator.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    bufferQueueLocator.numBuffers = 2;
    SLAndroidDataFormat_PCM_EX format;
    format.formatType = SL_ANDROID_DATAFORMAT_PCM_EX;
    format.numChannels = shm->channels;
    format.sampleRate = SL_SAMPLINGRATE_44_1;
    format.bitsPerSample = 32;
    format.containerSize = 32;
    format.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    format.endianness = SL_BYTEORDER_LITTLEENDIAN;
    format.representation = SL_ANDROID_PCM_REPRESENTATION_FLOAT;
    SLDataSource dataSource;
    dataSource.pLocator = &bufferQueueLocator;
    dataSource.pFormat = &format;
    SLDataLocator_OutputMix outputMixLocator;
    outputMixLocator.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    outputMixLocator.outputMix = audioOutputMixObject;
    SLDataSink sink { };
    sink.pLocator = &outputMixLocator;
    SLInterfaceID interfaceId = SL_IID_BUFFERQUEUE;
    SLboolean required = SL_BOOLEAN_TRUE;
    result = (*audioEngine)->CreateAudioPlayer(audioEngine, &audioPlayerObject, &dataSource, &sink, 1, &interfaceId, &required);
    if (result != SL_RESULT_SUCCESS)
    {
        SNDDMA_DisposeBuffers();
        return false;
    }
    result = (*audioPlayerObject)->Realize(audioPlayerObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS)
    {
        SNDDMA_DisposeBuffers();
        return false;
    }
    result = (*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_PLAY, &audioPlayer);
    if (result != SL_RESULT_SUCCESS)
    {
        SNDDMA_DisposeBuffers();
        return false;
    }
    result = (*audioPlayerObject)->GetInterface(audioPlayerObject, SL_IID_BUFFERQUEUE, &audioBufferQueue);
    if (result != SL_RESULT_SUCCESS)
    {
        SNDDMA_DisposeBuffers();
        return false;
    }
    result = (*audioBufferQueue)->RegisterCallback(audioBufferQueue, SNDDMA_Callback, nullptr);
    if (result != SL_RESULT_SUCCESS)
    {
        SNDDMA_DisposeBuffers();
        return false;
    }
    result = (*audioPlayer)->SetPlayState(audioPlayer, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS)
    {
        SNDDMA_DisposeBuffers();
        return false;
    }
    result = (*audioBufferQueue)->Enqueue(audioBufferQueue, shm->buffer.data(), shm->samples >> 1);
    if (result != SL_RESULT_SUCCESS)
    {
        SNDDMA_DisposeBuffers();
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

void SNDDMA_Shutdown(void)
{
    std::lock_guard<std::mutex> lock(Locks::SoundMutex);

    if (audioPlayerObject != nullptr)
    {
        (*audioPlayer)->SetPlayState(audioPlayer, SL_PLAYSTATE_STOPPED);
        (*audioPlayerObject)->Destroy(audioPlayerObject);
        audioPlayer = nullptr;
        audioPlayerObject = nullptr;
    }
    if (audioOutputMixObject != nullptr)
    {
        (*audioOutputMixObject)->Destroy(audioOutputMixObject);
        audioOutputMixObject = nullptr;
    }
    if (audioEngineObject != nullptr)
    {
        (*audioEngineObject)->Destroy(audioEngineObject);
        audioEngine = nullptr;
        audioEngineObject = nullptr;
    }
}
