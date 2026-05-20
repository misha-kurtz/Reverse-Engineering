// A06_1a_persistence_marker.cpp
// Controlled marker executable launched via Run-key persistence

#include <windows.h>
#include <string>
#include <sstream>
#include <stdio.h>
#include <tlhelp32.h>

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
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond);

    return std::string(buffer);
}

static DWORD GetParentPID()
{
    DWORD ppid = 0;

    HANDLE snapshot =
        CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    DWORD currentPID = GetCurrentProcessId();

    if (Process32First(snapshot, &pe))
    {
        do
        {
            if (pe.th32ProcessID == currentPID)
            {
                ppid = pe.th32ParentProcessID;
                break;
            }
        } while (Process32Next(snapshot, &pe));
    }

    CloseHandle(snapshot);

    return ppid;
}

static void WriteMarkerFile()
{
    // Optional delay to simulate realistic persistence execution
    Sleep(10000);

    DWORD pid = GetCurrentProcessId();
    DWORD ppid = GetParentPID();

    std::string processPath = GetProcessPath();
    std::string timestamp = GetFileTimeString();

    std::ostringstream oss;

    oss << "THESIS_A06_1A_RUNKEY_PERSISTENCE_TRIGGERED\r\n";
    oss << "Timestamp: " << timestamp << "\r\n";
    oss << "PID: " << pid << "\r\n";
    oss << "ParentPID: " << ppid << "\r\n";
    oss << "ProcessPath: " << processPath << "\r\n";
    oss << "PersistenceType: RegistryRunKey\r\n";
    oss << "ExecutionTrigger: UserLogon\r\n";
    oss << "PersistenceConfirmed: TRUE\r\n";

    std::string contents = oss.str();

    const char *outPath =
        "C:\\Users\\Public\\A06_1a_RunKeyPersistence_OK.txt";

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

        WriteFile(
            hFile,
            contents.c_str(),
            (DWORD)contents.size(),
            &bytesWritten,
            NULL);

        CloseHandle(hFile);
    }

    OutputDebugStringA(
        "THESIS_A06_1A_RUNKEY_PERSISTENCE_TRIGGERED");
}

int main()
{
    WriteMarkerFile();
    return 0;
}