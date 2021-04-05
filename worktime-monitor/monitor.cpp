// monitor.cpp
// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <winuser.h> //GetLastInputInfo
#include <iostream> //cout
#include <sstream> //stringstream, std::getline
#include <fstream> //filestream
#include <vector>
#include <iomanip> //setfill etc
#include <cmath> //fmod
#include <Shellapi.h> //ShellExecute

#define TIMER_10S 10000
#define INACTIVITY_PERIOD 600.0
#define SHORT_WORK_PERIOD 120.0

#define TRAYICONID	IDI_ICON1//				ID number for the Notify Icon
#define SWM_TRAYMSG	WM_APP//		the message ID sent to our window
#define SWM_SHOW	WM_APP + 1//	show the window
#define SWM_HIDE	WM_APP + 2//	hide the window
#define SWM_FOLDER  WM_APP + 3//    show the folder of logfiles
#define SWM_EXIT	WM_APP + 4//	close the window


// Global variables

// The main window class name.
static TCHAR szWindowClass[] = _T("DesktopApp");

// The string that appears in the application's title bar.
static TCHAR szTitle[] = _T("Worktime Monitor");

HINSTANCE       hInst;  //current instance
NOTIFYICONDATA  niData; //notify icon data

// Forward declarations of functions included in this code module:
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ShowContextMenu(HWND hWnd);
void CreateTrayIcon(HWND hWnd);

// Own definitions
bool oninit = true;
bool working = false;
std::vector<SYSTEMTIME> times_start;
std::vector<SYSTEMTIME> times_end;

class cTimerHelper {

public:
    std::string todayToFilename() {
        SYSTEMTIME now;
        GetLocalTime(&now);

        //Filename
        std::ostringstream os_year;  os_year << std::setw(2) << std::setfill('0') << now.wYear;
        std::ostringstream os_month; os_month << std::setw(2) << std::setfill('0') << now.wMonth;
        std::ostringstream os_day;   os_day << std::setw(2) << std::setfill('0') << now.wDay;
        return std::string("worktime-log_") + os_year.str() + std::string("-") + os_month.str() + "-" + os_day.str() + std::string(".txt");
    }

    SYSTEMTIME stringToSystemtime(std::string input) {
        //expects input of type hh:mm
        std::string hours = input.substr(0, input.find(":"));
        int h = std::stoi(hours);
        std::string minutes = input.substr(input.find(":") + 1);
        int m = std::stoi(minutes);

        SYSTEMTIME output;
        GetLocalTime(&output);
        output.wHour = h;
        output.wMinute = m;

        return output;
    }

    void checkTodaysWorkingTimes() {
        std::string filename = todayToFilename();
        std::ifstream logfile;
        logfile.open(filename, std::ios_base::in);
        if (!logfile) {
            return;
        } else {
            std::string line;
            while (std::getline(logfile, line)) {
                if (line.rfind("Start", 0) == 0) {
                    continue;
                } else if (line.rfind("->", 0) == 0) {
                    continue;
                } else {
                    std::string time1 = line.substr(0, 5); //start: hh:mm
                    std::string time2 = line.substr(line.find("- ")+2, 5); //end: hh:mm
                    times_start.emplace_back(stringToSystemtime(time1));
                    times_end.emplace_back(stringToSystemtime(time2));
                }
            }
        }
    }

    double getSecondsSinceUserInput() {
        DWORD te;
        te = ::GetTickCount(); //number of milliseconds that have elapsed since the system was started
        LASTINPUTINFO li;
        li.cbSize = sizeof(LASTINPUTINFO);
        ::GetLastInputInfo(&li); //tickcount of last user input
        //seconds since last user input
        return (te - li.dwTime) / 1000.0;
    }

