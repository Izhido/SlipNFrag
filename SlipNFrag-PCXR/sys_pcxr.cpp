#include "quakedef.h"
#include "sys_pcxr.h"
#include "errno.h"
#include <sys/stat.h>
#include "Locks.h"
#include <iterator>
#include <random>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Logger_pcxr.h"

int sys_argc;
char** sys_argv;
float frame_lapse = 1.0f / 60;
qboolean isDedicated;

std::vector<FILE*> sys_handles;

std::string sys_errormessage;

int sys_nogamedata;
int sys_errorcalled;
int sys_quitcalled;

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
}

void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length)
{
}

void Sys_Error(const char* error, ...)
{
    va_list argptr;
    std::vector<char> string(1024);
    
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
    printf("[LOG_ERROR] ");
    printf(Logger_pcxr::tag);
    printf(": ");
    printf(string.data());
    printf("\n");
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
    va_start(argptr, fmt);
    vprintf(fmt, argptr);
    va_end(argptr);
}

void Sys_Quit()
{
    Host_Shutdown();
    sys_quitcalled = 1;
}

double Sys_FloatTime()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return (double)time.QuadPart / (double)frequency.QuadPart;
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
    static std::default_random_engine engine { };
    static std::uniform_int_distribution<> distribution(0, 32767);
    return distribution(engine);
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

	std::vector<std::string> bindingstext;

	auto hand = Cvar_VariableString ("dominant_hand");
	auto isLeftHanded = (Q_strncmp(hand, "left", 4) == 0);

	if (isLeftHanded)
	{
		bindingstext.emplace_back("Left handed controls:");
	}
	else
	{
		bindingstext.emplace_back("Right handed controls:");
	}
	bindingstext.emplace_back("");
	bindingstext.emplace_back("Left or Right Joysticks:");
	bindingstext.emplace_back("Walk Forward / Backpedal,");
	bindingstext.emplace_back("   Step Left / Step Right");
	bindingstext.emplace_back("");
	if (isLeftHanded)
	{
		bindingstext.emplace_back("(Y): Jump");
		bindingstext.emplace_back("(X): Swim down");
	}
	else
	{
		bindingstext.emplace_back("(B): Jump");
		bindingstext.emplace_back("(A): Swim down");
	}
	bindingstext.emplace_back("");
	bindingstext.emplace_back("Triggers: Attack");
	bindingstext.emplace_back("Grip Triggers: Run");
	bindingstext.emplace_back("Click Joysticks: Change Weapon");
	bindingstext.emplace_back("");
	if (isLeftHanded)
	{
		bindingstext.emplace_back("(A): Toggle Main Menu");
	}
	else
	{
		bindingstext.emplace_back("(X): Toggle Main Menu");
	}

	for (int i = 0; i < (int)bindingstext.size(); i++)
	{
		M_Print (48, 32 + i * 8, bindingstext[i].c_str());
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
    printf("[LOG_VERBOSE] ");
    printf(Logger_pcxr::tag);
    printf(": Host_Init\n");
    Host_Init(&parms);
	cl_immersivemenudrawfn = CL_ImmersiveMenuDraw;
	cl_immersivemenukeyfn = CL_ImmersiveMenuKey;
	in_menudrawfn = IN_MenuDraw;
	in_menukeyfn = IN_MenuKey;
}
