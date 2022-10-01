#pragma once

#include <string>

extern int sys_argc;
extern char** sys_argv;
extern float frame_lapse;
extern std::string sys_errormessage;
extern int sys_nogamedata;
extern int sys_errorcalled;
extern int sys_quitcalled;

void Sys_Init(int argc, char** argv);
void Sys_Frame(float frame_lapse);
