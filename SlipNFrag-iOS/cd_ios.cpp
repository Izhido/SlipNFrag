//
//  cd_ios.cpp
//  SlipNFrag-iOS
//
//  Created by Heriberto Delgado on 3/24/21.
//  Copyright © 2021 Heriberto Delgado. All rights reserved.
//

#include "quakedef.h"
#include "stb_vorbis.c"
#include <AudioToolbox/AudioToolbox.h>
#include <pthread.h>
#include <dirent.h>

stb_vorbis* cdaudio_stream;
stb_vorbis_info cdaudio_info;

std::unordered_map<int, std::string> cdaudio_tracks;
std::vector<byte> cdaudio_trackContents;
std::vector<short> cdaudio_stagingBuffer;

AudioQueueRef cdaudio_audioqueue = NULL;
AudioQueueBufferRef cdaudio_firstBuffer = NULL;
AudioQueueBufferRef cdaudio_secondBuffer = NULL;

pthread_mutex_t cdaudio_lock;

int cdaudio_lastCopied;

static qboolean cdaudio_cdValid = false;
static qboolean	cdaudio_playing = false;
static qboolean	cdaudio_wasPlaying = false;
static qboolean	cdaudio_initialized = false;
static qboolean	cdaudio_enabled = false;
static qboolean cdaudio_playLooping = false;
static float	cdaudio_cdvolume;
static byte		cdaudio_playTrack;

void CDAudio_Callback(void *userdata, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
	if (!cdaudio_enabled)
		return;

	if (!cdaudio_playing)
		return;

	if (cdaudio_stream == nullptr)
		return;

	if (cdaudio_audioqueue == nullptr)
		return;

	cdaudio_lastCopied = stb_vorbis_get_samples_short_interleaved(cdaudio_stream, cdaudio_info.channels, cdaudio_stagingBuffer.data(), cdaudio_stagingBuffer.size());
	if (cdaudio_lastCopied == 0 && cdaudio_playLooping)
	{
		stb_vorbis_seek_start(cdaudio_stream);
        cdaudio_lastCopied = stb_vorbis_get_samples_short_interleaved(cdaudio_stream, cdaudio_info.channels, cdaudio_stagingBuffer.data(), cdaudio_stagingBuffer.size());
	}

	if (cdaudio_lastCopied == 0)
		return;

    pthread_mutex_lock(&cdaudio_lock);
    memcpy(buffer->mAudioData, cdaudio_stagingBuffer.data(), cdaudio_lastCopied << 2);
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
    pthread_mutex_unlock(&cdaudio_lock);
}

void CDAudio_FindInPath (const char *path, const char *directory, const char *prefix, const char *extension, std::vector<std::string>& result)
{
	auto prefix_len = Q_strlen(prefix);
	auto extension_len = Q_strlen(extension);
	auto to_search = std::string(path) + "/" + directory;
	auto dir = opendir (to_search.c_str());
	if (dir != nullptr)
	{
		struct dirent* entry;
		while ((entry = readdir (dir)) != nullptr)
		{
			if (entry->d_type == DT_REG)
			{
				char* file = entry->d_name;
				auto match = Q_strncasecmp (file, prefix, prefix_len);
				if (match == 0)
				{
					match = Q_strncasecmp (file + strlen(file) - extension_len, extension, extension_len);
					if (match == 0)
					{
						result.push_back(std::string(directory) + file);
					}
				}
			}
		}
		closedir (dir);
	}
}

static int CDAudio_GetAudioDiskInfo(void)
{
	cdaudio_cdValid = false;

	cdaudio_tracks.clear();

	std::vector<std::string> tracks;
	COM_FindAllFiles("music/", "track", ".ogg", &CDAudio_FindInPath, tracks);

	if (tracks.size() == 0)
	{
		Con_DPrintf("CDAudio: no music tracks\n");
		return -1;
	}

	auto prefix_len = strlen("music/track");
	for (auto& track : tracks)
	{
		auto number = atoi(track.c_str() + prefix_len);
		if (number > 1)
		{
			cdaudio_tracks.insert({ number, track });
		}
	}

    cdaudio_cdValid = true;

	return 0;
}

