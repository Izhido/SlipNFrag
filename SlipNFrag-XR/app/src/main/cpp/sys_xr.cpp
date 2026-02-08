#include "quakedef.h"
#include "sys_xr.h"
#include <cerrno>
#include <sys/stat.h>
#include <android/log.h>
#include "Locks.h"
#include "Logger_xr.h"
#include "d_lists.h"

int sys_argc;
char** sys_argv;
float frame_lapse = 1.0f / 60;
qboolean isDedicated;

std::vector<FILE*> sys_handles;

std::string sys_errormessage;

int sys_nogamedata;
int sys_errorcalled;
int sys_quitcalled;

std::vector<std::string> sys_bindingstext;

unsigned int sys_randseed;

int findhandle()
{
    for (size_t i = 0; i < sys_handles.size(); i++)
    {
        if (sys_handles[i] == nullptr)
        {
            return (int)i;
        }
    }
    sys_handles.emplace_back();
    return (int)sys_handles.size() - 1;
}

int filelength(FILE* f)
{
    auto pos = ftell(f);
    fseek(f, 0, SEEK_END);
    auto end = ftell(f);
    fseek(f, pos, SEEK_SET);
    return (int)end;
}

int Sys_FileOpenRead(const char* path, int* hndl)
{
    auto i = findhandle();
    auto f = fopen(path, "rb");
    if (f == nullptr)
    {
        *hndl = -1;
        return -1;
    }
    sys_handles[i] = f;
    *hndl = i;
    return filelength(f);
}

int Sys_FileOpenWrite(const char* path, qboolean abortonfail)
{
    auto i = findhandle();
    auto f = fopen(path, "wb");
	if (f == nullptr)
	{
		// UGLY HACK. If the file cannot be opened because of EACCES, the existing file might not
		// have the proper permissions from the tool that originally copied it. Since currently
		// no files opened for writing are have Sys_FileSeek() called on them, it can be safely
		// assumed that any files opened using Sys_FileOpenWrite() will be written from scratch.
		// Thus, delete the existing file, and try opening it again.
		if (errno == EACCES)
		{
			remove(path);
			f = fopen(path, "wb");
		}
	}
    if (f == nullptr)
    {
        if (abortonfail)
        {
            Sys_Error("Error opening %s: %s", path, strerror(errno));
        }
        return -1;
    }
    sys_handles[i] = f;
    return i;
}

int Sys_FileOpenAppend(const char* path)
{
    auto i = findhandle();
    auto f = fopen(path, "a");
    if (f == nullptr)
    {
        return -1;
    }
    sys_handles[i] = f;
    return i;
}

void Sys_FileClose(int handle)
{
    fclose(sys_handles[handle]);
    sys_handles[handle] = nullptr;
}

