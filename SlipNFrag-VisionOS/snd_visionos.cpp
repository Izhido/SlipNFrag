//
//  snd_visionos.cpp
//  SlipNFrag-VisionOS
//
//  Created by Heriberto Delgado on 22/7/23.
//  Copyright © 2023 Heriberto Delgado. All rights reserved.
//

#include "quakedef.h"
#include <AudioToolbox/AudioToolbox.h>
#include "Locks.h"

AudioQueueRef snd_audioqueue = NULL;

qboolean snd_forceclear;

void SNDDMA_Callback(void *userdata, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
	if (shm == NULL || snd_audioqueue == nil)
	{
		return;
	}

	std::lock_guard<std::mutex> lock(Locks::SoundMutex);
	
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
}

qboolean SNDDMA_Init(void)
{
	std::lock_guard<std::mutex> lock(Locks::SoundMutex);
	
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
	
	if (snd_audioqueue != NULL)
	{
		AudioQueueStop(snd_audioqueue, false);
		AudioQueueDispose(snd_audioqueue, false);
	}
	snd_audioqueue = NULL;
}
