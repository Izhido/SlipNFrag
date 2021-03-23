#include "quakedef.h"
#include "stb_vorbis.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <dirent.h>

stb_vorbis* cdaudio_stream;
stb_vorbis_info cdaudio_info;

std::unordered_map<int, std::string> cdaudioTracks;
std::vector<byte> cdaudioTrackContents;
std::vector<short> cdaudioStagingBuffer;

SLObjectItf cdaudioEngineObject;
SLEngineItf cdaudioEngine;
SLObjectItf cdaudioOutputMixObject;
SLObjectItf cdaudioPlayerObject;
SLPlayItf cdaudioPlayer;
SLAndroidSimpleBufferQueueItf cdaudioBufferQueue;
int cdaudioLastCopied;

static qboolean cdValid = false;
static qboolean	playing = false;
static qboolean	wasPlaying = false;
static qboolean	initialized = false;
static qboolean	enabled = false;
static qboolean playLooping = false;
static float	cdvolume;
static byte		playTrack;

void CDAudio_Callback(SLAndroidSimpleBufferQueueItf bufferQueue, void* context)
{
	if (!enabled)
		return;

	if (!playing)
		return;

	if (cdaudio_stream == nullptr)
		return;

	if (cdaudioPlayer == nullptr)
		return;

	cdaudioLastCopied = stb_vorbis_get_samples_short_interleaved(cdaudio_stream, cdaudio_info.channels, cdaudioStagingBuffer.data(), cdaudioStagingBuffer.size() >> 1);
	if (cdaudioLastCopied == 0 && playLooping)
	{
		stb_vorbis_seek_start(cdaudio_stream);
		cdaudioLastCopied = stb_vorbis_get_samples_short_interleaved(cdaudio_stream, cdaudio_info.channels, cdaudioStagingBuffer.data(), cdaudioStagingBuffer.size() >> 1);
	}

	if (cdaudioLastCopied == 0)
		return;

	(*cdaudioBufferQueue)->Enqueue(cdaudioBufferQueue, cdaudioStagingBuffer.data(), cdaudioLastCopied << 2);
}

void CDAudio_FindInPath (const char *path, const char *directory, const char *prefix, const char *extension, std::vector<std::string>& result)
{
	auto extension_len = strlen(extension);
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
				auto in_str = strcasestr (file, prefix);
				if (in_str == file)
				{
					in_str = strcasestr (file + strlen(file) - extension_len, extension);
					if (in_str != nullptr)
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
	cdValid = false;

	cdaudioTracks.clear();

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
			cdaudioTracks.insert({ number, track });
		}
	}

	cdValid = true;

	return 0;
}

void CDAudio_DisposeBuffers()
{
	if (cdaudioPlayerObject != nullptr)
	{
		(*cdaudioPlayerObject)->Destroy(cdaudioPlayerObject);
		cdaudioPlayer = nullptr;
		cdaudioPlayerObject = nullptr;
	}
	if (cdaudioOutputMixObject != nullptr)
	{
		(*cdaudioOutputMixObject)->Destroy(cdaudioOutputMixObject);
		cdaudioOutputMixObject = nullptr;
	}
	if (cdaudioEngineObject != nullptr)
	{
		(*cdaudioEngineObject)->Destroy(cdaudioEngineObject);
		cdaudioEngine = nullptr;
		cdaudioEngineObject = nullptr;
	}
	if (cdaudio_stream != nullptr)
	{
		stb_vorbis_close(cdaudio_stream);
		cdaudio_stream = nullptr;
	}
}

