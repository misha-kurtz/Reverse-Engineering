#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <time.h>

#pragma comment(lib, "User32.lib")

// Helper function to append a timestamped log entry to a file
void WriteBeaconLog(const wchar_t *message)
{
    FILE *logFile = NULL;
    // Updated log path to explicitly isolate A06_3a data
    wchar_t logPath[MAX_PATH] = L"C:\\Users\\Public\\A06_3a_COM_Logon_Triggered_Scheduled_Task_SampleApp_log.txt";

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
    // Updated initial execution log string to accurately describe the execution context
    WriteBeaconLog(L"SampleApp initialized via A06_3a COM Logon-Triggered Scheduled Task.");

    // Background worker beacon loop
    while (TRUE)
    {
        // Sleep for 5000 milliseconds (5 seconds)
        Sleep(5000);

        // Append an alive notification to prove the task scheduler is keeping the process alive
        WriteBeaconLog(L"Beacon: A06_3a COM Logon-Triggered Scheduled Task process is alive and looping.");
    }

    return 0;
}