    SYSTEMTIME subSecondsFromSystemtime(SYSTEMTIME st, double seconds) {
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

    double diffSystemtimes(SYSTEMTIME st1, SYSTEMTIME st2) {
        FILETIME ft1, ft2;
        SystemTimeToFileTime(&st1, &ft1);
        SystemTimeToFileTime(&st2, &ft2);

        ULARGE_INTEGER ul1, ul2;
        ul1.LowPart = ft1.dwLowDateTime;
        ul1.HighPart = ft1.dwHighDateTime;
        ul2.LowPart = ft2.dwLowDateTime;
        ul2.HighPart = ft2.dwHighDateTime;

        ul2.QuadPart -= ul1.QuadPart;

        ULARGE_INTEGER uliRetValue;
        uliRetValue.QuadPart = 0;
        uliRetValue = ul2;
        double result = uliRetValue.QuadPart / 10000000.0; //seconds
        if (result > 1.E6) return 0.0;
        else return result;
    }

    bool drawWorkingTimes(HDC hdc) {
        TextOut(hdc, 5, 5, _T("Date: "), _tcslen(_T("Date: ")));
        SYSTEMTIME now;
        GetLocalTime(&now);
        std::wstringstream ss_date(L"");
        ss_date << std::setw(2) << std::setfill(L'0') << now.wDay << "." << std::setw(2) << std::setfill(L'0') << now.wMonth << "." << std::setw(2) << std::setfill(L'0') << now.wYear;
        TextOut(hdc, 100, 5, ss_date.str().c_str(), _tcslen(ss_date.str().c_str()));

        TextOut(hdc, 5, 20, _T("Currently: "), _tcslen(_T("Currently: ")));
        if (working) TextOut(hdc, 100, 20, _T("working       "), _tcslen(_T("working       ")));
        else TextOut(hdc, 100, 20, _T("not working"), _tcslen(_T("not working")));

        // Starting times
        TextOut(hdc, 5, 50, _T("Start:   -"), _tcslen(_T("Start:   -")));
        TextOut(hdc, 60, 50, _T("End:"), _tcslen(_T("End:")));
        TextOut(hdc, 120, 50, _T("(Duration):"), _tcslen(_T("(Duration):")));

        int accumulated_y = 65;
        double work_total_s = 0.0;

        for (int i = 0; i < times_start.size(); i++) {
            SYSTEMTIME time_start = times_start.at(i);
            SYSTEMTIME time_end;

            bool long_work_period = true;
            //Ending Times and Duration
            if (i < times_end.size()) {
                time_end = times_end.at(i);
                if (diffSystemtimes(time_start, time_end) < SHORT_WORK_PERIOD) {
                    long_work_period = false;
                }
                if (long_work_period /* || print_short_work_periods */) {
                    std::wstringstream ss_end(L"");
                    ss_end << std::setw(2) << std::setfill(L'0') << time_end.wHour << ":" << std::setw(2) << std::setfill(L'0') << time_end.wMinute;
                    TextOut(hdc, 60, accumulated_y, ss_end.str().c_str(), _tcslen(ss_end.str().c_str()));

                    std::wstringstream ss_period(L"");
                    double work_s = diffSystemtimes(times_start.at(i), time_end);
                    work_total_s += work_s;
                    int work_h = (int)(work_s / 3600.0);
                    int work_m = (int)(std::fmod(work_s, 3600) / 60.0);
                    ss_period << "(" << std::setfill(L'0') << work_h << ":" << std::setw(2) << std::setfill(L'0') << work_m << "h)";
                    TextOut(hdc, 120, accumulated_y, ss_period.str().c_str(), _tcslen(ss_period.str().c_str()));
                }
            }
            //Starting Times
            if (long_work_period /* || print_short_work_periods */) {
                std::wstringstream ss_start(L"");
                ss_start << std::setw(2) << std::setfill(L'0') << time_start.wHour << ":" << std::setw(2) << std::setfill(L'0') << time_start.wMinute << "  -";
                TextOut(hdc, 5, accumulated_y, ss_start.str().c_str(), _tcslen(ss_start.str().c_str()));
                accumulated_y += 15;
            }
        }

        //If currently working (no ending time) calculate current working time up to current system time
        if (times_end.size() < times_start.size()) {
            SYSTEMTIME now;
            GetLocalTime(&now);

            std::wstringstream ss_period(L"");
            double work_s = diffSystemtimes(times_start.at(times_start.size()-1), now);
            work_total_s += work_s;
            int work_h = (int)(work_s / 3600.0);
            int work_m = (int)(std::fmod(work_s, 3600) / 60.0);
            ss_period << "(" << std::setfill(L'0') << work_h << ":" << std::setw(2) << std::setfill(L'0') << work_m << "h)";
            TextOut(hdc, 120, accumulated_y-15, ss_period.str().c_str(), _tcslen(ss_period.str().c_str()));
        }

        TextOut(hdc, 300, 5, _T("Total Worktime: "), _tcslen(_T("Total Worktime: ")));
        std::wstringstream ss_total(L"");
        int work_h = (int)(work_total_s / 3600.0);
        int work_m = (int)(std::fmod(work_total_s, 3600) / 60.0);
        ss_total << std::setfill(L'0') << work_h << ":" << std::setw(2) << std::setfill(L'0') << work_m;
        TextOut(hdc, 410, 5, ss_total.str().c_str(), _tcslen(ss_total.str().c_str()));

        return true;
    }

    bool writeToFile() {
        std::string filename = todayToFilename();
        std::ofstream logfile;
        logfile.open(filename, std::ios_base::out); //app for append

        SYSTEMTIME now;
        GetLocalTime(&now);
        //Insert ending time if none given
        if (times_end.size() < times_start.size()) {
            times_end.emplace_back(now);
        }

        logfile << "Start - End, (Duration)" << std::endl;
        double work_total_s = 0.0;

        for (int i = 0; i < times_start.size(); i++) {
            SYSTEMTIME time_start = times_start.at(i);
            SYSTEMTIME time_end;

            //Starting Times
            logfile << std::setw(2) << std::setfill('0') << time_start.wHour << ":" << std::setw(2) << std::setfill('0') << time_start.wMinute << " - ";

            //Ending Times and Duration
            if (i < times_end.size()) {
                time_end = times_end.at(i);
                logfile << std::setw(2) << std::setfill('0') << time_end.wHour << ":" << std::setw(2) << std::setfill('0') << time_end.wMinute;

                double work_s = diffSystemtimes(times_start.at(i), time_end);
                work_total_s += work_s;
                int work_h = (int)(work_s / 3600.0);
                int work_m = (int)(std::fmod(work_s, 3600) / 60.0);
                logfile << " (" << std::setfill('0') << work_h << ":" << std::setw(2) << std::setfill('0') << work_m << " h)" << std::endl;
            }
        }

        //Total work time
        int work_h = (int)(work_total_s / 3600.0);
        int work_m = (int)(std::fmod(work_total_s, 3600) / 60.0);
        logfile << "-> Total working time: " << std::setfill('0') << work_h << ":" << std::setw(2) << std::setfill('0') << work_m << std::endl;

        logfile.close();
        return true;
    }
};

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

