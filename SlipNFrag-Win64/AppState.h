#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>

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
    bool started;
    bool painting;
    LARGE_INTEGER previousTime;
    bool cursorRelocated;
    std::vector<unsigned char> palette;
};

extern AppState appState;
