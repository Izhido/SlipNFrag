#pragma once

#include <string>

#define LOG_TAG "SlipNFrag"

extern int sys_argc;
extern char** sys_argv;
extern float frame_lapse;
extern std::string sys_errormessage;
extern int sys_nogamedata;
extern int sys_quitcalled;

void Sys_Init(int argc, char** argv);
