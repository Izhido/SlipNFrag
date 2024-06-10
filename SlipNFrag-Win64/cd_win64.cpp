#include "quakedef.h"
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#include "AppState.h"
#include <mmeapi.h>
#include <mmiscapi.h>
#include <mmreg.h>
#include "Locks.h"

stb_vorbis* cdaudio_stream;
stb_vorbis_info cdaudio_info;

std::unordered_map<int, std::string> cdaudio_tracks;
std::vector<byte> cdaudio_trackContents;
std::vector<float> cdaudio_stagingBuffer;

WAVEFORMATEX cdaudio_waveformat { };
HWAVEOUT cdaudio_waveout = NULL;
HANDLE cdaudio_wavebuffer1 = NULL;
HPSTR cdaudio_wavedata1 = NULL;
HGLOBAL cdaudio_waveheader1 = NULL;
LPWAVEHDR cdaudio_waveheaderptr1 = NULL;
HANDLE cdaudio_wavebuffer2 = NULL;
HPSTR cdaudio_wavedata2 = NULL;
HGLOBAL cdaudio_waveheader2 = NULL;
LPWAVEHDR cdaudio_waveheaderptr2 = NULL;

int cdaudio_lastCopied;

static qboolean cdaudio_cdValid = false;
static qboolean	cdaudio_playing = false;
static qboolean	cdaudio_wasPlaying = false;
static qboolean	cdaudio_initialized = false;
static qboolean	cdaudio_enabled = false;
static qboolean cdaudio_playLooping = false;
static float	cdaudio_cdvolume;
static byte		cdaudio_playTrack;

void CDAudio_DisposeBuffers()
{
	if (cdaudio_waveout != NULL)
	{
		waveOutReset(cdaudio_waveout);
		if (cdaudio_waveheader2 != NULL)
		{
			waveOutUnprepareHeader(cdaudio_waveout, cdaudio_waveheaderptr2, sizeof(WAVEHDR));
			GlobalUnlock(cdaudio_waveheader2);
			GlobalFree(cdaudio_waveheader2);
			cdaudio_waveheader2 = NULL;
			cdaudio_waveheaderptr2 = NULL;
		}
		if (cdaudio_wavebuffer2 != NULL)
		{
			GlobalUnlock(cdaudio_wavebuffer2);
			GlobalFree(cdaudio_wavebuffer2);
			cdaudio_wavebuffer2 = NULL;
			cdaudio_wavedata2 = NULL;
		}
		if (cdaudio_waveheader1 != NULL)
		{
			waveOutUnprepareHeader(cdaudio_waveout, cdaudio_waveheaderptr1, sizeof(WAVEHDR));
			GlobalUnlock(cdaudio_waveheader1);
			GlobalFree(cdaudio_waveheader1);
			cdaudio_waveheader1 = NULL;
			cdaudio_waveheaderptr1 = NULL;
		}
		if (cdaudio_wavebuffer1 != NULL)
		{
			GlobalUnlock(cdaudio_wavebuffer1);
			GlobalFree(cdaudio_wavebuffer1);
			cdaudio_wavebuffer1 = NULL;
			cdaudio_wavedata1 = NULL;
		}
		waveOutClose(cdaudio_waveout);
		cdaudio_waveout = NULL;
		cdaudio_waveformat = { };
	}
	if (cdaudio_stream != nullptr)
	{
		stb_vorbis_close(cdaudio_stream);
		cdaudio_stream = nullptr;
	}
}

