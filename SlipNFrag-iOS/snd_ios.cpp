//
//  snd_ios.cpp
//  SlipNFrag-iOS
//
//  Created by Heriberto Delgado on 9/13/20.
//  Copyright © 2020 Heriberto Delgado. All rights reserved.
//

#include "quakedef.h"
#include <AudioToolbox/AudioToolbox.h>
#include <pthread.h>

AudioQueueRef snd_audioqueue = NULL;

pthread_mutex_t snd_lock;

qboolean snd_forceclear;

void SNDDMA_Callback(void *userdata, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
    if (snd_audioqueue == nil)
    {
        return;
    }
    pthread_mutex_lock(&snd_lock);
    if (snd_forceclear)
    {
		std::fill(shm->buffer.begin(), shm->buffer.end(), 0);
        snd_forceclear = false;
    }
    memcpy(buffer->mAudioData, shm->buffer.data() + (shm->samplepos << 2), shm->samples >> 1);
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
    shm->samplepos += (shm->samples >> 3);
    if(shm->samplepos >= shm->samples)
    {
        shm->samplepos = 0;
    }
    pthread_mutex_unlock(&snd_lock);
}

qboolean SNDDMA_Init(void)
{
    pthread_mutex_init(&snd_lock, NULL);
    pthread_mutex_lock(&snd_lock);
	shm = &sn;
    shm->splitbuffer = 0;
    shm->samplebits = 32;
    shm->speed = 44100;
    shm->channels = 2;
    shm->samples = 65536;
    shm->samplepos = (shm->samples >> 3);
    shm->soundalive = true;
    shm->gamealive = true;
    shm->submission_chunk = 1;
    shm->buffer.resize(shm->samples * shm->samplebits/8);
    AudioStreamBasicDescription format { };
    format.mSampleRate = shm->speed;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    format.mBitsPerChannel = shm->samplebits;
    format.mChannelsPerFrame = shm->channels;
    format.mBytesPerFrame = shm->channels * shm->samplebits/8;
    format.mFramesPerPacket = 1;
    format.mBytesPerPacket = format.mBytesPerFrame * format.mFramesPerPacket;
    format.mReserved = 0;
    OSStatus status = AudioQueueNewOutput(&format, SNDDMA_Callback, NULL, NULL, 0, 0, &snd_audioqueue);
    if (status != 0)
    {
        return false;
    }
    AudioQueueBufferRef firstBuffer;
    status = AudioQueueAllocateBuffer(snd_audioqueue, shm->samples >> 1, &firstBuffer);
    if (status != 0)
    {
        AudioQueueDispose(snd_audioqueue, false);
        return false;
    }
    firstBuffer->mAudioDataByteSize = shm->samples >> 1;
    memset(firstBuffer->mAudioData, 0, firstBuffer->mAudioDataByteSize);
    status = AudioQueueEnqueueBuffer(snd_audioqueue, firstBuffer, 0, NULL);
    if (status != 0)
    {
        AudioQueueFreeBuffer(snd_audioqueue, firstBuffer);
        AudioQueueDispose(snd_audioqueue, false);
        return false;
    }
    AudioQueueBufferRef secondBuffer;
    status = AudioQueueAllocateBuffer(snd_audioqueue, shm->samples >> 1, &secondBuffer);
    if (status != 0)
    {
        AudioQueueFreeBuffer(snd_audioqueue, firstBuffer);
        AudioQueueDispose(snd_audioqueue, false);
        return false;
    }
    secondBuffer->mAudioDataByteSize = shm->samples >> 1;
    memset(secondBuffer->mAudioData, 0, secondBuffer->mAudioDataByteSize);
    status = AudioQueueEnqueueBuffer(snd_audioqueue, secondBuffer, 0, NULL);
    if (status != 0)
    {
        AudioQueueFreeBuffer(snd_audioqueue, secondBuffer);
        AudioQueueFreeBuffer(snd_audioqueue, firstBuffer);
        AudioQueueDispose(snd_audioqueue, false);
        return false;
    }
    status = AudioQueueStart(snd_audioqueue, NULL);
    if (status != 0)
    {
        return false;
    }
    pthread_mutex_unlock(&snd_lock);
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
	pthread_mutex_lock(&snd_lock);
	std::fill(shm->buffer.begin(), shm->buffer.end(), 0);
	pthread_mutex_unlock(&snd_lock);
}

void SNDDMA_Shutdown(void)
{
    pthread_mutex_lock(&snd_lock);
    if (snd_audioqueue != NULL)
    {
        AudioQueueStop(snd_audioqueue, false);
        AudioQueueDispose(snd_audioqueue, false);
    }
    snd_audioqueue = NULL;
    pthread_mutex_unlock(&snd_lock);
}
