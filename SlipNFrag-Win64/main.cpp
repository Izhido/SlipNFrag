#include "AppState.h"
#include <dwmapi.h>
#include <locale>
#include <codecvt>
#include <vector>
#include "sys_win64.h"
#include "vid_win64.h"
#include "r_local.h"
#include "virtualkeymap.h"
#include "in_win64.h"
#include "resource.h"
#include "snd_win64.h"
#include "cd_win64.h"
#include <mmsystem.h>
#include <objidl.h>
#include <gdiplus.h>
#include "Engine.h"
#include "Locks.h"
#include "Input.h"
#include "DirectRect.h"
#include <Xinput.h>

extern m_state_t m_state;

using namespace Gdiplus;

#pragma comment (lib,"Gdiplus.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#define IDC_PLAY_BUTTON 100

AppState appState { };

BOOL IsDarkThemeActive()
{
    DWORD type;
    DWORD value;
    DWORD count = 4;
    auto err = RegGetValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_REG_DWORD, &type, &value, &count);
    if (err == ERROR_SUCCESS && type == REG_DWORD)
    {
        return (value == 0);
    }
    return FALSE;
}

void HandleXInputState(XINPUT_STATE& previousState, XINPUT_STATE& newState)
{
    auto previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0);
    auto newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0);
    if (previousButton != newButton)
    {
        Input::AddKeyInput(K_ESCAPE, newButton);
    }

    previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0);
    newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0);
    if (previousButton != newButton)
    {
        Input::AddKeyInput(K_ESCAPE, newButton);
    }

    previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0);
    newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0);
    if (previousButton != newButton)
    {
        if (key_dest == key_menu)
        {
            if (m_state != m_quit)
            {
                Input::AddKeyInput(K_ENTER, newButton);
            }
        }
        else if (key_dest == key_game)
        {
            if (newButton)
            {
                Input::AddCommandInput("+jump");
            }
            else
            {
                Input::AddCommandInput("-jump");
            }
        }
    }

    previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0);
    newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0);
    if (previousButton != newButton)
    {
        if (key_dest == key_menu)
        {
            if (m_state != m_quit)
            {
                Input::AddKeyInput(K_ESCAPE, newButton);
            }
        }
        else if (key_dest == key_game)
        {
            if (newButton)
            {
                Input::AddCommandInput("+movedown");
            }
            else
            {
                Input::AddCommandInput("-movedown");
            }
        }
    }

    previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0);
    newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0);
    if (previousButton != newButton)
    {
        if (key_dest == key_menu)
        {
            if (m_state != m_quit)
            {
                Input::AddKeyInput(K_ENTER, newButton);
            }
        }
        else if (key_dest == key_game)
        {
            if (newButton)
            {
                Input::AddCommandInput("+speed");
            }
            else
            {
                Input::AddCommandInput("-speed");
            }
        }
    }

    previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0);
    newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0);
    if (previousButton != newButton)
    {
        if (key_dest == key_menu)
        {
            if (m_state == m_quit)
            {
                Input::AddKeyInput('y', newButton);
            }
            else
            {
                Input::AddKeyInput(K_ESCAPE, newButton);
            }
        }
        else if (key_dest == key_game)
        {
            if (newButton)
            {
                Input::AddCommandInput("+moveup");
            }
            else
            {
                Input::AddCommandInput("-moveup");
            }
        }
    }

    previousButton = (previousState.Gamepad.bLeftTrigger > 127);
    newButton = (newState.Gamepad.bLeftTrigger > 127);
    if (previousButton != newButton)
    {
        if (key_dest == key_menu)
        {
            if (m_state != m_quit)
            {
                Input::AddKeyInput(K_ENTER, newButton);
            }
        }
        else if (key_dest == key_game)
        {
            if (newButton)
            {
                Input::AddCommandInput("+attack");
            }
            else
            {
                Input::AddCommandInput("-attack");
            }
        }
    }

    previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
    newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
    if (previousButton != newButton)
    {
        if (key_dest == key_menu)
        {
            if (m_state != m_quit)
            {
                Input::AddKeyInput(K_ESCAPE, newButton);
            }
        }
        else if (key_dest == key_game)
        {
            if (newButton)
            {
                Input::AddCommandInput("impulse 10");
            }
        }
    }

    previousButton = (previousState.Gamepad.bRightTrigger > 127);
    newButton = (newState.Gamepad.bRightTrigger > 127);
    if (previousButton != newButton)
    {
        if (key_dest == key_menu)
        {
            if (m_state != m_quit)
            {
                Input::AddKeyInput(K_ENTER, newButton);
            }
        }
        else if (key_dest == key_game)
        {
            if (newButton)
            {
                Input::AddCommandInput("+attack");
            }
            else
            {
                Input::AddCommandInput("-attack");
            }
        }
    }

    previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
    newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
    if (previousButton != newButton)
    {
        if (key_dest == key_menu)
        {
            if (m_state != m_quit)
            {
                Input::AddKeyInput(K_ESCAPE, newButton);
            }
        }
        else if (key_dest == key_game)
        {
            if (newButton)
            {
                Input::AddCommandInput("impulse 10");
            }
        }
    }

    previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
    newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
    if (previousButton != newButton)
    {
        Input::AddKeyInput(K_UPARROW, newButton);
    }

    previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
    newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
    if (previousButton != newButton)
    {
        Input::AddKeyInput(K_LEFTARROW, newButton);
    }

    previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
    newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
    if (previousButton != newButton)
    {
        Input::AddKeyInput(K_RIGHTARROW, newButton);
    }

    previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
    newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
    if (previousButton != newButton)
    {
        Input::AddKeyInput(K_DOWNARROW, newButton);
    }

    auto previousXValue = previousState.Gamepad.sThumbLX;
    if (previousXValue < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && previousXValue > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        previousXValue = 0;
    }
    auto newXValue = newState.Gamepad.sThumbLX;
    if (newXValue < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && newXValue > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        newXValue = 0;
    }
    if (previousXValue != newXValue)
    {
        pdwRawValue[JOY_AXIS_X] = -(float)newXValue / 32768;
    }

    auto previousYValue = previousState.Gamepad.sThumbLY;
    if (previousYValue < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && previousYValue > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        previousYValue = 0;
    }
    auto newYValue = newState.Gamepad.sThumbLY;
    if (newYValue < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && newYValue > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        newYValue = 0;
    }
    if (previousYValue != newYValue)
    {
        pdwRawValue[JOY_AXIS_Y] = -(float)newYValue / 32768;
    }

    previousXValue = previousState.Gamepad.sThumbRX;
    if (previousXValue < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && previousXValue > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        previousXValue = 0;
    }
    newXValue = newState.Gamepad.sThumbRX;
    if (newXValue < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && newXValue > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        newXValue = 0;
    }
    if (previousXValue != newXValue)
    {
        pdwRawValue[JOY_AXIS_Z] = (float)newXValue / 32768;
    }

    previousYValue = previousState.Gamepad.sThumbRY;
    if (previousYValue < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && previousYValue > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        previousYValue = 0;
    }
    newYValue = newState.Gamepad.sThumbRY;
    if (newYValue < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && newYValue > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
        newYValue = 0;
    }
    if (previousYValue != newYValue)
    {
        pdwRawValue[JOY_AXIS_R] = -(float)newYValue / 32768;
    }

    previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
    newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
    if (previousButton != newButton)
    {
		if (newButton)
		{
			Input::AddCommandInput("centerview");
		}
    }

    previousButton = ((previousState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
    newButton = ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
    if (previousButton != newButton)
    {
        if (newButton)
        {
            Input::AddCommandInput("+mlook");
        }
        else
        {
            Input::AddCommandInput("-mlook");
        }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        RECT windowRect;
        GetWindowRect(hWnd, &windowRect);
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        auto dpi = GetDpiForWindow(hWnd);
        auto halfWidth = 75 * dpi / USER_DEFAULT_SCREEN_DPI;
        auto halfHeight = halfWidth;
        appState.nonClientWidth = (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
        appState.nonClientHeight = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);
        auto left = (clientRect.left + clientRect.right) / 2 - halfWidth;
        auto top = (clientRect.top + clientRect.bottom) / 2 - halfHeight;
        auto width = halfWidth * 2;
        auto height = halfHeight * 2;
        appState.playButton = CreateWindow(L"BUTTON", L"Play", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, left, top, width, height, hWnd, (HMENU)IDC_PLAY_BUTTON, appState.instance, NULL);
        break;
    }
    case WM_GETMINMAXINFO:
    {
        auto lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = 320 + appState.nonClientWidth;
        lpMMI->ptMinTrackSize.y = 200 + appState.nonClientHeight;
        break;
    }
    case WM_SETFOCUS:
        if (appState.playButton != NULL)
        {
            UpdateWindow(appState.playButton);
        }
        break;
    case WM_SIZE:
    {
        if (appState.playButton != NULL)
        {
            auto width = LOWORD(lParam);
            auto height = HIWORD(lParam);
            if (width > 0 && height > 0)
            {
                auto dpi = GetDpiForWindow(hWnd);
                auto halfWidth = 75 * dpi / USER_DEFAULT_SCREEN_DPI;
                auto halfHeight = halfWidth;
                auto left = width / 2 - halfWidth;
                auto top = height / 2 - halfHeight;
                auto newWidth = halfWidth * 2;
                auto newHeight = halfHeight * 2;
                SetWindowPos(appState.playButton, NULL, left, top, newWidth, newHeight, 0);
                UpdateWindow(appState.playButton);
            }
        }
        break;
    }
    case WM_MOVE:
        if (appState.playButton != NULL)
        {
            UpdateWindow(appState.playButton);
        }
        break;
    case WM_COMMAND:
        if (wParam == IDC_PLAY_BUTTON)
        {
            DestroyWindow(appState.playButton);
            appState.playButton = NULL;

            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            auto windowWidth = clientRect.right - clientRect.left;
            auto windowHeight = clientRect.bottom - clientRect.top;
            if (windowWidth < 320) windowWidth = 320;
            if (windowHeight < 200) windowHeight = 200;
            auto windowRowBytes = (windowWidth + 3) & ~3;
            float factor = windowWidth / 320.f;
            float consoleWidth = 320;
            auto consoleHeight = ceil(windowHeight / factor);
            if (consoleHeight < 200)
            {
                factor = consoleHeight / 200;
                consoleHeight = 200;
                consoleWidth = ceil(consoleWidth / factor);
            }
            auto consoleRowBytes = (float)(((int)ceil(consoleWidth) + 3) & ~3);
            vid_width = (int)windowWidth;
            vid_height = (int)windowHeight;
            vid_rowbytes = (int)windowRowBytes;
            con_width = (int)consoleWidth;
            con_height = (int)consoleHeight;
            con_rowbytes = (int)consoleRowBytes;

            std::vector<std::string> arguments;
            arguments.emplace_back();
            auto word_count = 0;
            for (auto c : appState.commandLine)
            {
                if (c <= ' ')
                {
                    if (word_count == 0 && Q_strcasecmp(arguments[arguments.size() - 1].c_str(), "quake") == 0)
                    {
                        arguments[arguments.size() - 1].clear();
                    }
                    else if (!arguments[arguments.size() - 1].empty())
                    {
                        arguments.emplace_back();
                        word_count++;
                    }
                }
                else
                {
                    arguments[arguments.size() - 1] += c;
                }
            }
            if (arguments[arguments.size() - 1].empty())
            {
                arguments.pop_back();
            }

            appState.engineThread = new std::thread(&Engine::StartEngine, &appState.engine, arguments);

            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    case WM_PAINT:
    {
        if (Locks::StopEngine)
        {
            if (sys_errormessage.length() > 0)
            {
                if (sys_nogamedata)
                {
                    MessageBox(NULL, L"Ensure that the game data files are in the same folder as the app, or check your command line.", L"Slip & Frag - Game data not found", MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY | MB_OK);
                }
                else
                {
                    std::wstring message = L"(The error message could not be converted.)";
                    auto size = MultiByteToWideChar(CP_UTF8, 0, sys_errormessage.c_str(), (int)sys_errormessage.length(), NULL, 0);
                    if (size > 0)
                    {
                        message.resize(size + 16);
                        MultiByteToWideChar(CP_UTF8, 0, sys_errormessage.c_str(), (int)sys_errormessage.length(), message.data(), (int)message.size());
                    }

                    MessageBox(NULL, message.c_str(), L"Slip & Frag - Sys_Error:", MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY | MB_OK);
                }
            }
            PostQuitMessage(0);
        }
        else
        {
            if (host_initialized)
            {
                if (!appState.painting)
                {
                    appState.painting = true;

                    XINPUT_STATE xInputState{ };
                    if (XInputGetState(0, &xInputState) == ERROR_SUCCESS)
                    {
                        if (!appState.xInputEnabled)
                        {
                            appState.xInputEnabled = true;

                            joy_avail = true;

                            Cvar_SetValue("joyadvanced", 1);
                            Cvar_SetValue("joyadvaxisx", AxisSide);
                            Cvar_SetValue("joyadvaxisy", AxisForward);
                            Cvar_SetValue("joyadvaxisz", AxisTurn);
                            Cvar_SetValue("joyadvaxisr", AxisLook);

                            Joy_AdvancedUpdate_f();
                        }
                        HandleXInputState(appState.xInputState, xInputState);
                    }
                    else
                    {
                        xInputState = { };
                        HandleXInputState(appState.xInputState, xInputState);
                        if (appState.xInputEnabled)
                        {
                            appState.xInputEnabled = false;

                            in_forwardmove = 0.0;
                            in_sidestepmove = 0.0;
                            in_rollangle = 0.0;
                            in_pitchangle = 0.0;

                            joy_avail = false;
                        }
                    }
                    appState.xInputState = xInputState;

                    RECT clientRect;
                    GetClientRect(hWnd, &clientRect);
                    auto windowWidth = clientRect.right - clientRect.left;
                    auto windowHeight = clientRect.bottom - clientRect.top;
                    if (windowWidth < 320) windowWidth = 320;
                    if (windowHeight < 200) windowHeight = 200;
                    auto windowRowBytes = (windowWidth + 3) & ~3;
                    float factor = windowWidth / 320.f;
                    float consoleWidth = 320;
                    auto consoleHeight = ceil(windowHeight / factor);
                    if (consoleHeight < 200)
                    {
                        factor = consoleHeight / 200;
                        consoleHeight = 200;
                        consoleWidth = ceil(consoleWidth / factor);
                    }
                    auto consoleRowBytes = (float)(((int)ceil(consoleWidth) + 3) & ~3);

                    PAINTSTRUCT ps;
                    HDC hdc;
                    HDC memdc;
                    HBITMAP bitmap;
                    VOID* bitmapBits;

                    {
                        std::lock_guard<std::mutex> lock(Locks::RenderMutex);

                        if (vid_width != (int)windowWidth || vid_height != (int)windowHeight)
                        {
                            vid_width = (int)windowWidth;
                            vid_height = (int)windowHeight;
                            vid_rowbytes = (int)windowRowBytes;
                            con_width = (int)consoleWidth;
                            con_height = (int)consoleHeight;
                            con_rowbytes = (int)consoleRowBytes;
                            VID_Resize();
                        }
                        hdc = BeginPaint(hWnd, &ps);
                        if (vid_width != appState.screenBitmapInfo.bmiHeader.biWidth ||
                            -vid_height != appState.screenBitmapInfo.bmiHeader.biHeight)
                        {
                            if (appState.screenBitmap)
                            {
                                DeleteObject(appState.screenBitmap);
                            }
                            if (appState.screenDC)
                            {
                                DeleteDC(appState.screenDC);
                            }
                            memdc = CreateCompatibleDC(hdc);
                            appState.screenBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                            appState.screenBitmapInfo.bmiHeader.biPlanes = 1;
                            appState.screenBitmapInfo.bmiHeader.biBitCount = 8;
                            appState.screenBitmapInfo.bmiHeader.biCompression = BI_RGB;
                            appState.screenBitmapInfo.bmiHeader.biWidth = vid_width;
                            appState.screenBitmapInfo.bmiHeader.biHeight = -vid_height;
                            bitmap = CreateDIBSection(memdc, &appState.screenBitmapInfo, DIB_RGB_COLORS, &bitmapBits, NULL, 0);
                            appState.screenDC = memdc;
                            appState.screenBitmap = bitmap;
                            appState.screenBitmapBits = bitmapBits;
                        }
                        else
                        {
                            memdc = appState.screenDC;
                            bitmap = appState.screenBitmap;
                            bitmapBits = appState.screenBitmapBits;
                        }
                        unsigned char* vidSource = vid_buffer.data();
                        unsigned char* conSource = con_buffer.data();
                        auto target = (unsigned char*)bitmapBits;
                        auto stepX = (uint64_t)(consoleWidth * 1024 * 1024 / windowWidth);
                        auto stepY = (uint64_t)(consoleHeight * 1024 * 1024 / windowHeight);
                        auto conRow = conSource;
                        uint64_t posY = 0;
                        uint64_t posYDiv20 = 0;
                        auto previousPosYDiv20 = MAXUINT64;
                        bool hasCon;
                        auto copyCount = 0;
                        for (auto y = 0; y < vid_height; y++)
                        {
                            if (previousPosYDiv20 != posYDiv20)
                            {
                                if (copyCount > 0)
                                {
                                    memcpy(target, vidSource, copyCount);
                                    vidSource += copyCount;
                                    target += copyCount;
                                    copyCount = 0;
                                }
                                hasCon = false;
                                uint64_t posX = 0;
                                for (auto x = 0; x < vid_rowbytes; x += 4)
                                {
                                    auto out = *(uint32_t*)vidSource;
                                    uint32_t con = conRow[posX >> 20];
                                    if (con < 255)
                                    {
                                        out = (out & 0xFFFFFF00) | con;
                                        hasCon = true;
                                    }
                                    posX += stepX;
                                    con = conRow[posX >> 20];
                                    if (con < 255)
                                    {
                                        out = (out & 0xFFFF00FF) | (con << 8);
                                        hasCon = true;
                                    }
                                    posX += stepX;
                                    con = conRow[posX >> 20];
                                    if (con < 255)
                                    {
                                        out = (out & 0xFF00FFFF) | (con << 16);
                                        hasCon = true;
                                    }
                                    posX += stepX;
                                    con = conRow[posX >> 20];
                                    if (con < 255)
                                    {
                                        out = (out & 0xFFFFFF) | (con << 24);
                                        hasCon = true;
                                    }
                                    posX += stepX;
                                    *((uint32_t*)target) = out;
                                    vidSource += 4;
                                    target += 4;
                                }
                                previousPosYDiv20 = posYDiv20;
                            }
                            else if (hasCon)
                            {
                                if (copyCount > 0)
                                {
                                    memcpy(target, vidSource, copyCount);
                                    vidSource += copyCount;
                                    target += copyCount;
                                    copyCount = 0;
                                }
                                uint64_t posX = 0;
                                for (auto x = 0; x < vid_rowbytes; x += 4)
                                {
                                    auto out = *(uint32_t*)vidSource;
                                    uint32_t con = conRow[posX >> 20];
                                    if (con < 255)
                                    {
                                        out = (out & 0xFFFFFF00) | con;
                                    }
                                    posX += stepX;
                                    con = conRow[posX >> 20];
                                    if (con < 255)
                                    {
                                        out = (out & 0xFFFF00FF) | (con << 8);
                                    }
                                    posX += stepX;
                                    con = conRow[posX >> 20];
                                    if (con < 255)
                                    {
                                        out = (out & 0xFF00FFFF) | (con << 16);
                                    }
                                    posX += stepX;
                                    con = conRow[posX >> 20];
                                    if (con < 255)
                                    {
                                        out = (out & 0xFFFFFF) | (con << 24);
                                    }
                                    posX += stepX;
                                    *((uint32_t*)target) = out;
                                    vidSource += 4;
                                    target += 4;
                                }
                            }
                            else
                            {
                                copyCount += vid_rowbytes;
                            }
                            posY += stepY;
                            posYDiv20 = (uint32_t)(posY >> 20);
                            conRow = conSource + posYDiv20 * con_rowbytes;
                        }
                        if (copyCount > 0)
                        {
                            memcpy(target, vidSource, copyCount);
                        }
                    }

                    {
                        std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);

                        for (auto& directRect : DirectRect::directRects)
                        {
                            auto x = directRect.x * (con_width - directRect.width) / (directRect.vid_rowbytes - directRect.width);
                            auto y = directRect.y * (con_height - directRect.width) / (directRect.vid_height - directRect.height);

                            auto left = x * vid_rowbytes / con_width;
                            auto top = y * vid_height / con_height;
                            auto right = (x + directRect.width) * vid_rowbytes / con_width;
                            auto bottom = (y + directRect.height) * vid_height / con_height;

                            auto source = directRect.data;
                            auto target = (unsigned char*)bitmapBits + top * vid_rowbytes + left;

                            auto stepX = (uint64_t)(consoleWidth * 1024 * 1024 / windowWidth);
                            auto stepY = (uint64_t)(consoleHeight * 1024 * 1024 / windowHeight);

                            uint64_t posY = 0;
                            uint64_t prevPosYDiv20 = 0;
                            for (auto v = top; v < bottom; v++)
                            {
                                uint64_t posX = 0;
                                for (auto h = left; h < right; h++)
                                {
                                    *target++ = *(source + (posX >> 20));

                                    posX += stepX;
                                }

                                posY += stepY;
                                auto posYDiv20 = posY >> 20;
                                if (prevPosYDiv20 != posYDiv20)
                                {
                                    source += directRect.width;
                                    prevPosYDiv20 = posYDiv20;
                                }

                                target += (vid_rowbytes - right + left);
                            }
                        }
                    }

                    auto previous = SelectObject(memdc, bitmap);
                    SetDIBColorTable(memdc, 0, 256, (RGBQUAD*)d_8to24table);
                    BitBlt(hdc, 0, 0, vid_width, vid_height, memdc, 0, 0, SRCCOPY);
                    SelectObject(memdc, previous);
                    EndPaint(hWnd, &ps);

                    appState.painting = false;
                }
            }

            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    }
    case WM_CTLCOLORBTN:
        return (INT_PTR)appState.backgroundBrush;
    case WM_DRAWITEM:
    {
        auto dis = (DRAWITEMSTRUCT*)lParam;
        if (dis->CtlID == IDC_PLAY_BUTTON)
        {
            auto dpi = GetDpiForWindow(hWnd);
            auto halfWidth = 75 * dpi / USER_DEFAULT_SCREEN_DPI;
            auto halfHeight = halfWidth;
            auto width = halfWidth * 2;
            auto height = halfHeight * 2;
            Bitmap backBuffer(width, height);
            Graphics graphicsBackBuffer(&backBuffer);
            graphicsBackBuffer.SetSmoothingMode(SmoothingModeAntiAlias);
            SolidBrush brush(Color(appState.backgroundR, appState.backgroundG, appState.backgroundB));
            graphicsBackBuffer.FillRectangle(&brush, 0, 0, width, height);
            Pen pen(Color(128, 128, 128), 3 * dpi / USER_DEFAULT_SCREEN_DPI);
            BYTE types[]{ PathPointTypeLine, PathPointTypeLine, PathPointTypeLine, PathPointTypeLine | PathPointTypeCloseSubpath };
            if ((dis->itemState & ODS_SELECTED) == ODS_SELECTED)
            {
                graphicsBackBuffer.DrawEllipse(&pen, 
                    (INT)(3 * dpi / USER_DEFAULT_SCREEN_DPI), 
                    (INT)(3 * dpi / USER_DEFAULT_SCREEN_DPI),
                    (INT)(width - 6 * dpi / USER_DEFAULT_SCREEN_DPI),
                    (INT)(height - 6 * dpi / USER_DEFAULT_SCREEN_DPI));
                Point points[] { 
                    Point(56 * dpi / USER_DEFAULT_SCREEN_DPI, 46 * dpi / USER_DEFAULT_SCREEN_DPI),
                    Point(104 * dpi / USER_DEFAULT_SCREEN_DPI, halfHeight),
                    Point(56 * dpi / USER_DEFAULT_SCREEN_DPI, 104 * dpi / USER_DEFAULT_SCREEN_DPI),
                    Point(56 * dpi / USER_DEFAULT_SCREEN_DPI, 46 * dpi / USER_DEFAULT_SCREEN_DPI)
                };
                GraphicsPath path(points, types, 4);
                graphicsBackBuffer.DrawPath(&pen, &path);
            }
            else
            {
                graphicsBackBuffer.DrawEllipse(&pen,
                    (INT)(2 * dpi / USER_DEFAULT_SCREEN_DPI),
                    (INT)(2 * dpi / USER_DEFAULT_SCREEN_DPI),
                    (INT)(width - 4 * dpi / USER_DEFAULT_SCREEN_DPI),
                    (INT)(height - 4 * dpi / USER_DEFAULT_SCREEN_DPI));
                Point points[] {
                    Point(55 * dpi / USER_DEFAULT_SCREEN_DPI, 45 * dpi / USER_DEFAULT_SCREEN_DPI),
                    Point(105 * dpi / USER_DEFAULT_SCREEN_DPI, halfHeight),
                    Point(55 * dpi / USER_DEFAULT_SCREEN_DPI, 105 * dpi / USER_DEFAULT_SCREEN_DPI),
                    Point(55 * dpi / USER_DEFAULT_SCREEN_DPI, 45 * dpi / USER_DEFAULT_SCREEN_DPI)
                };
                GraphicsPath path(points, types, 4);
                graphicsBackBuffer.DrawPath(&pen, &path);
            }
            Graphics graphics(dis->hDC);
            graphics.DrawImage(&backBuffer, (INT)(dis->rcItem.left - 1), (INT)(dis->rcItem.top - 1));
        }
        break;
    }
    case WM_KEYDOWN:
    {
        if (host_initialized && !Locks::StopEngine)
        {
            auto mapped = 0;
            if (wParam >= 0 && wParam < sizeof(virtualkeymap) / sizeof(int))
            {
                mapped = virtualkeymap[wParam];
            }
            if (mapped == 0)
            {
                auto final = MapVirtualKeyA(wParam, 2); // == MAPVK_VK_TO_CHAR
                if (final > 255) final = 255;
                Input::AddKeyInput(final, true);
            }
            else
            {
                Input::AddKeyInput(mapped, true);
            }
        }
        break;
    }
    case WM_KEYUP:
    {
        if (host_initialized && !Locks::StopEngine)
        {
            auto mapped = 0;
            if (wParam >= 0 && wParam < sizeof(virtualkeymap) / sizeof(int))
            {
                mapped = virtualkeymap[wParam];
            }
            if (mapped == 0)
            {
                auto final = MapVirtualKeyA(wParam, 2); // == MAPVK_VK_TO_CHAR
                if (final > 255) final = 255;
                Input::AddKeyInput(final, false);
            }
            else
            {
                Input::AddKeyInput(mapped, false);
            }
        }
        break;
    }
    case WM_LBUTTONDOWN:
        if (host_initialized && !Locks::StopEngine)
        {
            Input::AddKeyInput(K_MOUSE1, true);
        }
        break;
    case WM_LBUTTONUP:
        if (host_initialized && !Locks::StopEngine)
        {
            Input::AddKeyInput(K_MOUSE1, false);
        }
        break;
    case WM_RBUTTONDOWN:
        if (host_initialized && !Locks::StopEngine)
        {
            Input::AddKeyInput(K_MOUSE2, true);
        }
        break;
    case WM_RBUTTONUP:
        if (host_initialized && !Locks::StopEngine)
        {
            Input::AddKeyInput(K_MOUSE2, false);
        }
        break;
    case WM_MBUTTONDOWN:
        if (host_initialized && !Locks::StopEngine)
        {
            Input::AddKeyInput(K_MOUSE3, true);
        }
        break;
    case WM_MBUTTONUP:
        if (host_initialized && !Locks::StopEngine)
        {
            Input::AddKeyInput(K_MOUSE3, false);
        }
        break;
    case WM_MOUSEWHEEL:
        if (host_initialized && !Locks::StopEngine)
        {
            auto clicks = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            while (clicks > 0)
            {
                Input::AddKeyInput(K_MWHEELUP, true);
                Input::AddKeyInput(K_MWHEELUP, false);
                clicks--;
            }
            while (clicks < 0)
            {
                Input::AddKeyInput(K_MWHEELDOWN, true);
                Input::AddKeyInput(K_MWHEELDOWN, false);
                clicks++;
            }
        }
        break;
    case WM_SETCURSOR:
        if (host_initialized && !Locks::StopEngine && key_dest == key_game && GetForegroundWindow() == appState.hWnd)
        {
            SetCursor(NULL);
            return TRUE;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    case MM_WOM_DONE:
        SNDDMA_Callback((void*)wParam, (void*)lParam);
        CDAudio_Callback((void*)wParam, (void*)lParam);
        break;
    case WM_CLOSE:
        Locks::StopEngine = true;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    appState.windowClass = L"SlipNFrag";
    appState.windowTitle = L"Slip & Frag";
    appState.instance = hInstance;

    std::string converted_cmdline;
    auto size = WideCharToMultiByte(CP_UTF8, 0, lpCmdLine, -1, NULL, 0, NULL, NULL);
    if (size > 0)
    {
        converted_cmdline.resize(size + 16);
        WideCharToMultiByte(CP_UTF8, 0, lpCmdLine, -1, converted_cmdline.data(), converted_cmdline.size(), NULL, NULL);
    }

    appState.commandLine = std::string("slipnfrag.exe ") + converted_cmdline;

    appState.usesDarkMode = IsDarkThemeActive();
    if (appState.usesDarkMode)
    {
        appState.backgroundR = 0;
        appState.backgroundG = 0;
        appState.backgroundB = 0;
        appState.backgroundBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    }
    else
    {
        appState.backgroundR = 255;
        appState.backgroundG = 255;
        appState.backgroundB = 255;
        appState.backgroundBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    }

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASSEXW wcex { };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = appState.backgroundBrush;
    wcex.lpszClassName = appState.windowClass.c_str();

    RegisterClassExW(&wcex);

    auto hwnd = CreateWindowW(appState.windowClass.c_str(), appState.windowTitle.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, appState.instance, nullptr);
    if (hwnd == NULL)
    {
        return 0;
    }

    appState.hWnd = hwnd;

    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &appState.usesDarkMode, sizeof(appState.usesDarkMode));

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);

    return (int)msg.wParam;
}