    //InitCommonControls();

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL, _T("Call to RegisterClassEx failed!"), _T("Windows Desktop Guided Tour"), NULL);
        return 1;
    }

    // Store instance handle in our global variable
    hInst = hInstance;

    //HWND hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(1), NULL, (DLGPROC)DlgProc);
    // The parameters to CreateWindow explained:
    HWND hWnd = CreateWindow(
        szWindowClass,                  // szWindowClass: the name of the application
        szTitle,                        // szTitle: the text that appears in the title bar
        WS_OVERLAPPEDWINDOW,            // WS_OVERLAPPEDWINDOW: the type of window to create
        CW_USEDEFAULT, CW_USEDEFAULT,   // CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
        500, 300,                       // 500, 300: initial size (width, length)
        NULL,                           // HWND: the parent of this window
        NULL,                           // HMENU: this application does not have a menu bar
        hInstance,                      // hInstance: the first parameter from WinMain
        NULL                            // LPVOID: not used in this application
    );

    if (!hWnd)
    {
        MessageBox(NULL, _T("Call to CreateWindow failed!"), _T("Windows Desktop Guided Tour"), NULL);
        return 1;
    }

    CreateTrayIcon(hWnd);

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

    cTimerHelper ctimerhelper;

    if (oninit) {
        ctimerhelper.checkTodaysWorkingTimes();
        SetTimer(hWnd, TIMER_10S, TIMER_10S, NULL);
        oninit = false;
    }

    

    double elapsed = ctimerhelper.getSecondsSinceUserInput();

    if (working) {
        if (elapsed >= INACTIVITY_PERIOD) {
            SYSTEMTIME now;
            GetLocalTime(&now);
            SYSTEMTIME nowMinusElapsed = ctimerhelper.subSecondsFromSystemtime(now, elapsed);
            times_end.emplace_back(nowMinusElapsed);
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

    int wmId, wmEvent;

    switch (message)
    {
    //case WM_INITDIALOG:
    //    SetTimer(hWnd, TIMER_10S, TIMER_10S, NULL);
    //    break;
    case SWM_TRAYMSG: //Tray Icon Clicks
        switch (lParam)
        {
        case WM_LBUTTONDBLCLK:
            ShowWindow(hWnd, SW_RESTORE);
            break;
        case WM_RBUTTONDOWN:
        case WM_CONTEXTMENU:
            ShowContextMenu(hWnd);
        }
        break;
    case WM_COMMAND: //Commands e.g. from Tray Icon selection
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);

        switch (wmId)
        {
        case SWM_SHOW:
            ShowWindow(hWnd, SW_RESTORE);
            break;
        case SWM_HIDE:
            ShowWindow(hWnd, SW_HIDE);
            break;
        case SWM_FOLDER:
            ShellExecute(NULL, NULL, NULL, NULL, L".", SW_SHOWDEFAULT); //Opens current working directory
            break;
        case SWM_EXIT:
            DestroyWindow(hWnd);
            break;
        }
        return 1;
    case WM_TIMER:
        if (wParam == TIMER_10S)
        {
            InvalidateRect(hWnd, NULL, TRUE);   // invalidate whole window
        }
        break;
    case WM_PAINT:
    {
        hdc = BeginPaint(hWnd, &ps);

        // Application printouts
        ctimerhelper.drawWorkingTimes(hdc);

        // End application-specific layout section.

        EndPaint(hWnd, &ps);
    }
        break;
    case WM_CLOSE: //X Button shall not destroy window but minimize to tray
        ShowWindow(hWnd, SW_HIDE);
        break;
    case WM_ENDSESSION:
    case WM_DESTROY:
    {
        niData.uFlags = 0;
        Shell_NotifyIcon(NIM_DELETE, &niData);
        KillTimer(hWnd, TIMER_10S);
        ctimerhelper.writeToFile();
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
}

// Displays the right click context menu for the tray icon
void ShowContextMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    if (hMenu)
    {
        if (IsWindowVisible(hWnd))
            InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_HIDE, _T("Hide"));
        else
            InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_SHOW, _T("Show"));
        InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_FOLDER, _T("Show Logfiles"));
        InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, _T("Exit"));

        SetForegroundWindow(hWnd); //necessary for PopoupMenus on Tray Icons to disappear when wanted, see: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-trackpopupmenuex

        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
        DestroyMenu(hMenu);
    }
}

void CreateTrayIcon(HWND hWnd)
{
    ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

    niData.cbSize = NOTIFYICONDATA_V2_SIZE;
    niData.uID = 1;
    niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;

    // load Tray icon
    niData.hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(101));

    // load Window Bar Icon
    HICON hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(101), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
    SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    // the window to send messages to and the message to send
    niData.hWnd = hWnd;
    niData.uCallbackMessage = SWM_TRAYMSG;

    // tooltip for tray icon
    lstrcpyn(niData.szTip, _T("worktime-monitor"), sizeof(niData.szTip) / sizeof(TCHAR));

    Shell_NotifyIcon(NIM_ADD, &niData);

    // free icon handle
    if (niData.hIcon && DestroyIcon(niData.hIcon))
        niData.hIcon = NULL;
}