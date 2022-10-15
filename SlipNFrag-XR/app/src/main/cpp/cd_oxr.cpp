#include "quakedef.h"
#include "stb_vorbis.c"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <dirent.h>

stb_vorbis* cdaudio_stream;
stb_vorbis_info cdaudio_info;

std::unordered_map<int, std::string> cdaudio_tracks;
std::vector<byte> cdaudio_trackContents;
std::vector<float> cdaudio_stagingBuffer;

SLObjectItf cdaudio_engineObject;
SLEngineItf cdaudio_engine;
SLObjectItf cdaudio_outputMixObject;
SLObjectItf cdaudio_playerObject;
SLPlayItf cdaudio_player;
SLAndroidSimpleBufferQueueItf cdaudio_bufferQueue;
int cdaudio_lastCopied;

static qboolean cdaudio_cdValid = false;
qboolean	cdaudio_playing = false;
static qboolean	cdaudio_wasPlaying = false;
static qboolean	cdaudio_initialized = false;
static qboolean	cdaudio_enabled = false;
static qboolean cdaudio_playLooping = false;
static float	cdaudio_cdvolume;
byte		cdaudio_playTrack;

void CDAudio_Callback(SLAndroidSimpleBufferQueueItf bufferQueue, void* context)
{
	if (!cdaudio_enabled)
		return;

	if (!cdaudio_playing)
		return;

	if (cdaudio_stream == nullptr)
		return;

	if (cdaudio_player == nullptr)
		return;

	cdaudio_lastCopied = stb_vorbis_get_samples_float_interleaved(cdaudio_stream, cdaudio_info.channels, cdaudio_stagingBuffer.data(), cdaudio_stagingBuffer.size() >> 1);
	if (cdaudio_lastCopied == 0 && cdaudio_playLooping)
	{
		stb_vorbis_seek_start(cdaudio_stream);
		cdaudio_lastCopied = stb_vorbis_get_samples_float_interleaved(cdaudio_stream, cdaudio_info.channels, cdaudio_stagingBuffer.data(), cdaudio_stagingBuffer.size() >> 1);
	}

	if (cdaudio_lastCopied == 0)
		return;

	(*cdaudio_bufferQueue)->Enqueue(cdaudio_bufferQueue,cdaudio_stagingBuffer.data(), cdaudio_lastCopied << 3);
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
	if (cdaudio_playerObject != nullptr)
	{
		(*cdaudio_playerObject)->Destroy(cdaudio_playerObject);
		cdaudio_player = nullptr;
		cdaudio_playerObject = nullptr;
	}
	if (cdaudio_outputMixObject != nullptr)
	{
		(*cdaudio_outputMixObject)->Destroy(cdaudio_outputMixObject);
		cdaudio_outputMixObject = nullptr;
	}
	if (cdaudio_engineObject != nullptr)
	{
		(*cdaudio_engineObject)->Destroy(cdaudio_engineObject);
		cdaudio_engine = nullptr;
		cdaudio_engineObject = nullptr;
	}
	if (cdaudio_stream != nullptr)
	{
		stb_vorbis_close(cdaudio_stream);
		cdaudio_stream = nullptr;
	}
}

