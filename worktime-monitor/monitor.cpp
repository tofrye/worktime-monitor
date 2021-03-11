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
#define INACTIVITY_PERIOD 300.0

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

double getSecondsSinceUserInput() {
    DWORD te;
    te = ::GetTickCount(); //number of milliseconds that have elapsed since the system was started
    LASTINPUTINFO li;
    li.cbSize = sizeof(LASTINPUTINFO);
    ::GetLastInputInfo(&li); //tickcount of last user input
    //seconds since last user input
    return (te - li.dwTime) / 1000.0;
}

SYSTEMTIME sub(SYSTEMTIME st, double seconds) {
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);

    ULARGE_INTEGER u;
    memcpy(&u, &ft, sizeof(u));

    const double secondsPer100nsInterval = 100. * 1.E-9;
    u.QuadPart -= seconds / secondsPer100nsInterval;

    memcpy(&ft, &u, sizeof(ft));

    FileTimeToSystemTime(&ft, &st);
    return st;
}

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
        MessageBox(NULL, _T("Call to RegisterClassEx failed!"), _T("Windows Desktop Guided Tour"), NULL);
        return 1;
    }

    // Store instance handle in our global variable
    hInst = hInstance;

    // The parameters to CreateWindow explained:
    HWND hWnd = CreateWindow(
        szWindowClass,                  // szWindowClass: the name of the application
        szTitle,                        // szTitle: the text that appears in the title bar
        WS_OVERLAPPEDWINDOW,            // WS_OVERLAPPEDWINDOW: the type of window to create
        CW_USEDEFAULT, CW_USEDEFAULT,   // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
        500, 400,                       // 500, 100: initial size (width, length)
        NULL,                           // NULL: the parent of this window
        NULL,                           // NULL: this application does not have a menu bar
        hInstance,                      // hInstance: the first parameter from WinMain
        NULL                            // NULL: not used in this application
    );

    if (!hWnd)
    {
        MessageBox(NULL, _T("Call to CreateWindow failed!"), _T("Windows Desktop Guided Tour"), NULL);
        return 1;
    }

    // The parameters to ShowWindow explained:
    // hWnd: the value returned from CreateWindow
    // nCmdShow: the fourth parameter from WinMain
    ShowWindow(hWnd, nCmdShow);
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

    double elapsed = getSecondsSinceUserInput();

    if (working) {
        if (elapsed >= INACTIVITY_PERIOD) {
            SYSTEMTIME now;
            GetLocalTime(&now);
            SYSTEMTIME nowMinusInactivityPeriod = sub(now, INACTIVITY_PERIOD);
            times_end.emplace_back(nowMinusInactivityPeriod);
            working = false;
        }
    } else {
        if (elapsed <= INACTIVITY_PERIOD) {
            SYSTEMTIME now;
            GetLocalTime(&now);
            times_start.emplace_back(now);
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
        TextOut(hdc, 5, 5, _T("Currently: "), _tcslen(_T("Currently: ")));
        if (working) TextOut(hdc, 100, 5, _T("working"), _tcslen(_T("working")));
        else TextOut(hdc, 100, 5, _T("not working"), _tcslen(_T("not working")));

        // Starting times
        TextOut(hdc, 5, 20, _T("Start:"), _tcslen(_T("Start:")));
        int accumulated_y = 35;
        //for (auto time : times_start) {
        for (int i = 0; i < times_start.size(); i++) {
            //if (time_diff(times_start.at(i), times_end.at(i)) /* || print_list_without_filtering_short_periods */) {
                auto time = times_start.at(i);
                std::wstringstream ss_start(L"");
                ss_start << std::setw(2) << std::setfill(L'0') << time.wHour << ":" << std::setw(2) << std::setfill(L'0') << time.wMinute;
                TextOut(hdc, 5, accumulated_y, ss_start.str().c_str(), _tcslen(ss_start.str().c_str()));
                accumulated_y += 15;
            //}
        }
        // Ending times
        TextOut(hdc, 100, 20, _T("End:"), _tcslen(_T("End:")));
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

int time_diff(SYSTEMTIME time_start, SYSTEMTIME time_end) {
    //time_start - time_end
    return 1;
}
