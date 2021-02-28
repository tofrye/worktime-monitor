// monitor.cpp
// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <winuser.h> //GetLastInputInfo
#include <iostream> //cout
#include <sstream> //stringstream
#include <vector>
#include <iomanip>

#define SECOND_TIMER 1000

// Global variables

// The main window class name.
static TCHAR szWindowClass[] = _T("DesktopApp");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Worktime Monitor");

HINSTANCE hInst;

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Own definitions
bool oninit = true;
bool working = false;
std::vector<SYSTEMTIME> times_start;
std::vector<SYSTEMTIME> times_end;

int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }

    // Store instance handle in our global variable
    hInst = hInstance;

    // The parameters to CreateWindow explained:
    // szWindowClass: the name of the application
    // szTitle: the text that appears in the title bar
    // WS_OVERLAPPEDWINDOW: the type of window to create
    // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
    // 500, 100: initial size (width, length)
    // NULL: the parent of this window
    // NULL: this application does not have a menu bar
    // hInstance: the first parameter from WinMain
    // NULL: not used in this application
    HWND hWnd = CreateWindow(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 400,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd)
    {
        MessageBox(NULL,
            _T("Call to CreateWindow failed!"),
            _T("Windows Desktop Guided Tour"),
            NULL);

        return 1;
    }

    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(hWnd,
        nCmdShow);
    UpdateWindow(hWnd);

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    if (oninit) {
        SetTimer(hWnd, SECOND_TIMER, SECOND_TIMER, NULL);
        oninit = false;
    }

    //if working==false and elapsed <= 300s:
    //  store st as starting time
    //  set working to true
    //if elapsed >= 300s:
    //  store st as ending time
    //  set working to false

    DWORD te;
    te = ::GetTickCount(); //number of milliseconds that have elapsed since the system was started
    LASTINPUTINFO li;
    li.cbSize = sizeof(LASTINPUTINFO);
    ::GetLastInputInfo(&li); //tickcount of last user input
    //seconds since last user input
    double elapsed = (te - li.dwTime) / 1000.0;

    if (working) {
        if (elapsed >= 300.0) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            times_end.emplace_back(st);
            working = false;
        }
    } else {
        if (elapsed <= 300.0) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            times_start.emplace_back(st);
            working = true;
        }
    }

    switch (message)
    {
    //case WM_COMMAND/WM_INITDIALOG:
    //    SetTimer(hWnd, SECOND_TIMER, SECOND_TIMER, NULL);
    //    break;
    case WM_TIMER:
        if (wParam == SECOND_TIMER)
        {
            InvalidateRect(hWnd, NULL, FALSE);   // invalidate whole window
        }
        break;
    case WM_PAINT:
    {
        hdc = BeginPaint(hWnd, &ps);

        // Application printouts
        TCHAR currently[] = _T("Currently: ");
        TCHAR working[] = _T("working");
        TCHAR not_working[] = _T("not working");
        std::wstringstream ss_elapsed(L"");
        ss_elapsed << " since " << elapsed << "s";
        TextOut(hdc, 5, 5, currently, _tcslen(currently));
        if (working) TextOut(hdc, 100, 5, working, _tcslen(working));
        else TextOut(hdc, 100, 5, not_working, _tcslen(currently));
        TextOut(hdc, 200, 5, ss_elapsed.str().c_str(), _tcslen(ss_elapsed.str().c_str()));

        // Starting times
        TCHAR start[] = _T("Start:");
        TextOut(hdc, 5, 20, start, _tcslen(start));
        int accumulated_y = 35;
        for (auto time : times_start) {
            std::wstringstream ss_start(L"");
            ss_start << std::setw(2) << std::setfill(L'0') << time.wHour << ":" << std::setw(2) << std::setfill(L'0') << time.wMinute;
            TextOut(hdc,5, accumulated_y,ss_start.str().c_str(), _tcslen(ss_start.str().c_str()));
            accumulated_y += 15;
        }
        // Ending times
        TCHAR end[] = _T("End:");
        TextOut(hdc, 100, 20, end, _tcslen(end));
        TCHAR greeting[] = _T("End:");
        accumulated_y = 35;
        for (auto time : times_end) {
            std::wstringstream ss_end(L"");
            ss_end << std::setw(2) << std::setfill(L'0') << time.wHour << ":" << std::setw(2) << std::setfill(L'0') << time.wMinute;
            TextOut(hdc, 100, accumulated_y, ss_end.str().c_str(), _tcslen(ss_end.str().c_str()));
            accumulated_y += 15;
        }
        // End application-specific layout section.

        EndPaint(hWnd, &ps);
    }
        break;
    case WM_DESTROY:
        KillTimer(hWnd, SECOND_TIMER);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}

/*double getSystemIdleTime()
{
    int systemUptime = Environment::TickCount;
    int idleTicks = 0;
    LASTINPUTINFO lastInputInfo;
    lastInputInfo.cbSize = (UInt32)Marshal::SizeOf(lastInputInfo);
    lastInputInfo.dwTime = 0;
    if (GetLastInputInfo(&lastInputInfo))
    {
        int lastInputTicks = (int)lastInputInfo.dwTime;
        idleTicks = systemUptime - lastInputTicks;
    }
    return ((idleTicks > 0) ? (idleTicks / 1000) : idleTicks);
}*/
