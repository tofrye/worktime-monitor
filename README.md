# worktime-monitor

[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![build](https://github.com/tofrye/worktime-monitor/actions/workflows/msbuild.yml/badge.svg)](https://github.com/tofrye/worktime-monitor/actions/workflows/msbuild.yml)

This tool shall help users to track their working time automatically and without manual writing down of start and end times.

It uses the Windows API to retrieve information about the user activity, which is usable from Windows 2000 and above (XP, Vista, 7, 8, 10):
https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-lastinputinfo

The current state of the program looks like this:

![Screenshot](doc/program.png)

## Features

- Tracks your time using the LastInputInfo information
- If no input is detected for 10 minutes the user is expected to be not working
- The inactivity period (e.g. 10 minutes) will not be stored as working time but is already non-working time
- On closing or shutdown of computer the program will store a logifle in the current directory of the executable
- A new logfile will be stored for every day with the naming: worktime-log_yyyy-mm-dd.txt
- If your device is restarted the program will load previous working times from the file of today
- If you press X on the program it will not be shut down but minimized to tray
- On the tray icon you can select to "Show", "Show logfiles" and "Close"

## Known limitations and Bugs

- Currently the ending time is not stored correctly if system is set to standby or hibernation

# Usage
You can download the prebuild binary for your Platform [here](https://github.com/tofrye/worktime-monitor/releases) or build the application yourself from source.

For convenient usage I recommend to set a link in autostart folder so the application will be started automatically upon every start.

To add an application to autostart place the link here for your user:
```bash
C:\Users\{USER}\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup
```

## How to build
Download MS Visual Studio 2019 and load the solution file (worktime-monitor.sln).

Afterwards you can run "Build solution" and start the program.

## License
worktime-monitor is licensed under the MIT license, see license file for details.