void CDAudio_DisposeBuffers()
{
    pthread_mutex_lock(&cdaudio_lock);
    if (cdaudio_audioqueue != NULL)
    {
        AudioQueueStop(cdaudio_audioqueue, false);
        if (cdaudio_secondBuffer != NULL)
        {
            AudioQueueFreeBuffer(cdaudio_audioqueue, cdaudio_secondBuffer);
            cdaudio_secondBuffer = NULL;
        }
        if (cdaudio_firstBuffer != NULL)
        {
            AudioQueueFreeBuffer(cdaudio_audioqueue, cdaudio_firstBuffer);
        }
        AudioQueueDispose(cdaudio_audioqueue, false);
        cdaudio_audioqueue = NULL;
    }
    pthread_mutex_unlock(&cdaudio_lock);
	if (cdaudio_stream != nullptr)
	{
		stb_vorbis_close(cdaudio_stream);
		cdaudio_stream = nullptr;
	}
}

void CDAudio_Play(byte track, qboolean looping)
{
	if (!cdaudio_enabled)
		return;

	if (!cdaudio_cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdaudio_cdValid)
			return;
	}

	if (cdaudio_playing)
	{
		if (cdaudio_playTrack == track)
			return;
		CDAudio_DisposeBuffers();
	}

	auto entry = cdaudio_tracks.find(track);
	if (entry == cdaudio_tracks.end())
	{
		Con_Printf("CDAudio: Track %u not found.\n", track);
		return;
	}

	COM_LoadFile(cdaudio_tracks[track].c_str(), cdaudio_trackContents);
	if (cdaudio_trackContents.size() == 0)
	{
		Con_DPrintf("CDAudio: Empty file for track %u.\n", track);
		return;
	}

	int error = VORBIS__no_error;
	cdaudio_stream = stb_vorbis_open_memory(cdaudio_trackContents.data(), cdaudio_trackContents.size() - 1, &error, nullptr);
	if (error != VORBIS__no_error)
	{
		Con_DPrintf("CDAudio: Error %i opening track %u.\n", error, track);
		cdaudio_stream = nullptr;
		return;
	}
	cdaudio_info = stb_vorbis_get_info(cdaudio_stream);
    pthread_mutex_lock(&cdaudio_lock);
    AudioStreamBasicDescription format { };
    format.mSampleRate = cdaudio_info.sample_rate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    format.mBitsPerChannel = 16;
    format.mChannelsPerFrame = cdaudio_info.channels;
    format.mBytesPerFrame = cdaudio_info.channels * 16/8;
    format.mFramesPerPacket = 1;
    format.mBytesPerPacket = format.mBytesPerFrame * format.mFramesPerPacket;
    format.mReserved = 0;
    OSStatus status = AudioQueueNewOutput(&format, CDAudio_Callback, NULL, NULL, 0, 0, &cdaudio_audioqueue);
    if (status != 0)
    {
        CDAudio_DisposeBuffers();
        return;
    }
    int samples;
    for (samples = 1; samples < 1024 * 1024 * 1024; samples <<= 1)
    {
        if (samples >= cdaudio_info.sample_rate)
        {
            break;
        }
    }
    status = AudioQueueAllocateBuffer(cdaudio_audioqueue, samples >> 2, &cdaudio_firstBuffer);
    if (status != 0)
    {
        CDAudio_DisposeBuffers();
        return;
    }
    cdaudio_firstBuffer->mAudioDataByteSize = samples >> 2;
    memset(cdaudio_firstBuffer->mAudioData, 0, cdaudio_firstBuffer->mAudioDataByteSize);
    status = AudioQueueEnqueueBuffer(cdaudio_audioqueue, cdaudio_firstBuffer, 0, NULL);
    if (status != 0)
    {
        CDAudio_DisposeBuffers();
        return;
    }
    status = AudioQueueAllocateBuffer(cdaudio_audioqueue, samples >> 2, &cdaudio_secondBuffer);
    if (status != 0)
    {
        CDAudio_DisposeBuffers();
        return;
    }
    cdaudio_secondBuffer->mAudioDataByteSize = samples >> 2;
    memset(cdaudio_secondBuffer->mAudioData, 0, cdaudio_secondBuffer->mAudioDataByteSize);
    status = AudioQueueEnqueueBuffer(cdaudio_audioqueue, cdaudio_secondBuffer, 0, NULL);
    if (status != 0)
    {
        CDAudio_DisposeBuffers();
        return;
    }
    status = AudioQueueStart(cdaudio_audioqueue, NULL);
    if (status != 0)
    {
        CDAudio_DisposeBuffers();
        return;
    }
    pthread_mutex_unlock(&cdaudio_lock);
    cdaudio_stagingBuffer.resize(samples >> 3);
    cdaudio_lastCopied = -1;

	cdaudio_playLooping = looping;
    cdaudio_playTrack = track;
    cdaudio_playing = true;

	if (cdaudio_cdvolume == 0.0)
		CDAudio_Pause ();
}

