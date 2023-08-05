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

using namespace Gdiplus;

#pragma comment (lib,"Gdiplus.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#define IDC_PLAY_BUTTON 100

AppState appState { };

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
        appState.nonClientWidth = (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
        appState.nonClientHeight = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);
        auto left = (clientRect.left + clientRect.right) / 2 - 75;
        auto top = (clientRect.top + clientRect.bottom) / 2 - 75;
        appState.playButton = CreateWindow(L"BUTTON", L"Play", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, left, top, 150, 150, hWnd, (HMENU)IDC_PLAY_BUTTON, appState.instance, NULL);
        break;
    }
    case WM_GETMINMAXINFO:
    {
        auto lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = 320 + appState.nonClientWidth;
        lpMMI->ptMinTrackSize.y = 200 + appState.nonClientHeight;
        break;
    }
    case WM_SIZE:
    {
        if (appState.playButton != NULL)
        {
            auto width = LOWORD(lParam);
            auto height = HIWORD(lParam);
            if (width > 0 && height > 0)
            {
                auto left = width / 2 - 75;
                auto top = height / 2 - 75;
                SetWindowPos(appState.playButton, NULL, left, top, 150, 150, 0);
                UpdateWindow(appState.playButton);
            }
        }
        break;
    }
    case WM_COMMAND:
        if (wParam == IDC_PLAY_BUTTON)
        {
            DestroyWindow(appState.playButton);

            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            float windowWidth = clientRect.right - clientRect.left;
            float windowHeight = clientRect.bottom - clientRect.top;
            if (windowWidth < 320) windowWidth = 320;
            if (windowHeight < 200) windowHeight = 200;
            auto windowRowBytes = (float)(((int)ceil(windowWidth) + 3) & ~3);
            auto factor = windowWidth / 320;
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
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                if (sys_nogamedata)
                {
                    MessageBox(NULL, L"Ensure that the game data files are in the same folder as the app, or check your command line.", L"Slip & Frag - Game data not found", MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY | MB_OK);
                }
                else
                {
                    MessageBox(NULL, converter.from_bytes(sys_errormessage).c_str(), L"Slip & Frag - Sys_Error:", MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY | MB_OK);
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

                    RECT clientRect;
                    GetClientRect(hWnd, &clientRect);
                    float windowWidth = clientRect.right - clientRect.left;
                    float windowHeight = clientRect.bottom - clientRect.top;
                    if (windowWidth < 320) windowWidth = 320;
                    if (windowHeight < 200) windowHeight = 200;
                    auto windowRowBytes = (float)(((int)ceil(windowWidth) + 3) & ~3);
                    auto factor = windowWidth / 320;
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
                        memdc = CreateCompatibleDC(hdc);
                        BITMAPINFO bitmapInfo { };
                        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                        bitmapInfo.bmiHeader.biPlanes = 1;
                        bitmapInfo.bmiHeader.biBitCount = 8;
                        bitmapInfo.bmiHeader.biCompression = BI_RGB;
                        bitmapInfo.bmiHeader.biWidth = vid_width;
                        bitmapInfo.bmiHeader.biHeight = -vid_height;
                        bitmap = CreateDIBSection(memdc, &bitmapInfo, DIB_RGB_COLORS, &bitmapBits, NULL, 0);
                        unsigned char* vidSource = vid_buffer.data();
                        unsigned char* conSource = con_buffer.data();
                        auto target = (unsigned char*)bitmapBits;
                        auto stepX = (uint64_t)(consoleWidth * 1024 * 1024 / windowWidth);
                        auto stepY = (uint64_t)(consoleHeight * 1024 * 1024 / windowHeight);
                        auto conRow = conSource;
                        uint64_t posY = 0;
                        uint32_t posYDiv20 = 0;
                        auto previousPosYDiv20 = -1;
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
                            posYDiv20 = posY >> 20;
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

                            auto posY = 0;
                            auto prevPosYDiv20 = 0;
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
                    DeleteObject(bitmap);
                    DeleteDC(memdc);
                    EndPaint(hWnd, &ps);

                    appState.painting = false;
                }
            }

            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    }
    case WM_DRAWITEM:
    {
        auto dis = (DRAWITEMSTRUCT*)lParam;
        if (dis->CtlID == IDC_PLAY_BUTTON)
        {
            Graphics graphics(dis->hDC);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
            SolidBrush brush(Color(0, 0, 0));
            graphics.FillRectangle(&brush, 
                (INT)(dis->rcItem.left - 1),
                (INT)(dis->rcItem.top - 1),
                (INT)(dis->rcItem.right - dis->rcItem.left + 1),
                (INT)(dis->rcItem.bottom - dis->rcItem.top + 1));
            Pen pen(Color(128, 128, 128), 3);
            BYTE types[]{ PathPointTypeLine, PathPointTypeLine, PathPointTypeLine, PathPointTypeLine | PathPointTypeCloseSubpath };
            if ((dis->itemState & ODS_SELECTED) == ODS_SELECTED)
            {
                graphics.DrawEllipse(&pen, 3, 3, 145, 145);
                Point points[] { Point(56, 46), Point(104, 75), Point(56, 104), Point(56, 46) };
                GraphicsPath path(points, types, 4);
                graphics.DrawPath(&pen, &path);
            }
            else
            {
                graphics.DrawEllipse(&pen, 2, 2, 147, 147);
                Point points[] { Point(55, 45), Point(105, 75), Point(55, 105), Point(55, 45) };
                GraphicsPath path(points, types, 4);
                graphics.DrawPath(&pen, &path);
            }
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

    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    appState.commandLine = std::string("slipnfrag.exe ") + converter.to_bytes(lpCmdLine);

    appState.usesDarkMode = TRUE;

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
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
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
