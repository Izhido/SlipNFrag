#pragma once

#include <string>

enum fileoperation_t
{
	fo_idle,
	fo_error,
	fo_openread,
	fo_openwrite,
	fo_openappend,
	fo_read,
	fo_write,
	fo_time
};

struct file_t
{
	winrt::Windows::Storage::StorageFile file = nullptr;
	winrt::Windows::Storage::Streams::IRandomAccessStream stream = nullptr;
};

extern int sys_argc;
extern char** sys_argv;
extern float frame_lapse;
extern std::string sys_errormessage;
extern int sys_nogamedata;
extern std::vector<file_t> sys_files;
extern fileoperation_t sys_fileoperation;
extern int sys_fileoperationindex;
extern std::string sys_fileoperationname;
extern byte* sys_fileoperationbuffer;
extern int sys_fileoperationsize;
extern std::string sys_fileoperationerror;
extern int sys_fileoperationtime;
extern qboolean sys_errorcalled;
extern qboolean sys_quitcalled;
extern std::mutex sys_iomutex;

void Sys_Init(int argc, char** argv);
void Sys_Frame(float frame_lapse);