void CDAudio_Callback(void* waveOut, void* waveHeader)
{
	std::lock_guard<std::mutex> lock(Locks::SoundMutex);

	if (!cdaudio_enabled)
		return;

	if (!cdaudio_playing)
		return;

	if (cdaudio_stream == nullptr)
		return;

	if (cdaudio_waveout == NULL || cdaudio_waveout != waveOut)
		return;

	cdaudio_lastCopied = stb_vorbis_get_samples_float_interleaved(cdaudio_stream, cdaudio_info.channels, cdaudio_stagingBuffer.data(), cdaudio_stagingBuffer.size() >> 1);
	if (cdaudio_lastCopied == 0 && cdaudio_playLooping)
	{
		stb_vorbis_seek_start(cdaudio_stream);
		cdaudio_lastCopied = stb_vorbis_get_samples_float_interleaved(cdaudio_stream, cdaudio_info.channels, cdaudio_stagingBuffer.data(), cdaudio_stagingBuffer.size() >> 1);
	}

	if (cdaudio_lastCopied == 0)
		return;

	memcpy(((LPWAVEHDR)waveHeader)->lpData, cdaudio_stagingBuffer.data(), cdaudio_lastCopied << 3);
	auto result = waveOutWrite(cdaudio_waveout, ((LPWAVEHDR)waveHeader), sizeof(WAVEHDR));
	if (result != MMSYSERR_NOERROR)
	{
		CDAudio_DisposeBuffers();

		cdaudio_wasPlaying = false;
		cdaudio_playing = false;
	}
}

void CDAudio_FindInPath(const char* path, const char* directory, const char* prefix, const char* extension, std::vector<std::string>& result)
{
	auto prefix_len = Q_strlen(prefix);
	auto extension_len = Q_strlen(extension);
	auto to_search = std::string(path) + "\\" + directory + "*";
	WIN32_FIND_DATAA find;
	auto handle = FindFirstFileA(to_search.c_str(), &find);
	if (handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			char* file = find.cFileName;
			auto match = Q_strncasecmp(file, prefix, prefix_len);
			if (match == 0)
			{
				match = Q_strncasecmp(file + strlen(file) - extension_len, extension, extension_len);
				if (match == 0)
				{
					result.push_back(std::string(directory) + file);
				}
			}
		} while (FindNextFileA(handle, &find));
		FindClose(handle);
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

		std::lock_guard<std::mutex> lock(Locks::SoundMutex);

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

	std::lock_guard<std::mutex> lock(Locks::SoundMutex);

	int error = VORBIS__no_error;
	cdaudio_stream = stb_vorbis_open_memory(cdaudio_trackContents.data(), cdaudio_trackContents.size() - 1, &error, nullptr);
	if (error != VORBIS__no_error)
	{
		Con_DPrintf("CDAudio: Error %i opening track %u.\n", error, track);
		cdaudio_stream = nullptr;
		return;
	}
	cdaudio_info = stb_vorbis_get_info(cdaudio_stream);

	int samples;
	for (samples = 1; samples < 1024 * 1024 * 1024; samples <<= 1)
	{
		if (samples >= cdaudio_info.sample_rate)
		{
			break;
		}
	}

	cdaudio_waveformat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	cdaudio_waveformat.nChannels = cdaudio_info.channels;
	cdaudio_waveformat.nSamplesPerSec = cdaudio_info.sample_rate;
	cdaudio_waveformat.nAvgBytesPerSec = cdaudio_info.sample_rate * cdaudio_info.channels * 4;
	cdaudio_waveformat.nBlockAlign = cdaudio_info.channels * 4;
	cdaudio_waveformat.wBitsPerSample = 32;

	auto result = waveOutOpen(&cdaudio_waveout, WAVE_MAPPER, &cdaudio_waveformat, (DWORD_PTR)appState.hWnd, 0, CALLBACK_WINDOW);
	if (result != MMSYSERR_NOERROR)
	{
		CDAudio_DisposeBuffers();
		return;
	}

	cdaudio_wavebuffer1 = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, samples >> 2);
	if (cdaudio_wavebuffer1 == NULL)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	cdaudio_wavedata1 = (LPSTR)GlobalLock(cdaudio_wavebuffer1);
	if (cdaudio_wavebuffer1 == NULL)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	cdaudio_waveheader1 = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (DWORD)sizeof(WAVEHDR));
	if (cdaudio_waveheader1 == NULL)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	cdaudio_waveheaderptr1 = (LPWAVEHDR)GlobalLock(cdaudio_waveheader1);
	if (cdaudio_waveheaderptr1 == NULL)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	cdaudio_waveheaderptr1->lpData = cdaudio_wavedata1;
	cdaudio_waveheaderptr1->dwBufferLength = (samples >> 2);
	cdaudio_waveheaderptr1->dwLoops = 0;
	cdaudio_waveheaderptr1->dwFlags = 0;
	result = waveOutPrepareHeader(cdaudio_waveout, cdaudio_waveheaderptr1, sizeof(WAVEHDR));
	if (result != MMSYSERR_NOERROR)
	{
		CDAudio_DisposeBuffers();
		return;
	}

	result = waveOutWrite(cdaudio_waveout, cdaudio_waveheaderptr1, sizeof(WAVEHDR));
	if (result != MMSYSERR_NOERROR)
	{
		CDAudio_DisposeBuffers();
		return;
	}

	cdaudio_wavebuffer2 = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, samples >> 2);
	if (cdaudio_wavebuffer2 == NULL)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	cdaudio_wavedata2 = (LPSTR)GlobalLock(cdaudio_wavebuffer2);
	if (cdaudio_wavebuffer2 == NULL)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	cdaudio_waveheader2 = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (DWORD)sizeof(WAVEHDR));
	if (cdaudio_waveheader2 == NULL)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	cdaudio_waveheaderptr2 = (LPWAVEHDR)GlobalLock(cdaudio_waveheader2);
	if (cdaudio_waveheaderptr2 == NULL)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	cdaudio_waveheaderptr2->lpData = cdaudio_wavedata2;
	cdaudio_waveheaderptr2->dwBufferLength = (samples >> 2);
	cdaudio_waveheaderptr2->dwLoops = 0;
	cdaudio_waveheaderptr2->dwFlags = 0;
	result = waveOutPrepareHeader(cdaudio_waveout, cdaudio_waveheaderptr2, sizeof(WAVEHDR));
	if (result != MMSYSERR_NOERROR)
	{
		CDAudio_DisposeBuffers();
		return;
	}

	result = waveOutWrite(cdaudio_waveout, cdaudio_waveheaderptr2, sizeof(WAVEHDR));
	if (result != MMSYSERR_NOERROR)
	{
		CDAudio_DisposeBuffers();
		return;
	}
	cdaudio_stagingBuffer.resize(samples >> 3);
	cdaudio_lastCopied = -1;

	cdaudio_playLooping = looping;
	cdaudio_playTrack = track;
	cdaudio_playing = true;

	if (cdaudio_cdvolume == 0.0)
		CDAudio_Pause();
}

