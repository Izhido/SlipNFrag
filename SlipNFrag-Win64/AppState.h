#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>
#include <string>
#include <vector>
#include <thread>
#include "Engine.h"

struct AppState
{
    std::string commandLine;
    HINSTANCE instance;
    HWND hWnd;
    std::wstring windowTitle;
    std::wstring windowClass;
    BOOL usesDarkMode;
    int nonClientWidth;
    int nonClientHeight;
    HWND playButton;
    bool painting;
    bool cursorRelocated;
    std::vector<unsigned char> palette;
    Engine engine;
    std::thread* engineThread;
    XINPUT_STATE xInputState;
    bool xInputEnabled;
};

extern AppState appState;