void Sys_FileSeek(int handle, int position)
{
    fseek(sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead(int handle, void* dest, int count)
{
    return (int)fread(dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite(int handle, const void* data, int count)
{
    return (int)fwrite(data, count, 1, sys_handles[handle]);
}

int Sys_FileTime(char* path)
{
    struct stat s;
    if (stat(path, &s) == 0)
    {
        return (int)s.st_mtime;
    }
    return -1;
}

void Sys_mkdir (char* path)
{
    mkdir(path, 0644);
}

void Sys_MakeCodeWriteable(unsigned long /*startaddr*/, unsigned long /*length*/)
{
}

void Sys_Error(const char* error, ...)
{
    va_list argptr;
    static std::vector<char> string(1024);
    
    while (true)
    {
        va_start (argptr, error);
        auto needed = vsnprintf(string.data(), string.size(), error, argptr);
        va_end (argptr);
        if (needed < string.size())
        {
            break;
        }
        string.resize(needed + 1);
    }
    __android_log_print(ANDROID_LOG_ERROR, Logger_xr::tag, "Sys_Error: %s", string.data());
    sys_errormessage = string.data();
    Host_Shutdown();
	sys_errorcalled = 1;
#ifdef USE_LONGJMP
	longjmp (host_abortserver, 1);
#else
	throw std::runtime_error("Sys_Error called");
#endif
}

void Sys_Printf(const char* fmt, ...)
{
    va_list argptr;
    std::vector<char> string(1024);
    static std::vector<char> buffered;
    int needed;

    while (true)
    {
        va_start (argptr, fmt);
        needed = vsnprintf(string.data(), string.size(), fmt, argptr);
        va_end (argptr);
        if (needed < string.size())
        {
            break;
        }
        string.resize(needed + 1);
    }

    std::lock_guard<std::mutex> lock(Locks::SysPrintMutex);

    std::copy(string.begin(), string.begin() + needed, std::back_inserter(buffered));
    auto start = 0;
    for (auto i = 0; i < buffered.size(); i++)
    {
        if (buffered[i] == '\n')
        {
            buffered[i] = 0;
            __android_log_print(ANDROID_LOG_VERBOSE, Logger_xr::tag, "%s", buffered.data() + start);
            start = i + 1;
        }
    }
    if (start > 0)
    {
        buffered.erase(buffered.cbegin(), buffered.cbegin() + start);
    }
}

void Sys_Quit()
{
    Host_Shutdown();
    sys_quitcalled = 1;
}

double Sys_FloatTime()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (now.tv_sec * 1e9 + now.tv_nsec) * 0.000000001;
}

char* Sys_ConsoleInput()
{
    return nullptr;
}

void Sys_Sleep()
{
}

void Sys_SendKeyEvents()
{
}

void Sys_HighFPPrecision()
{
}

void Sys_LowFPPrecision()
{
}

int Sys_Random()
{
    return rand_r(&sys_randseed);
}

void Sys_BeginClearMemory()
{
    Locks::RenderMutex.lock();
}

void Sys_EndClearMemory()
{
    Locks::RenderMutex.unlock();
}

extern void M_Menu_Options_f (void);
extern void M_Print (int cx, int cy, const char *str);
extern void M_DrawCharacter (int cx, int line, int num);
extern void M_DrawTransPic (int x, int y, qpic_t *pic);
extern void M_DrawPic (int x, int y, qpic_t *pic);
extern void M_DrawCheckbox (int x, int y, int on);

extern qboolean	m_entersound;

int		imm_cursor;

void CL_ImmersiveMenuDraw ()
{
	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	auto p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	M_Print (16, 32, "      Immersive:      ");

	M_Print (16, 48, "               Enabled");
	M_DrawCheckbox (220, 48, cl_immersive_enabled.value);

	M_Print (16, 56, "      Weapons in hands");
	M_DrawCheckbox (220, 56, cl_immersive_hands_enabled.value);

	M_Print (16, 64, "         Dominant hand");
	auto hand = Cvar_VariableString ("dominant_hand");
	if (Q_strncmp(hand, "left", 4) == 0)
		M_Print (220, 64, "Left");
	else
		M_Print (220, 64, "Right");

	M_Print (16, 72, "            Show hands");
	M_DrawCheckbox (220, 72, cl_immersive_show_hands.value);

	M_Print (16, 80, "      Sbar in off-hand");
	M_DrawCheckbox (220, 80, cl_immersive_sbar_on_hand.value);

	M_Print (16, 88, "          Head bobbing");
	M_DrawCheckbox (220, 88, (cl_bobdisabled.value == 0));

// cursor
	M_DrawCharacter (200, 48 + imm_cursor*8, 12+((int)(realtime*4)&1));
}

void CL_ImmersiveToggle ()
{
	const char* hand;

	switch (imm_cursor)
	{
	case 0:
		Cvar_SetValue ("immersive_enabled", !cl_immersive_enabled.value);
		break;
	case 1:
		Cvar_SetValue ("hands_enabled", !cl_immersive_hands_enabled.value);
		break;
	case 2:
		hand = Cvar_VariableString ("dominant_hand");
		if (Q_strncmp(hand, "left", 4) == 0)
			Cvar_Set("dominant_hand", "right");
		else
			Cvar_Set ("dominant_hand", "left");
		break;
	case 3:
		Cvar_SetValue ("show_hands", !cl_immersive_show_hands.value);
		break;
	case 4:
		Cvar_SetValue ("sbar_on_hand", !cl_immersive_sbar_on_hand.value);
		break;
	default:
		Cvar_SetValue ("cl_bobdisabled", !cl_bobdisabled.value);
		break;
	}

}
void CL_ImmersiveMenuKey (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_ENTER:
		m_entersound = true;
		CL_ImmersiveToggle();
		return;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		imm_cursor--;
		if (imm_cursor < 0)
			imm_cursor = 6-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		imm_cursor++;
		if (imm_cursor >= 6)
			imm_cursor = 0;
		break;

	case K_LEFTARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu3.wav");
		CL_ImmersiveToggle();
		break;
	}
}

void IN_MenuDraw ()
{
	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	auto p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	for (int i = 0; i < (int)sys_bindingstext.size(); i++)
	{
		M_Print (48, 32 + i * 8, sys_bindingstext[i].c_str());
	}
}

void IN_MenuKey (int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;
	}
}

void Sys_Init(int argc, char** argv)
{
    static quakeparms_t parms;
    parms.basedir = ".";
    COM_InitArgv(argc, argv);
    parms.argc = com_argc;
    parms.argv = com_argv;
    __android_log_print(ANDROID_LOG_VERBOSE, Logger_xr::tag, "Host_Init");
    Host_Init(&parms);
	cl_immersivemenudrawfn = CL_ImmersiveMenuDraw;
	cl_immersivemenukeyfn = CL_ImmersiveMenuKey;
	in_menudrawfn = IN_MenuDraw;
	in_menukeyfn = IN_MenuKey;
}

extern std::unordered_map<std::string_view, xcommand_t>	cmd_functions;
extern std::unordered_map<std::string_view, cvar_t*> cvar_index;

void COM_ClearFilesystem (void);

extern qboolean	scr_initialized;
extern m_state_t m_state;
extern int	m_main_cursor;
extern int	m_singleplayer_cursor;
extern int		load_cursor;
extern int	m_multiplayer_cursor;
extern int	m_net_cursor;
extern int		options_cursor;
extern int		keys_cursor;
extern int		serialConfig_cursor;
extern int		modemConfig_cursor;
extern int		gameoptions_cursor;
extern int		slist_cursor;
extern double		oldrealtime;
extern qboolean	r_skyinitialized;
extern qboolean	r_skyRGBAinitialized;
extern qboolean	r_skyboxinitialized;
extern std::string r_skyboxprefix;
extern std::unordered_map<std::string, texture_t**> r_skyboxtexsources;
extern std::vector<byte> r_24to8table;
extern std::unordered_map<std::string, qpic_t*> menu_cachepics;
extern std::list<model_t> mod_known;
extern std::list<sfx_t> known_sfx;
extern std::unordered_map<std::string, std::list<sfx_t>::iterator> known_sfx_index;
extern sfx_t		*ambient_sfx[NUM_AMBIENTS];

void Sys_Terminate()
{
	sys_quitcalled = 0;
	sys_errorcalled = 0;
	sys_errormessage = "";
	sys_nogamedata = 0;
	pr_edict_size = 0;
	pr_globals = nullptr;
	pr_global_struct = nullptr;
	pr_statements = nullptr;
	pr_globaldefs = nullptr;
	pr_fielddefs = nullptr;
	pr_strings = nullptr;
	pr_functions = nullptr;
	progs = nullptr;
	D_ResetLists();
	if (fakedma)
	{
		delete shm;
	}
	shm = nullptr;
	known_sfx_index.clear();
	for (auto& s : known_sfx)
	{
		delete[] s.data;
	}
	known_sfx.clear();
	Q_memset(ambient_sfx, 0, sizeof(ambient_sfx));
	for (auto& mod : mod_known)
	{
		if (mod.type == mod_alias) delete[] mod.extradata;
	}
	mod_known.clear();
	imm_cursor = 0;
	oldrealtime = 0;
	realtime = 0;
	slist_cursor = 0;
	gameoptions_cursor = 0;
	modemConfig_cursor = 0;
	serialConfig_cursor = 0;
	keys_cursor = 0;
	options_cursor = 0;
	m_net_cursor = 0;
	m_multiplayer_cursor = 0;
	load_cursor = 0;
	m_singleplayer_cursor = 0;
	m_main_cursor = 0;
	m_state = m_none;
	key_dest = key_game;
	for (auto& keybinding : keybindings)
	{
		delete[] keybinding;
		keybinding = nullptr;
	}
	for (auto& entry : menu_cachepics)
	{
		auto data = (byte*)entry.second;
		delete[] data;
	}
	menu_cachepics.clear();
	snd_initialized = false;
	r_skyinitialized = false;
	r_skyRGBAinitialized = false;
	r_skyboxinitialized = false;
	r_skyboxprefix = "";
	for (auto& entry : r_skyboxtexsources)
	{
		for (size_t i=0 ; i<6 ; i++)
		{
			delete[] entry.second[i];
		}
		delete[] entry.second;
	}
	host_initialized = false;
	con_forcedup = false;
	scr_disabled_for_loading = false;
	scr_fullupdate = 0;
	con_initialized = false;
	scr_initialized = false;
	Key_ClearStates ();
	Cmd_ClearAliases ();
	cmd_functions.clear();
	cvar_index.clear();
	cvar_vars = nullptr;
	COM_ClearFilesystem ();
	r_24to8table.clear();
	cls = { };
	cl.Clear();
	sv.Clear();
    com_token = "";
    com_gamedir = "";
    com_filesize = 0;
    standard_quake = false;
    rogue = false;
    hipnotic = false;
    msg_readcount = 0;
    msg_badread = false;
    cmd_source = src_command;
	for (auto& handle : sys_handles)
	{
		if (handle != nullptr)
		{
			fclose(handle);
		}
	}
	sys_handles.clear();
	frame_lapse = 1.0f / 60;
}