void CDAudio_Stop(void)
{
	if (!cdaudio_enabled)
		return;

	if (!cdaudio_playing)
		return;

	CDAudio_DisposeBuffers();

    cdaudio_wasPlaying = false;
    cdaudio_playing = false;
}

void CDAudio_Pause(void)
{
	if (!cdaudio_enabled)
		return;

	if (!cdaudio_playing)
		return;

    OSStatus status = AudioQueuePause(cdaudio_audioqueue);
    if (status != 0)
    {
		Con_DPrintf ("Audio track pause failed (%i)", status);
	}

	cdaudio_wasPlaying = cdaudio_playing;
	cdaudio_playing = false;
}

void CDAudio_Resume(void)
{
	if (!cdaudio_enabled)
		return;

	if (!cdaudio_cdValid)
		return;

    OSStatus status = AudioQueueStart(cdaudio_audioqueue, NULL);
    if (status != 0)
	{
		Con_DPrintf ("Audio track resume failed (%i)", status);
	}

	if (!cdaudio_wasPlaying)
		return;

    cdaudio_playing = true;
}


static void CD_f (void)
{
	const char *command;

	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv (1);

	if (Q_strcasecmp(command, "on") == 0)
	{
        cdaudio_enabled = true;
		return;
	}

	if (Q_strcasecmp(command, "off") == 0)
	{
		if (cdaudio_playing)
			CDAudio_Stop();
        cdaudio_enabled = false;
		return;
	}

	if (Q_strcasecmp(command, "reset") == 0)
	{
        cdaudio_enabled = true;
		if (cdaudio_playing)
			CDAudio_Stop();
		CDAudio_GetAudioDiskInfo();
		return;
	}

	if (!cdaudio_cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdaudio_cdValid)
		{
			Con_Printf("No CD in player.\n");
			return;
		}
	}

	if (Q_strcasecmp(command, "play") == 0)
	{
		CDAudio_Play((byte)Q_atoi(Cmd_Argv (2)), false);
		return;
	}

	if (Q_strcasecmp(command, "loop") == 0)
	{
		CDAudio_Play((byte)Q_atoi(Cmd_Argv (2)), true);
		return;
	}

	if (Q_strcasecmp(command, "stop") == 0)
	{
		CDAudio_Stop();
		return;
	}

	if (Q_strcasecmp(command, "pause") == 0)
	{
		CDAudio_Pause();
		return;
	}

	if (Q_strcasecmp(command, "resume") == 0)
	{
		CDAudio_Resume();
		return;
	}

	if (Q_strcasecmp(command, "eject") == 0)
	{
		if (cdaudio_playing)
			CDAudio_Stop();
        cdaudio_cdValid = false;
		return;
	}

	if (Q_strcasecmp(command, "info") == 0)
	{
		Con_Printf("%u tracks\n", cdaudio_tracks.size());
		if (cdaudio_playing)
			Con_Printf("Currently %s track %u\n", cdaudio_playLooping ? "looping" : "playing", cdaudio_playTrack);
		else if (cdaudio_wasPlaying)
			Con_Printf("Paused %s track %u\n", cdaudio_playLooping ? "looping" : "playing", cdaudio_playTrack);
		Con_Printf("Volume is %f\n", cdaudio_cdvolume);
		return;
	}
}


void CDAudio_Update(void)
{
	if (!cdaudio_enabled)
		return;

	if (bgmvolume.value != cdaudio_cdvolume)
	{
		if (cdaudio_cdvolume)
		{
			Cvar_SetValue ("bgmvolume", 0.0);
            cdaudio_cdvolume = bgmvolume.value;
			CDAudio_Pause ();
		}
		else
		{
			Cvar_SetValue ("bgmvolume", 1.0);
            cdaudio_cdvolume = bgmvolume.value;
			CDAudio_Resume ();
		}
	}

	if (cdaudio_lastCopied == 0)
	{
		CDAudio_Stop();
	}
}

int CDAudio_Init(void)
{
	if (cls.state == ca_dedicated)
		return -1;

	if (COM_CheckParm("-nocdaudio"))
		return -1;

    cdaudio_initialized = true;
    cdaudio_enabled = true;

    pthread_mutex_init(&cdaudio_lock, NULL);

    if (CDAudio_GetAudioDiskInfo())
	{
		Con_Printf("CDAudio_Init: No CD in player.\n");
        cdaudio_cdValid = false;
	}

	Cmd_AddCommand ("cd", CD_f);

	Con_Printf("CD Audio Initialized\n");

	return 0;
}

void CDAudio_Shutdown(void)
{
	if (!cdaudio_initialized)
		return;
	CDAudio_Stop();
}
