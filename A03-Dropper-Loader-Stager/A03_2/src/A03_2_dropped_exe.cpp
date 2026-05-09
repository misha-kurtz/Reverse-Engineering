// A03_2_dropped_exe.cpp
// Custom executable to be dropped to disk and executed after successful network download.

#include <windows.h>
#include <string>
#include <sstream>
#include <stdio.h>

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
    oss << "THESIS_A03_2_EXE_DROPPED_AND_EXECUTED_VIA_NETWORK_STAGER\r\n";
    oss << "Timestamp: " << timestamp << "\r\n";
    oss << "PID: " << pid << "\r\n";
    oss << "ProcessPath: " << processPath << "\r\n";

    std::string contents = oss.str();

    const char *outPath = "C:\\Users\\Public\\A03_2_Network_Stager_OK.txt";

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

    OutputDebugStringA("THESIS_A03_2_EXE_DROPPED_AND_EXECUTED_VIA_NETWORK_STAGER\r\n");
}

int main()
{
    WriteMarkerFile();
    return 0;
}