//
//  sys_macos.cpp
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 5/29/19.
//  Copyright © 2019 Heriberto Delgado. All rights reserved.
//

#include "quakedef.h"
#include "errno.h"
#include <sys/stat.h>
#include "Locks.h"

extern "C" double CACurrentMediaTime();

int sys_argc;
char** sys_argv;
float frame_lapse = 1.0f / 60;
qboolean isDedicated;

std::vector<FILE*> sys_handles;

std::string sys_errormessage;

int sys_nogamedata;
int sys_quitcalled;

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
    mkdir(path, 0644);
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
    printf("Sys_Error: %s\n", string.data());
    sys_errormessage = string.data();
    Host_Shutdown();
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
    return CACurrentMediaTime();
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
    return rand();
}

void Sys_BeginClearMemory()
{
	Locks::RenderMutex.lock();
}

void Sys_EndClearMemory()
{
	Locks::RenderMutex.unlock();
}

void Sys_Init(int argc, char** argv)
{
    static quakeparms_t parms;
    parms.basedir = ".";
    COM_InitArgv(argc, argv);
    parms.argc = com_argc;
    parms.argv = com_argv;
    printf("Host_Init\n");
    Host_Init(&parms);
}
