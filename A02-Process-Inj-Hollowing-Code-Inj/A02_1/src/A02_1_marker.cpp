// a02_1_marker.cpp
// Custom DLL for DLL injection
#include <windows.h>
#include <string>
#include <sstream>

#pragma comment(lib, "Advapi32.lib")

static std::string GetProcessPath()
{
    char path[MAX_PATH] = {0};
    DWORD len = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
        return "unknown_process";
    return std::string(path);
}

static std::string GetFileTimeString()
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    char buffer[128] = {0};
    sprintf_s(
        buffer,
        "%04d-%02d-%02d %02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);

    return std::string(buffer);
}

static void WriteMarkerFile()
{
    DWORD pid = GetCurrentProcessId();
    std::string processPath = GetProcessPath();
    std::string timestamp = GetFileTimeString();

    std::ostringstream oss;
    oss << "THESIS_A02_1_DLL_LOADED\r\n";
    oss << "Timestamp: " << timestamp << "\r\n";
    oss << "PID: " << pid << "\r\n";
    oss << "ProcessPath: " << processPath << "\r\n";

    std::string contents = oss.str();

    const char *outPath = "C:\\Users\\Public\\A02_1_Injected_OK.txt";

    HANDLE hFile = CreateFileA(
        outPath,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten = 0;
        WriteFile(hFile, contents.c_str(), (DWORD)contents.size(), &bytesWritten, NULL);
        CloseHandle(hFile);
    }

    OutputDebugStringA("THESIS_A02_1_DLL_LOADED");
}

DWORD WINAPI MarkerThread(LPVOID)
{
    WriteMarkerFile();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);

        HANDLE hThread = CreateThread(
            NULL,
            0,
            MarkerThread,
            NULL,
            0,
            NULL);

        if (hThread != NULL)
        {
            CloseHandle(hThread);
        }
        break;
    }

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    return TRUE;
}