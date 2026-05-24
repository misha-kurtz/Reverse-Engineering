#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <time.h>

#pragma comment(lib, "User32.lib")

// Helper function to append a timestamped log entry to a file
void WriteBeaconLog(const wchar_t *message)
{
    FILE *logFile = NULL;
    wchar_t logPath[MAX_PATH] = L"C:\\Users\\Public\\A06_2_Persistent_Service_SampleApp_log.txt";

    // Fetch the current local system time
    time_t rawTime;
    struct tm *timeInfo;
    wchar_t timeString[64];

    time(&rawTime);
    timeInfo = localtime(&rawTime);
    wcsftime(timeString, sizeof(timeString) / sizeof(wchar_t), L"[%Y-%m-%d %H:%M:%S]", timeInfo);

    // Open the log file in append mode with UTF-8 encoding configuration
    _wfopen_s(&logFile, logPath, L"a,ccs=UTF-8");
    if (logFile != NULL)
    {
        fwprintf(logFile, L"%s %s\n", timeString, message);
        fclose(logFile);
    }
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // Log the initial execution event
    WriteBeaconLog(L"SampleApp initialized successfully under the logged-on user session.");

    // Optional: Keep the MessageBox for immediate visual confirmation during dynamic analysis,
    // or comment it out if you want the app to run completely silently in the background.
    MessageBoxW(NULL, L"SampleApp is running under the logged-in session!", L"A06_2 Watchdog POC", MB_OK | MB_ICONINFORMATION);

    // Background worker beacon loop
    while (TRUE)
    {
        // Sleep for 5000 milliseconds (5 seconds)
        Sleep(5000);

        // Append a alive notification to prove the watchdog is keeping the process alive
        WriteBeaconLog(L"Beacon: SampleApp process is alive and looping.");
    }

    return 0;
}