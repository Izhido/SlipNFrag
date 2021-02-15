//
//  snd_macos.cpp
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 6/23/19.
//  Copyright © 2019 Heriberto Delgado. All rights reserved.
//

#include "quakedef.h"
#include <AudioToolbox/AudioToolbox.h>
#include <pthread.h>

AudioQueueRef snd_audioqueue = NULL;

pthread_mutex_t snd_lock;

volatile int snd_current_sample_pos = 0;

void SNDDMA_Callback(void *userdata, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
    pthread_mutex_lock(&snd_lock);
    if (snd_audioqueue == nil)
    {
        return;
    }
    memcpy(buffer->mAudioData, shm->buffer.data() + (snd_current_sample_pos << 1), shm->samples >> 2);
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
    snd_current_sample_pos += (shm->samples >> 3);
    if(snd_current_sample_pos >= shm->samples)
    {
        snd_current_sample_pos = 0;
    }
    pthread_mutex_unlock(&snd_lock);
}

qboolean SNDDMA_Init(void)
{
    pthread_mutex_init(&snd_lock, NULL);
    pthread_mutex_lock(&snd_lock);
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
    shm->buffer.resize(shm->samples * shm->samplebits/8);
    AudioStreamBasicDescription format;
    format.mSampleRate = shm->speed;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
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
    AudioQueueBufferRef buffer;
    status = AudioQueueAllocateBuffer(snd_audioqueue, shm->samples >> 2, &buffer);
    if (status != 0)
    {
        return false;
    }
    buffer->mAudioDataByteSize = shm->samples >> 2;
    status = AudioQueueEnqueueBuffer(snd_audioqueue, buffer, 0, NULL);
    if (status != 0)
    {
        AudioQueueFreeBuffer(snd_audioqueue, buffer);
        return false;
    }
    status = AudioQueueStart(snd_audioqueue, NULL);
    if (status != 0)
    {
        return false;
    }
    snd_current_sample_pos = shm->samples >> 1;
    pthread_mutex_unlock(&snd_lock);
    return true;
}

int SNDDMA_GetDMAPos(void)
{
    shm->samplepos = snd_current_sample_pos;
    return shm->samplepos;
}

void SNDDMA_Submit(void)
{
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