void CDAudio_Stop(void)
{
	if (!cdaudio_enabled)
		return;

	if (!cdaudio_playing)
		return;

	std::lock_guard<std::mutex> lock(Locks::SoundMutex);

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

	auto result = waveOutPause(cdaudio_waveout);
	if (result != 0)
	{
		Con_DPrintf("Audio track pause failed (%i)", result);
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

	auto result = waveOutRestart(cdaudio_waveout);
	if (result != 0)
	{
		Con_DPrintf("Audio track resume failed (%i)", result);
	}

	if (!cdaudio_wasPlaying)
		return;

	cdaudio_playing = true;
}


static void CD_f(void)
{
	const char* command;

	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv(1);

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
		CDAudio_Play((byte)Q_atoi(Cmd_Argv(2)), false);
		return;
	}

	if (Q_strcasecmp(command, "loop") == 0)
	{
		CDAudio_Play((byte)Q_atoi(Cmd_Argv(2)), true);
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
			Cvar_SetValue("bgmvolume", 0.0);
			cdaudio_cdvolume = bgmvolume.value;
			CDAudio_Pause();
		}
		else
		{
			Cvar_SetValue("bgmvolume", 1.0);
			cdaudio_cdvolume = bgmvolume.value;
			CDAudio_Resume();
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

	Cmd_AddCommand("cd", CD_f);

	Con_Printf("CD Audio Initialized\n");

	return 0;
}

void CDAudio_Shutdown(void)
{
	if (!cdaudio_initialized)
		return;
	CDAudio_Stop();
}
