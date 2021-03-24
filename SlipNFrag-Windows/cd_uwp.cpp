#include "quakedef.h"
#include "stb_vorbis.c"

stb_vorbis* cdaudio_stream;
stb_vorbis_info cdaudio_info;

int cdaudio_refcount;

std::unordered_map<int, std::string> cdaudio_tracks;
std::vector<byte> cdaudio_trackContents;

qboolean cdaudio_cdValid = false;
qboolean cdaudio_playing = false;
qboolean cdaudio_wasPlaying = false;
qboolean cdaudio_initialized = false;
qboolean cdaudio_enabled = false;
qboolean cdaudio_playLooping = false;
float cdaudio_cdvolume;
byte cdaudio_playTrack;

// Declared here to avoid #including "sys_uwp.h":
void Sys_FindInPath(const char* path, const char* directory, const char* prefix, const char* extension, std::vector<std::string>& result);

static int CDAudio_GetAudioDiskInfo(void)
{
	cdaudio_cdValid = false;

	cdaudio_tracks.clear();

	std::vector<std::string> tracks;
	COM_FindAllFiles("music/", "track", ".ogg", &Sys_FindInPath, tracks);

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

	cdaudio_refcount++;

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

	cdaudio_wasPlaying = cdaudio_playing;
	cdaudio_playing = false;
}

void CDAudio_Resume(void)
{
	if (!cdaudio_enabled)
		return;

	if (!cdaudio_cdValid)
		return;

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