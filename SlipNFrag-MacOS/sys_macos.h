//
//  sys_macos.h
//  SlipNFrag-MacOS
//
//  Created by Heriberto Delgado on 5/29/19.
//  Copyright © 2019 Heriberto Delgado. All rights reserved.
//

#pragma once

#include <string>

extern int sys_argc;
extern char** sys_argv;
extern float frame_lapse;
extern std::string sys_errormessage;
extern int sys_nogamedata;
extern int sys_quitcalled;

void Sys_Init(int argc, char** argv);
