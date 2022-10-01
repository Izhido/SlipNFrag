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

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#define IDC_START_BUTTON 100

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
        auto left = (clientRect.left + clientRect.right) / 2 - 50;
        auto top = (clientRect.top + clientRect.bottom) / 2 - 50;
        appState.startButton = CreateWindow(L"BUTTON", L"Start", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, left, top, 100, 100, hWnd, (HMENU)IDC_START_BUTTON, appState.instance, NULL);
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
        if (appState.startButton != NULL)
        {
            auto width = LOWORD(lParam);
            auto height = HIWORD(lParam);
            auto left = width / 2 - 50;
            auto top = height / 2 - 50;
            SetWindowPos(appState.startButton, NULL, left, top, 100, 100, 0);
        }
        break;
    }
    case WM_COMMAND:
        if (wParam == IDC_START_BUTTON)
        {
            DestroyWindow(appState.startButton);
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
            sys_argc = (int)arguments.size();
            sys_argv = new char* [sys_argc];
            for (auto i = 0; i < arguments.size(); i++)
            {
                sys_argv[i] = new char[arguments[i].length() + 1];
                strcpy(sys_argv[i], arguments[i].c_str());
            }
            sys_version = "Win64 1.0.21";
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
            Sys_Init(sys_argc, sys_argv);
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
                PostQuitMessage(0);
            }
            appState.started = true;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    case WM_PAINT:
    {
        if (appState.started)
        {
            if (!appState.painting)
            {
                appState.painting = true;
                if (appState.previousTime.QuadPart == 0)
                {
                    QueryPerformanceCounter(&appState.previousTime);
                }
                else
                {
                    LARGE_INTEGER time;
                    QueryPerformanceCounter(&time);
                    LARGE_INTEGER frequency;
                    QueryPerformanceFrequency(&frequency);
                    auto elapsed = (float)(time.QuadPart - appState.previousTime.QuadPart) / (float)frequency.QuadPart;
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
                    if (r_cache_thrash)
                    {
                        VID_ReallocSurfCache();
                    }
                    Sys_Frame(elapsed);
                    if (sys_errorcalled || sys_quitcalled)
                    {
                        if (sys_errormessage.length() > 0)
                        {
                            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                            MessageBox(NULL, converter.from_bytes(sys_errormessage).c_str(), L"Slip & Frag - Sys_Error:", MB_ICONERROR | MB_DEFAULT_DESKTOP_ONLY | MB_OK);
                        }
                        appState.started = false;
                        PostQuitMessage(0);
                        break;
                    }
                    PAINTSTRUCT ps;
                    auto hdc = BeginPaint(hWnd, &ps);
                    auto memdc = CreateCompatibleDC(hdc);
                    BITMAPINFO bitmapInfo { };
                    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    bitmapInfo.bmiHeader.biPlanes = 1;
                    bitmapInfo.bmiHeader.biBitCount = 8;
                    bitmapInfo.bmiHeader.biCompression = BI_RGB;
                    bitmapInfo.bmiHeader.biWidth = vid_width;
                    bitmapInfo.bmiHeader.biHeight = -vid_height;
                    VOID* bitmapBits;
                    auto bitmap = CreateDIBSection(memdc, &bitmapInfo, DIB_RGB_COLORS, &bitmapBits, NULL, 0);
                    unsigned char* vidSource = vid_buffer.data();
                    unsigned char* conSource = con_buffer.data();
                    auto target = (unsigned char*)bitmapBits;
                    auto stepX = (uint64_t)(consoleWidth * 1024 * 1024 / windowWidth);
                    auto stepY = (uint64_t)(consoleHeight * 1024 * 1024 / windowHeight);
                    auto conRow = conSource;
                    uint64_t posY = 0;
                    uint32_t posYDiv20 = 0;
                    auto previousPosYDiv20 = -1;
                    auto vidTrailing = vid_rowbytes - vid_width;
                    bool hasCon;
                    for (auto y = 0; y < vid_height; y++)
                    {
                        if (previousPosYDiv20 != posYDiv20)
                        {
                            hasCon = false;
                            uint64_t posX = 0;
                            for (auto x = 0; x < vid_width; x++)
                            {
                                auto con = conRow[posX >> 20];
                                if (con > 0 && con < 255)
                                {
                                    *target++ = con;
                                    hasCon = true;
                                }
                                else
                                {
                                    *target++ = *vidSource;
                                }
                                posX += stepX;
                                vidSource++;
                            }
                            previousPosYDiv20 = posYDiv20;
                            vidSource += vidTrailing;
                            target += vidTrailing;
                        }
                        else if (hasCon)
                        {
                            uint64_t posX = 0;
                            for (auto x = 0; x < vid_width; x++)
                            {
                                auto con = conRow[posX >> 20];
                                if (con > 0 && con < 255)
                                {
                                    *target++ = con;
                                }
                                else
                                {
                                    *target++ = *vidSource;
                                }
                                posX += stepX;
                                vidSource++;
                            }
                            vidSource += vidTrailing;
                            target += vidTrailing;
                        }
                        else
                        {
                            memcpy(target, vidSource, vid_rowbytes);
                            vidSource += vid_rowbytes;
                            target += vid_rowbytes;
                        }
                        posY += stepY;
                        posYDiv20 = posY >> 20;
                        conRow = conSource + posYDiv20 * con_rowbytes;
                    }
                    auto previous = SelectObject(memdc, bitmap);
                    SetDIBColorTable(memdc, 0, 256, (RGBQUAD*)d_8to24table);
                    BitBlt(hdc, 0, 0, vid_width, vid_height, memdc, 0, 0, SRCCOPY);
                    SelectObject(memdc, previous);
                    DeleteObject(bitmap);
                    DeleteDC(memdc);
                    EndPaint(hWnd, &ps);
                    appState.previousTime = time;
                }
                InvalidateRect(hWnd, NULL, FALSE);
                appState.painting = false;
            }
        }
        break;
    }
    case WM_KEYDOWN:
        Key_Event(virtualkeymap[wParam], true);
        break;
    case WM_KEYUP:
        Key_Event(virtualkeymap[wParam], false);
        break;
    case WM_LBUTTONDOWN:
        Key_Event(K_MOUSE1, true);
        break;
    case WM_LBUTTONUP:
        Key_Event(K_MOUSE1, false);
        break;
    case WM_RBUTTONDOWN:
        Key_Event(K_MOUSE2, true);
        break;
    case WM_RBUTTONUP:
        Key_Event(K_MOUSE2, false);
        break;
    case WM_MBUTTONDOWN:
        Key_Event(K_MOUSE3, true);
        break;
    case WM_MBUTTONUP:
        Key_Event(K_MOUSE3, false);
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

    WNDCLASSEXW wcex { };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
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

    return (int)msg.wParam;
}
