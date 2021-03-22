#include "quakedef.h"
#include "stb_vorbis.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <dirent.h>

stb_vorbis* cdaudio_stream;
stb_vorbis_info cdaudio_info;

std::vector<byte> cdaudioTrackContents;
std::vector<short> cdaudioStagingBuffer;

SLObjectItf cdaudioEngineObject;
SLEngineItf cdaudioEngine;
SLObjectItf cdaudioOutputMixObject;
SLObjectItf cdaudioPlayerObject;
SLPlayItf cdaudioPlayer;
SLAndroidSimpleBufferQueueItf cdaudioBufferQueue;

void CDAudio_Callback(SLAndroidSimpleBufferQueueItf bufferQueue, void* context)
{
	auto copied = stb_vorbis_get_samples_short_interleaved(cdaudio_stream, cdaudio_info.channels, cdaudioStagingBuffer.data(), cdaudioStagingBuffer.size() >> 1);
	(*cdaudioBufferQueue)->Enqueue(cdaudioBufferQueue, cdaudioStagingBuffer.data(), copied << 2);
}

void CDAudio_FindInPath (const char *path, const char *prefix, const char *extension, std::vector<std::string>& result)
{
	auto extension_len = strlen(extension);
	auto to_search = std::string(path) + "/" + prefix;
	auto directory = opendir (to_search.c_str());
	if (directory != nullptr)
	{
		struct dirent* entry;
		while ((entry = readdir (directory)) != nullptr)
		{
			if (entry->d_type == DT_REG)
			{
				char* file = entry->d_name;
				auto in_str = strcasestr (file + strlen(file) - extension_len, extension);
				if (in_str != nullptr)
				{
					result.push_back(std::string(prefix) + file);
				}
			}
		}
		closedir (directory);
	}
}

void CDAudio_Play(byte track, qboolean looping)
{
	CDAudio_Shutdown();
	if (track < 2)
	{
		Con_DPrintf("CDAudio: Bad track number %u.\n", track);
		return;
	}
	std::vector<std::string> tracks;
	COM_FindAllFiles("music/", ".ogg", &CDAudio_FindInPath, tracks);
	if (track > tracks.size())
	{
		Con_DPrintf("CDAudio: Bad track number %u.\n", track);
		return;
	}
	std::sort(tracks.begin(), tracks.end());
	COM_LoadFile(tracks[track - 2].c_str(), cdaudioTrackContents);
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
		(*cdaudioEngineObject)->Destroy(cdaudioEngineObject);
		return;
	}
	result = (*cdaudioEngineObject)->GetInterface(cdaudioEngineObject, SL_IID_ENGINE, &cdaudioEngine);
	if (result != SL_RESULT_SUCCESS)
	{
		(*cdaudioEngineObject)->Destroy(cdaudioEngineObject);
		return;
	}
	result = (*cdaudioEngine)->CreateOutputMix(cdaudioEngine, &cdaudioOutputMixObject, 0, nullptr, nullptr);
	if (result != SL_RESULT_SUCCESS)
	{
		(*cdaudioEngineObject)->Destroy(cdaudioEngineObject);
		return;
	}
	result = (*cdaudioOutputMixObject)->Realize(cdaudioOutputMixObject, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
	{
		(*cdaudioOutputMixObject)->Destroy(cdaudioOutputMixObject);
		(*cdaudioEngineObject)->Destroy(cdaudioEngineObject);
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
		(*cdaudioOutputMixObject)->Destroy(cdaudioOutputMixObject);
		(*cdaudioEngineObject)->Destroy(cdaudioEngineObject);
		return;
	}
	result = (*cdaudioPlayerObject)->Realize(cdaudioPlayerObject, SL_BOOLEAN_FALSE);
	if (result != SL_RESULT_SUCCESS)
	{
		(*cdaudioPlayerObject)->Destroy(cdaudioPlayerObject);
		(*cdaudioOutputMixObject)->Destroy(cdaudioOutputMixObject);
		(*cdaudioEngineObject)->Destroy(cdaudioEngineObject);
		return;
	}
	result = (*cdaudioPlayerObject)->GetInterface(cdaudioPlayerObject, SL_IID_PLAY, &cdaudioPlayer);
	if (result != SL_RESULT_SUCCESS)
	{
		(*cdaudioPlayerObject)->Destroy(cdaudioPlayerObject);
		(*cdaudioOutputMixObject)->Destroy(cdaudioOutputMixObject);
		(*cdaudioEngineObject)->Destroy(cdaudioEngineObject);
		return;
	}
	result = (*cdaudioPlayerObject)->GetInterface(cdaudioPlayerObject, SL_IID_BUFFERQUEUE, &cdaudioBufferQueue);
	if (result != SL_RESULT_SUCCESS)
	{
		(*cdaudioPlayerObject)->Destroy(cdaudioPlayerObject);
		(*cdaudioOutputMixObject)->Destroy(cdaudioOutputMixObject);
		(*cdaudioEngineObject)->Destroy(cdaudioEngineObject);
		return;
	}
	result = (*cdaudioBufferQueue)->RegisterCallback(cdaudioBufferQueue, CDAudio_Callback, nullptr);
	if (result != SL_RESULT_SUCCESS)
	{
		(*cdaudioPlayerObject)->Destroy(cdaudioPlayerObject);
		(*cdaudioOutputMixObject)->Destroy(cdaudioOutputMixObject);
		(*cdaudioEngineObject)->Destroy(cdaudioEngineObject);
		return;
	}
	result = (*cdaudioPlayer)->SetPlayState(cdaudioPlayer, SL_PLAYSTATE_PLAYING);
	if (result != SL_RESULT_SUCCESS)
	{
		(*cdaudioPlayerObject)->Destroy(cdaudioPlayerObject);
		(*cdaudioOutputMixObject)->Destroy(cdaudioOutputMixObject);
		(*cdaudioEngineObject)->Destroy(cdaudioEngineObject);
		return;
	}
	for (auto samples = 1; samples < 1024 * 1024 * 1024; samples <<= 1)
	{
		if (samples >= cdaudio_info.sample_rate)
		{
			cdaudioStagingBuffer.resize(samples >> 3);
			auto copied = stb_vorbis_get_samples_short_interleaved(cdaudio_stream, cdaudio_info.channels, cdaudioStagingBuffer.data(), cdaudioStagingBuffer.size() >> 1);
			result = (*cdaudioBufferQueue)->Enqueue(cdaudioBufferQueue, cdaudioStagingBuffer.data(), copied << 2);
			if (result != SL_RESULT_SUCCESS)
			{
				(*cdaudioPlayerObject)->Destroy(cdaudioPlayerObject);
				(*cdaudioOutputMixObject)->Destroy(cdaudioOutputMixObject);
				(*cdaudioEngineObject)->Destroy(cdaudioEngineObject);
			}
			break;
		}
	}
}

void CDAudio_Stop(void)
{
}

void CDAudio_Pause(void)
{
}

void CDAudio_Resume(void)
{
}

void CDAudio_Update(void)
{
}

int CDAudio_Init(void)
{
	return 0;
}

void CDAudio_Shutdown(void)
{
	if (cdaudioPlayerObject != nullptr)
	{
		(*cdaudioPlayerObject)->Destroy(cdaudioPlayerObject);
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
		cdaudioEngineObject = nullptr;
	}
	if (cdaudio_stream != nullptr)
	{
		stb_vorbis_close(cdaudio_stream);
		cdaudio_stream = nullptr;
	}
}