#include "pch.h"
#include "quakedef.h"
#include "sys_uwp.h"
#include "errno.h"
#include <sys/stat.h>
#include <random>

using namespace winrt;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;

int sys_argc;
char** sys_argv;
float frame_lapse = 1.0f / 60;
qboolean isDedicated;
std::string sys_errormessage;
qboolean sys_errorcalled;
qboolean sys_quitcalled;
int sys_nogamedata;
std::vector<file_t> sys_files;
fileoperation_t sys_fileoperation;
int sys_fileoperationindex;
std::string sys_fileoperationname;
byte* sys_fileoperationbuffer;
int sys_fileoperationsize;
std::string sys_fileoperationerror;
int sys_fileoperationtime;
std::mutex sys_iomutex;
std::mutex sys_errormutex;

int findhandle()
{
	for (size_t i = 0; i < sys_files.size(); i++)
	{
		if (sys_files[i].file == nullptr)
		{
			return (int)i;
		}
	}
	sys_files.emplace_back();
	return (int)sys_files.size() - 1;
}

fileoperation_t Sys_WaitForIOCompletion(fileoperation_t operation)
{
	auto start = Sys_FloatTime();
	fileoperation_t result = fo_idle;
	do
	{
		std::this_thread::yield();
		{
			std::lock_guard lock(sys_iomutex);
			result = sys_fileoperation;
		}
	} while (result == operation && Sys_FloatTime() - start < 5);
	return result;
}

int Sys_FileOpenRead(const char* path, int* hndl)
{
	{
		std::lock_guard lock(sys_iomutex);
		sys_fileoperation = fo_openread;
		sys_fileoperationindex = findhandle();
		sys_fileoperationname = path;
	}
	auto operation = Sys_WaitForIOCompletion(fo_openread);
	if (operation == fo_openread)
	{
		Sys_Error("Sys_FileOpenRead timed out opening file after 5 seconds");
	}
	if (operation == fo_error)
	{
		*hndl = -1;
		return -1;
	}
	*hndl = sys_fileoperationindex;
	return (int)sys_fileoperationsize;
}

int Sys_FileOpenWrite(const char* path, qboolean abortonfail)
{
	{
		std::lock_guard lock(sys_iomutex);
		sys_fileoperation = fo_openwrite;
		sys_fileoperationindex = findhandle();
		sys_fileoperationname = path;
	}
	auto operation = Sys_WaitForIOCompletion(fo_openwrite);
	if (operation == fo_openwrite)
	{
		Sys_Error("Sys_FileOpenWrite timed out after 5 seconds");
	}
	if (operation == fo_error)
	{
		if (abortonfail)
		{
			Sys_Error("Error opening %s: %s", path, sys_fileoperationerror.c_str());
		}
		return -1;
	}
	return sys_fileoperationindex;
}

int Sys_FileOpenAppend(const char* path)
{
	{
		std::lock_guard lock(sys_iomutex);
		sys_fileoperation = fo_openappend;
		sys_fileoperationindex = findhandle();
		sys_fileoperationname = path;
	}
	auto operation = Sys_WaitForIOCompletion(fo_openappend);
	if (operation == fo_openappend)
	{
		Sys_Error("Sys_FileOpenAppend timed out after 5 seconds");
	}
	if (operation == fo_error)
	{
		return -1;
	}
	return sys_fileoperationindex;
}

void Sys_FileClose(int handle)
{
	sys_fileoperationindex = handle;
	sys_files[handle].stream.Close();
	sys_files[handle].stream = nullptr;
	sys_files[handle].file = nullptr;
}

void Sys_FileSeek(int handle, int position)
{
	sys_files[handle].stream.Seek(position);
}

int Sys_FileRead(int handle, void* dest, int count)
{
	{
		std::lock_guard lock(sys_iomutex);
		sys_fileoperation = fo_read;
		sys_fileoperationindex = handle;
		sys_fileoperationbuffer = (byte*)dest;
		sys_fileoperationsize = count;
	}
	auto operation = Sys_WaitForIOCompletion(fo_read);
	if (operation == fo_read)
	{
		Sys_Error("Sys_FileRead timed out after 5 seconds");
	}
	return (int)sys_fileoperationsize;
}

int Sys_FileWrite(int handle, const void* data, int count)
{
	{
		std::lock_guard lock(sys_iomutex);
		sys_fileoperation = fo_write;
		sys_fileoperationindex = handle;
		sys_fileoperationbuffer = (byte*)data;
		sys_fileoperationsize = count;
	}
	auto operation = Sys_WaitForIOCompletion(fo_write);
	if (operation == fo_write)
	{
		Sys_Error("Sys_FileWrite timed out after 5 seconds");
	}
	return (int)sys_fileoperationsize;
}

int Sys_FileTime(char* path)
{
	{
		std::lock_guard lock(sys_iomutex);
		sys_fileoperation = fo_time;
		sys_fileoperationname = path;
	}
	auto operation = Sys_WaitForIOCompletion(fo_time);
	if (operation == fo_time)
	{
		Sys_Error("Sys_FileTime timed out after 5 seconds");
	}
	if (operation == fo_error)
	{
		return -1;
	}
	return sys_fileoperationtime;
}

void Sys_mkdir(char* path)
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
		va_start(argptr, error);
		auto needed = vsnprintf(string.data(), string.size(), error, argptr);
		va_end(argptr);
		if (needed < string.size())
		{
			break;
		}
		string.resize(needed + 1);
	}
	Sys_Printf("Sys_Error: %s\n", string.data());
	{
		std::lock_guard lock(sys_errormutex);
		sys_errormessage = string.data();
		sys_errorcalled = true;
	}
	throw std::runtime_error("Sys_Error called.");
}

void Sys_Printf(const char* fmt, ...)
{
	va_list argptr;
	static std::vector<char> string(1024);

	while (true)
	{
		va_start(argptr, fmt);
		auto needed = vsnprintf(string.data(), string.size(), fmt, argptr);
		va_end(argptr);
		if (needed < string.size())
		{
			break;
		}
		string.resize(needed + 1);
	}
	OutputDebugStringA(string.data());
}

void Sys_Quit()
{
	Host_Shutdown();
	{
		std::lock_guard lock(sys_errormutex);
		sys_quitcalled = true;
	}
	throw std::runtime_error("Sys_Quit called.");
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
	static std::uniform_int_distribution distribution(0, 32767);
	return distribution(engine);
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

void Sys_Frame(float frame_lapse)
{
	auto updated = Host_FrameUpdate(frame_lapse);
	if (updated)
	{
		Host_FrameRender();
	}
	Host_FrameFinish(updated);
}