void CDAudio_Play(byte track, qboolean looping)
{
	if (!enabled)
		return;

	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
			return;
	}

	if (playing)
	{
		if (playTrack == track)
			return;
		CDAudio_DisposeBuffers();
	}

	auto entry = cdaudioTracks.find(track);
	if (entry == cdaudioTracks.end())
	{
		Con_Printf("CDAudio: Track %u not found.\n", track);
		return;
	}

	COM_LoadFile(cdaudioTracks[track].c_str(), cdaudioTrackContents);
	if (cdaudioTrackContents.size() == 0)
	{
		Con_DPrintf("CDAudio: Empty file for track %u.\n", track);
		return;
	}

	int error = VORBIS__no_error;
	cdaudio_stream = stb_vorbis_open_memory(cdaudioTrackContents.data(), cdaudioTrackContents.size() - 1, &error, nullptr);
	if (error != VORBIS__no_error)
	{
		Con_DPrintf("CDAudio: Error %i opening track %u.\n", error, track);
		cdaudio_stream = nullptr;
		return;
	}
	cdaudio_info = stb_vorbis_get_info(cdaudio_stream);
	auto result = slCreateEngine(&cdaudioEngineObject, 0, nullptr, 0, nullptr, nullptr);
	if (result != SL_RESULT_SUCCESS)
	{
		return;
	}
	result = (*cdaudioEngineObject)->Realize(cdaudioEngineObject, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	result = (*cdaudioEngineObject)->GetInterface(cdaudioEngineObject, SL_IID_ENGINE, &cdaudioEngine);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	result = (*cdaudioEngine)->CreateOutputMix(cdaudioEngine, &cdaudioOutputMixObject, 0, nullptr, nullptr);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	result = (*cdaudioOutputMixObject)->Realize(cdaudioOutputMixObject, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	SLDataLocator_AndroidSimpleBufferQueue bufferQueueLocator;
	bufferQueueLocator.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
	bufferQueueLocator.numBuffers = 2;
	SLDataFormat_PCM format;
	format.formatType = SL_DATAFORMAT_PCM;
	format.numChannels = cdaudio_info.channels;
	format.samplesPerSec = cdaudio_info.sample_rate * 1000;
	format.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	format.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
	format.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
	format.endianness = SL_BYTEORDER_LITTLEENDIAN;
	SLDataSource dataSource;
	dataSource.pLocator = &bufferQueueLocator;
	dataSource.pFormat = &format;
	SLDataLocator_OutputMix outputMixLocator;
	outputMixLocator.locatorType = SL_DATALOCATOR_OUTPUTMIX;
	outputMixLocator.outputMix = cdaudioOutputMixObject;
	SLDataSink sink { };
	sink.pLocator = &outputMixLocator;
	SLInterfaceID interfaceId = SL_IID_BUFFERQUEUE;
	SLboolean required = SL_BOOLEAN_TRUE;
	result = (*cdaudioEngine)->CreateAudioPlayer(cdaudioEngine, &cdaudioPlayerObject, &dataSource, &sink, 1, &interfaceId, &required);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	result = (*cdaudioPlayerObject)->Realize(cdaudioPlayerObject, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	result = (*cdaudioPlayerObject)->GetInterface(cdaudioPlayerObject, SL_IID_PLAY, &cdaudioPlayer);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	result = (*cdaudioPlayerObject)->GetInterface(cdaudioPlayerObject, SL_IID_BUFFERQUEUE, &cdaudioBufferQueue);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	result = (*cdaudioBufferQueue)->RegisterCallback(cdaudioBufferQueue, CDAudio_Callback, nullptr);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	result = (*cdaudioPlayer)->SetPlayState(cdaudioPlayer, SL_PLAYSTATE_PLAYING);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	for (auto samples = 1; samples < 1024 * 1024 * 1024; samples <<= 1)
	{
		if (samples >= cdaudio_info.sample_rate)
		{
			cdaudioStagingBuffer.resize(samples >> 3);
			break;
		}
	}
	cdaudioLastCopied = stb_vorbis_get_samples_short_interleaved(cdaudio_stream, cdaudio_info.channels, cdaudioStagingBuffer.data(), cdaudioStagingBuffer.size() >> 1);
	if (cdaudioLastCopied == 0)
	{
		Con_DPrintf("CDAudio: Empty track %u.\n", track);
		(*cdaudioPlayer)->SetPlayState(cdaudioPlayer, SL_PLAYSTATE_STOPPED);
		CDAudio_DisposeBuffers();
		return;
	}
	result = (*cdaudioBufferQueue)->Enqueue(cdaudioBufferQueue, cdaudioStagingBuffer.data(), cdaudioLastCopied << 2);
	if (result != SL_RESULT_SUCCESS)
	{
		(*cdaudioPlayer)->SetPlayState(cdaudioPlayer, SL_PLAYSTATE_STOPPED);
		CDAudio_DisposeBuffers();
		return;
	}

	playLooping = looping;
	playTrack = track;
	playing = true;

	if (cdvolume == 0.0)
		CDAudio_Pause ();
}

void CDAudio_Stop(void)
{
	if (!enabled)
		return;

	if (!playing)
		return;

	if (cdaudioPlayer != nullptr)
	{
		(*cdaudioPlayer)->SetPlayState(cdaudioPlayer, SL_PLAYSTATE_STOPPED);
	}

	CDAudio_DisposeBuffers();

	wasPlaying = false;
	playing = false;
}

void CDAudio_Pause(void)
{
	if (!enabled)
		return;

	if (!playing)
		return;

	auto result = (*cdaudioPlayer)->SetPlayState(cdaudioPlayer, SL_PLAYSTATE_PAUSED);
	if (result != SL_RESULT_SUCCESS)
	{
		Con_DPrintf ("Audio track pause failed (%i)", result);
	}

	wasPlaying = playing;
	playing = false;
}

void CDAudio_Resume(void)
{
	if (!enabled)
		return;

	if (!cdValid)
		return;

	auto result = (*cdaudioPlayer)->SetPlayState(cdaudioPlayer, SL_PLAYSTATE_PLAYING);
	if (result != SL_RESULT_SUCCESS)
	{
		Con_DPrintf ("Audio track resume failed (%i)", result);
	}

	if (!wasPlaying)
		return;

	playing = true;
}


static void CD_f (void)
{
	const char *command;
	int		ret;
	int		n;
	int		startAddress;

	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv (1);

	if (Q_strcasecmp(command, "on") == 0)
	{
		enabled = true;
		return;
	}

	if (Q_strcasecmp(command, "off") == 0)
	{
		if (playing)
			CDAudio_Stop();
		enabled = false;
		return;
	}

	if (Q_strcasecmp(command, "reset") == 0)
	{
		enabled = true;
		if (playing)
			CDAudio_Stop();
		CDAudio_GetAudioDiskInfo();
		return;
	}

	if (!cdValid)
	{
		CDAudio_GetAudioDiskInfo();
		if (!cdValid)
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
		if (playing)
			CDAudio_Stop();
		cdValid = false;
		return;
	}

	if (Q_strcasecmp(command, "info") == 0)
	{
		Con_Printf("%u tracks\n", cdaudioTracks.size());
		if (playing)
			Con_Printf("Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		else if (wasPlaying)
			Con_Printf("Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		Con_Printf("Volume is %f\n", cdvolume);
		return;
	}
}


void CDAudio_Update(void)
{
	if (!enabled)
		return;

	if (bgmvolume.value != cdvolume)
	{
		if (cdvolume)
		{
			Cvar_SetValue ("bgmvolume", 0.0);
			cdvolume = bgmvolume.value;
			CDAudio_Pause ();
		}
		else
		{
			Cvar_SetValue ("bgmvolume", 1.0);
			cdvolume = bgmvolume.value;
			CDAudio_Resume ();
		}
	}

	if (cdaudioLastCopied == 0)
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

	initialized = true;
	enabled = true;

	if (CDAudio_GetAudioDiskInfo())
	{
		Con_Printf("CDAudio_Init: No CD in player.\n");
		cdValid = false;
	}

	Cmd_AddCommand ("cd", CD_f);

	Con_Printf("CD Audio Initialized\n");

	return 0;
}

void CDAudio_Shutdown(void)
{
	if (!initialized)
		return;
	CDAudio_Stop();
}