qboolean CDAudio_Start (byte track)
{
	COM_LoadFile(cdaudio_tracks[track].c_str(), cdaudio_trackContents);
	if (cdaudio_trackContents.size() == 0)
	{
		Con_DPrintf("CDAudio: Empty file for track %u.\n", track);
		return false;
	}

	int error = VORBIS__no_error;
	cdaudio_stream = stb_vorbis_open_memory(cdaudio_trackContents.data(), cdaudio_trackContents.size() - 1, &error, nullptr);
	if (error != VORBIS__no_error)
	{
		Con_DPrintf("CDAudio: Error %i opening track %u.\n", error, track);
		cdaudio_stream = nullptr;
		return false;
	}
	cdaudio_info = stb_vorbis_get_info(cdaudio_stream);

	auto result = slCreateEngine(&cdaudio_engineObject, 0, nullptr, 0, nullptr, nullptr);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return false;
	}
	result = (*cdaudio_engineObject)->Realize(cdaudio_engineObject, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return false;
	}
	result = (*cdaudio_engineObject)->GetInterface(cdaudio_engineObject, SL_IID_ENGINE, &cdaudio_engine);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return false;
	}
	result = (*cdaudio_engine)->CreateOutputMix(cdaudio_engine, &cdaudio_outputMixObject, 0, nullptr, nullptr);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return false;
	}
	result = (*cdaudio_outputMixObject)->Realize(cdaudio_outputMixObject, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return false;
	}
	SLDataLocator_AndroidSimpleBufferQueue bufferQueueLocator;
	bufferQueueLocator.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
	bufferQueueLocator.numBuffers = 2;
	SLAndroidDataFormat_PCM_EX format;
	format.formatType = SL_ANDROID_DATAFORMAT_PCM_EX;
	format.numChannels = cdaudio_info.channels;
	format.sampleRate = cdaudio_info.sample_rate * 1000;
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
	outputMixLocator.outputMix = cdaudio_outputMixObject;
	SLDataSink sink { };
	sink.pLocator = &outputMixLocator;
	SLInterfaceID interfaceId = SL_IID_BUFFERQUEUE;
	SLboolean required = SL_BOOLEAN_TRUE;
	result = (*cdaudio_engine)->CreateAudioPlayer(cdaudio_engine, &cdaudio_playerObject, &dataSource, &sink, 1, &interfaceId, &required);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return false;
	}
	result = (*cdaudio_playerObject)->Realize(cdaudio_playerObject, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return false;
	}
	result = (*cdaudio_playerObject)->GetInterface(cdaudio_playerObject, SL_IID_PLAY, &cdaudio_player);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return false;
	}
	result = (*cdaudio_playerObject)->GetInterface(cdaudio_playerObject, SL_IID_BUFFERQUEUE, &cdaudio_bufferQueue);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return false;
	}
	result = (*cdaudio_bufferQueue)->RegisterCallback(cdaudio_bufferQueue, CDAudio_Callback, nullptr);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return false;
	}
	result = (*cdaudio_player)->SetPlayState(cdaudio_player, SL_PLAYSTATE_PLAYING);
	if (result != SL_RESULT_SUCCESS)
	{
		CDAudio_DisposeBuffers();
		return false;
	}
	for (auto samples = 1; samples < 1024 * 1024 * 1024; samples <<= 1)
	{
		if (samples >= cdaudio_info.sample_rate)
		{
			cdaudio_stagingBuffer.resize(samples >> 3);
			break;
		}
	}
	cdaudio_lastCopied = stb_vorbis_get_samples_float_interleaved(cdaudio_stream, cdaudio_info.channels, cdaudio_stagingBuffer.data(), cdaudio_stagingBuffer.size() >> 1);
	if (cdaudio_lastCopied == 0)
	{
		Con_DPrintf("CDAudio: Empty track %u.\n", track);
		(*cdaudio_player)->SetPlayState(cdaudio_player, SL_PLAYSTATE_STOPPED);
		CDAudio_DisposeBuffers();
		return false;
	}
	result = (*cdaudio_bufferQueue)->Enqueue(cdaudio_bufferQueue, cdaudio_stagingBuffer.data(), cdaudio_lastCopied << 3);
	if (result != SL_RESULT_SUCCESS)
	{
		(*cdaudio_player)->SetPlayState(cdaudio_player, SL_PLAYSTATE_STOPPED);
		CDAudio_DisposeBuffers();
		return false;
	}
	return true;
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
		cdaudio_playing = false;
		CDAudio_DisposeBuffers();
	}

	auto entry = cdaudio_tracks.find(track);
	if (entry == cdaudio_tracks.end())
	{
		Con_Printf("CDAudio: Track %u not found.\n", track);
		return;
	}

	if (!CDAudio_Start(track))
	{
		return;
	}

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

	if (cdaudio_player != nullptr)
	{
		(*cdaudio_player)->SetPlayState(cdaudio_player, SL_PLAYSTATE_STOPPED);
	}

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

	if (cdaudio_stream == nullptr)
		return;

	if (cdaudio_player == nullptr)
		return;

	auto result = (*cdaudio_player)->SetPlayState(cdaudio_player, SL_PLAYSTATE_PAUSED);
	if (result != SL_RESULT_SUCCESS)
	{
		Con_DPrintf ("Audio track pause failed (%i)", result);
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

	if (cdaudio_stream == nullptr)
		return;

	if (cdaudio_player == nullptr)
		return;

	auto result = (*cdaudio_player)->SetPlayState(cdaudio_player, SL_PLAYSTATE_PLAYING);
	if (result != SL_RESULT_SUCCESS)
	{
		Con_DPrintf ("Audio track resume failed (%i)", result);
	}

	if (!cdaudio_wasPlaying)
		return;

	cdaudio_playing = true;
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
