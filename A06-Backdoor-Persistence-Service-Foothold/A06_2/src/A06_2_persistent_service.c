#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <tlhelp32.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <shlwapi.h>
#include <winevt.h>
#include <stdarg.h>

#include "PersistentService.h"

#pragma comment(lib, "wevtapi.lib")
#pragma comment(lib, "wtsapi32.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")

#define BUFFER_SIZE 1024
#define DATETIME_BUFFER_SIZE 80
#define SERVICE_REG_KEY L"SOFTWARE\\A06_2_Persistent_Service"
#define SERVICE_KEY_NAME L"Path"
#define EVENT_SUBSCRIBE_PATH L"Security"
#define EVENT_SUBSCRIBE_QUERY L"Event/System[EventID=4624]"
#define LOG_FILE_NAME L"log.txt"

SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE hServiceStatus;
HWND hWnd = NULL;
HANDLE hPrevAppProcess = NULL;
HANDLE ghSvcStopEvent = NULL;

wchar_t m_szExeToFind[MAX_PATH] = L""; // Process executable name
wchar_t m_szExeToRun[MAX_PATH] = L"";  // Fully qualified binary path
BOOL g_bLoggedIn = FALSE;

// --- Event Log Subscription Logic (Procedural C Implementation) ---

DWORD WINAPI SubscriptionCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID pContext, EVT_HANDLE hEvent)
{
    if (action == EvtSubscribeActionDeliver)
    {
        WriteToLog(L"SubscriptionCallback invoked.");
        HANDLE hEventToSet = (HANDLE)pContext;
        if (hEventToSet != NULL)
        {
            SetEvent(hEventToSet);
        }
    }
    return ERROR_SUCCESS;
}

BOOL InitUserLoginListener(HANDLE *phWait, HANDLE *phSubscription)
{
    *phWait = CreateEvent(NULL, FALSE, FALSE, NULL);
    *phSubscription = EvtSubscribe(NULL, NULL,
                                   EVENT_SUBSCRIBE_PATH, EVENT_SUBSCRIBE_QUERY,
                                   NULL,
                                   *phWait, // Context passed directly to callback
                                   (EVT_SUBSCRIBE_CALLBACK)SubscriptionCallback,
                                   EvtSubscribeToFutureEvents);

    if (*phSubscription == NULL)
    {
        DWORD status = GetLastError();
        if (ERROR_EVT_CHANNEL_NOT_FOUND == status)
            WriteToLog(L"Channel %s was not found.\n", EVENT_SUBSCRIBE_PATH);
        else if (ERROR_EVT_INVALID_QUERY == status)
            WriteToLog(L"The query \"%s\" is not valid.\n", EVENT_SUBSCRIBE_QUERY);
        else
            WriteToLog(L"EvtSubscribe failed with %lu.\n", status);

        CloseHandle(*phWait);
        *phWait = NULL;
        return FALSE;
    }
    return TRUE;
}

void WaitForUserToLogIn(HANDLE hSubscription, HANDLE hWait)
{
    WriteToLog(L"Waiting for a user to log in...");
    EVT_HANDLE hEvents[1];
    DWORD dwReturned;

    // Check for an active session logon event already triggered
    if (EvtNext(hSubscription, 1, hEvents, INFINITE, 0, &dwReturned))
    {
        WriteToLog(L"Received a Logon event - a user has already logged in");
        EvtClose(hEvents[0]);
        return;
    }

    WaitForSingleObject(hWait, INFINITE);
    WriteToLog(L"Received a Logon event - a user has logged in");
}

void CleanupUserLoginListener(HANDLE hWait, HANDLE hSubscription)
{
    if (hWait != NULL)
        CloseHandle(hWait);
    if (hSubscription != NULL)
        EvtClose(hSubscription);
}

// --- Utilities & Diagnostic Interfacing ---

void enableConsole(void)
{
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    HWND hConsoleWnd = GetConsoleWindow();
    SetWindowLong(hConsoleWnd, GWL_EXSTYLE, GetWindowLong(hConsoleWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hConsoleWnd, 0, 170, LWA_ALPHA);
    (void)freopen("CON", "w", stdout);
}

void GetExePath(wchar_t *outBuffer, DWORD maxLen)
{
    GetModuleFileName(NULL, outBuffer, maxLen);
    int pos = -1;
    int index = 0;
    while (outBuffer[index])
    {
        if (outBuffer[index] == L'\\' || outBuffer[index] == L'/')
        {
            pos = index;
        }
        index++;
    }
    if (pos != -1)
    {
        outBuffer[pos + 1] = 0;
    }
}

void WriteToLog(LPCTSTR lpText, ...)
{
    FILE *fp;
    wchar_t log_file[MAX_PATH] = L"";
    wchar_t exePath[MAX_PATH];

    GetExePath(exePath, MAX_PATH);
    wcscpy_s(log_file, MAX_PATH, exePath);
    wcscat_s(log_file, MAX_PATH, LOG_FILE_NAME);

    time_t rawtime;
    struct tm *ptm;
    wchar_t buf_time[DATETIME_BUFFER_SIZE];
    time(&rawtime);
    ptm = gmtime(&rawtime);
    wcsftime(buf_time, sizeof(buf_time) / sizeof(*buf_time), L"%d.%m.%Y %H:%M", ptm);

    wchar_t buffer_in[BUFFER_SIZE];
    va_list ptr;
    va_start(ptr, lpText);
    vswprintf(buffer_in, BUFFER_SIZE, lpText, ptr);
    va_end(ptr);

    wchar_t buffer_out[BUFFER_SIZE];
    swprintf(buffer_out, BUFFER_SIZE, L"%s %s\n", buf_time, buffer_in);

    _wfopen_s(&fp, log_file, L"a,ccs=UTF-8");
    if (fp)
    {
        fwprintf(fp, L"%s\n", buffer_out);
        fclose(fp);
    }

    wcscat_s(buffer_out, BUFFER_SIZE, L"\n");
    HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdOut != NULL && stdOut != INVALID_HANDLE_VALUE)
    {
        DWORD written = 0;
        WriteConsole(stdOut, buffer_out, (DWORD)wcslen(buffer_out), &written, NULL);
    }
}

// --- Registry Interaction Functions ---

BOOL CreateRegistryKey(HKEY hKeyParent, PWCHAR subkey)
{
    DWORD dwDisposition;
    HKEY hKey;
    DWORD Ret = RegCreateKeyEx(hKeyParent, subkey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition);
    if (Ret != ERROR_SUCCESS)
    {
        WriteToLog(L"Error opening or creating new key\n");
        return FALSE;
    }
    RegCloseKey(hKey);
    return TRUE;
}

BOOL writeStringInRegistry(HKEY hKeyParent, PWCHAR subkey, PWCHAR valueName, PWCHAR strData)
{
    HKEY hKey;
    DWORD Ret = RegOpenKeyEx(hKeyParent, subkey, 0, KEY_WRITE, &hKey);
    if (Ret == ERROR_SUCCESS)
    {
        if (ERROR_SUCCESS != RegSetValueEx(hKey, valueName, 0, REG_SZ, (LPBYTE)(strData), (((DWORD)lstrlen(strData) + 1) * 2)))
        {
            RegCloseKey(hKey);
            return FALSE;
        }
        RegCloseKey(hKey);
        return TRUE;
    }
    return FALSE;
}

BOOL readStringFromRegistry(HKEY hKeyParent, PWCHAR subkey, PWCHAR valueName, wchar_t *outBuffer, DWORD maxLen)
{
    HKEY hKey;
    DWORD len = 1024;
    DWORD readDataLen = len;
    PWCHAR readBuffer = (PWCHAR)malloc(sizeof(wchar_t) * len);
    if (readBuffer == NULL)
        return FALSE;

    DWORD Ret = RegOpenKeyEx(hKeyParent, subkey, 0, KEY_READ, &hKey);
    if (Ret == ERROR_SUCCESS)
    {
        Ret = RegQueryValueEx(hKey, valueName, NULL, NULL, (BYTE *)readBuffer, &readDataLen);
        while (Ret == ERROR_MORE_DATA)
        {
            len += 1024;
            PWCHAR temp = (PWCHAR)realloc(readBuffer, sizeof(wchar_t) * len);
            if (!temp)
            {
                free(readBuffer);
                RegCloseKey(hKey);
                return FALSE;
            }
            readBuffer = temp;
            readDataLen = len;
            Ret = RegQueryValueEx(hKey, valueName, NULL, NULL, (BYTE *)readBuffer, &readDataLen);
        }
        if (Ret != ERROR_SUCCESS)
        {
            free(readBuffer);
            RegCloseKey(hKey);
            return FALSE;
        }
        wcsncpy_s(outBuffer, maxLen, readBuffer, _TRUNCATE);
        free(readBuffer);
        RegCloseKey(hKey);
        return TRUE;
    }
    free(readBuffer);
    return FALSE;
}

// --- Entry Point & Core Execution Switching ---

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpszCmdLine, int nCmdShow)
{
    enableConsole();
    WriteToLog(L"A06_2_Persistent_Service: executable invoked.");

    if (wcsstr(lpszCmdLine, SERVICE_COMMAND_Launcher) != NULL)
    {
        WriteToLog(L"Launcher instance started. Entering AppMainFunction.");
        return AppMainFunction();
    }

    // 1. Hardcode your dynamic analysis target application path here
    // This eliminates the need for the "Install#Path" command line argument
    wcsncpy_s(m_szExeToRun, MAX_PATH, L"C:\\Users\\Public\\A06_2_SampleApp.exe", _TRUNCATE);

    // 2. Try to connect to the Service Control Manager to check if we are running as a service
    // If StartServiceCtrlDispatcher succeeds, Windows SCM is executing this binary.
    SERVICE_TABLE_ENTRY serviceTableEntry[] =
        {
            {(LPWSTR)SERVICE_NAME, ServiceMain},
            {NULL, NULL}};

    WriteToLog(L"Attempting to connect to Service Control Dispatcher...");
    if (!StartServiceCtrlDispatcher(serviceTableEntry))
    {
        DWORD dwError = GetLastError();

        // ERROR_FAILED_SERVICE_CONTROLLER_CONNECT means it was run by a user/double-clicked,
        // NOT by the Service Control Manager. This is our cue to self-install!
        if (dwError == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
        {
            WriteToLog(L"Not running as a service. Initiating automated self-installation workflow.");

            // Verify the hardcoded target exists before setting registry state
            if (PathFileExists(m_szExeToRun))
            {
                WriteToLog(L"[WatchDog] Target payload verified: %s", m_szExeToRun);
                if (!CreateRegistryKey(HKEY_LOCAL_MACHINE, (PWCHAR)SERVICE_REG_KEY))
                {
                    WriteToLog(L"Failed to create persistence registry subkey.");
                }
                // Pre-populate the registry configuration key so the service knows what to monitor
                if (!writeStringInRegistry(HKEY_LOCAL_MACHINE, (PWCHAR)SERVICE_REG_KEY, (PWCHAR)SERVICE_KEY_NAME, m_szExeToRun))
                {
                    WriteToLog(L"Failed to write configuration target to registry.");
                }
            }
            else
            {
                WriteToLog(L"Warning: Target app '%s' not found. Self-installing anyway.", m_szExeToRun);
            }

            // Trigger the installation and service start
            InstallService();
            WriteToLog(L"Self-installation complete. Exiting installer process loop.");
        }
        else
        {
            WriteToLog(L"StartServiceCtrlDispatcher failed with unexpected error: %d", dwError);
        }
    }

    return 0;
}

// --- Service Lifecycle Subsystems ---

void WINAPI InstallService(void)
{
    TCHAR szServicePath[MAX_PATH] = _T("\"");
    TCHAR szPath[MAX_PATH] = {0};
    GetModuleFileName(NULL, szPath, MAX_PATH);
    lstrcat(szServicePath, szPath);
    lstrcat(szServicePath, _T("\""));

    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager)
        return;

    SC_HANDLE hService = CreateService(hSCManager, SERVICE_NAME, SERVICE_NAME,
                                       SERVICE_ALL_ACCESS | SERVICE_USER_DEFINED_CONTROL | READ_CONTROL | WRITE_DAC | WRITE_OWNER,
                                       SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
                                       szServicePath, NULL, NULL, NULL, NULL, _T(""));

    if (hService == NULL)
    {
        hService = OpenService(hSCManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
        if (hService == NULL)
        {
            CloseServiceHandle(hSCManager);
            return;
        }
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return;
    }

    SERVICE_DESCRIPTION description = {(LPTSTR) _T("Secured Globe Windows Service")};
    ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &description);

    if (!StartService(hService, 0, NULL))
    {
        WriteToLog(L"Error starting service %d\n", GetLastError());
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
}

void WINAPI UninstallService(void)
{
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager)
        return;

    SC_HANDLE hService = OpenService(hSCManager, SERVICE_NAME, DELETE | SERVICE_STOP);
    if (hService)
    {
        SERVICE_STATUS status;
        ControlService(hService, SERVICE_CONTROL_STOP, &status);
        if (DeleteService(hService))
        {
            WriteToLog(L"Service deleted successfully.");
        }
        else
        {
            WriteToLog(L"DeleteService failed. Error %d", GetLastError());
        }
        CloseServiceHandle(hService);
    }
    CloseServiceHandle(hSCManager);

    // Clean up persistent state registry keys
    SHDeleteKeyW(HKEY_LOCAL_MACHINE, SERVICE_REG_KEY);
}

void ReportServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    serviceStatus.dwCurrentState = dwCurrentState;
    serviceStatus.dwWin32ExitCode = dwWin32ExitCode;
    serviceStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
    {
        serviceStatus.dwControlsAccepted = 0;
    }
    else
    {
        serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SESSIONCHANGE;
    }

    if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
    {
        serviceStatus.dwCheckPoint = 0;
    }
    else
    {
        serviceStatus.dwCheckPoint = dwCheckPoint++;
    }

    SetServiceStatus(hServiceStatus, &serviceStatus);
}

// --- Session Deflowering & Token Execution ---

void ImpersonateActiveUserAndRun(void)
{
    DWORD session_id = -1;
    DWORD session_count = 0;
    WTS_SESSION_INFOW *pSession = NULL;

    if (!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSession, &session_count))
    {
        WriteToLog(L"WTSEnumerateSessions - failed. Error %d", GetLastError());
        return;
    }

    TCHAR szCurModule[MAX_PATH] = {0};
    GetModuleFileName(NULL, szCurModule, MAX_PATH);

    for (size_t i = 0; i < session_count; i++)
    {
        session_id = pSession[i].SessionId;
        WTS_CONNECTSTATE_CLASS wts_connect_state = WTSDisconnected;
        WTS_CONNECTSTATE_CLASS *ptr_wts_connect_state = NULL;
        DWORD bytes_returned = 0;

        if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, session_id, WTSConnectState,
                                       (LPTSTR *)&ptr_wts_connect_state, &bytes_returned))
        {
            wts_connect_state = *ptr_wts_connect_state;
            WTSFreeMemory(ptr_wts_connect_state);
            if (wts_connect_state != WTSActive)
                continue;
        }
        else
        {
            continue;
        }

        HANDLE hImpersonationToken;
        if (!WTSQueryUserToken(session_id, &hImpersonationToken))
            continue;

        DWORD neededSize1 = 0;
        HANDLE realToken = NULL;
        if (GetTokenInformation(hImpersonationToken, TokenLinkedToken, &realToken, sizeof(HANDLE), &neededSize1))
        {
            CloseHandle(hImpersonationToken);
            hImpersonationToken = realToken;
        }
        else
        {
            // Fail-safe handling if token is unlinked
            realToken = hImpersonationToken;
        }

        HANDLE hUserToken;
        if (!DuplicateTokenEx(realToken, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS | MAXIMUM_ALLOWED,
                              NULL, SecurityImpersonation, TokenPrimary, &hUserToken))
        {
            CloseHandle(realToken);
            continue;
        }

        ImpersonateLoggedOnUser(hUserToken);

        STARTUPINFOW StartupInfo = {0};
        StartupInfo.cb = sizeof(STARTUPINFOW);
        PROCESS_INFORMATION processInfo = {0};
        void *lpEnvironment = NULL;

        if (!CreateEnvironmentBlock(&lpEnvironment, hUserToken, FALSE))
        {
            WriteToLog(L"CreateEnvironmentBlock - failed. Error %d", GetLastError());
            CloseHandle(hUserToken);
            CloseHandle(realToken);
            RevertToSelf();
            continue;
        }

        wchar_t commandLine[1024];
        swprintf(commandLine, 1024, L"\"%s\" \"%s\"", szCurModule, SERVICE_COMMAND_Launcher);

        BOOL result = CreateProcessAsUserW(hUserToken, NULL, commandLine, NULL, NULL, FALSE,
                                           NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT, lpEnvironment, NULL, &StartupInfo, &processInfo);

        if (!result)
            WriteToLog(L"CreateProcessAsUser - failed. Error %d", GetLastError());
        else
            WriteToLog(L"CreateProcessAsUser - success");

        DestroyEnvironmentBlock(lpEnvironment);
        CloseHandle(hUserToken);
        CloseHandle(realToken);
        RevertToSelf();
    }
    WTSFreeMemory(pSession);
}

void GetLoggedInUser(wchar_t *outUser, DWORD maxLen)
{
    outUser[0] = L'\0';
    WTS_SESSION_INFO *SessionInfo;
    unsigned long SessionCount;
    unsigned long ActiveSessionId = -1;

    if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &SessionInfo, &SessionCount))
    {
        for (size_t i = 0; i < SessionCount; i++)
        {
            if (SessionInfo[i].State == WTSActive || SessionInfo[i].State == WTSConnected)
            {
                ActiveSessionId = SessionInfo[i].SessionId;
                break;
            }
        }

        wchar_t *UserName = NULL;
        if (ActiveSessionId != -1)
        {
            unsigned long BytesReturned;
            if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, ActiveSessionId, WTSUserName, &UserName, &BytesReturned))
            {
                wcsncpy_s(outUser, maxLen, UserName, _TRUNCATE);
                WTSFreeMemory(UserName);
            }
        }
        WTSFreeMemory(SessionInfo);
    }
}

void WINAPI ServiceMain(DWORD dwArgCount, LPTSTR lpszArgValues[])
{
    ZeroMemory(&serviceStatus, sizeof(SERVICE_STATUS));
    serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

    hServiceStatus = RegisterServiceCtrlHandlerEx(SERVICE_NAME, CtrlHandlerEx, NULL);
    if (hServiceStatus == NULL)
        return;

    ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 1000);

    ghSvcStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ghSvcStopEvent == NULL)
    {
        ReportServiceStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    ReportServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);

    ImpersonateActiveUserAndRun();

    WaitForSingleObject(ghSvcStopEvent, INFINITE);

    CloseHandle(ghSvcStopEvent);
    ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

DWORD AppMainFunction(void)
{
    WriteToLog(L"AppMainFunction start\n");
    HINSTANCE hInstance = GetModuleHandle(NULL);

    TCHAR szDirPath[MAX_PATH] = {0};
    GetModuleFileName(NULL, szDirPath, MAX_PATH);
    PathRemoveFileSpec(szDirPath);
    SetCurrentDirectory(szDirPath);

    WNDCLASSEX wcex = {sizeof(wcex)};
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = S_WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = MAIN_CLASS_NAME;
    RegisterClassEx(&wcex);

    hWnd = CreateWindow(MAIN_CLASS_NAME, _T(""), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (hWnd)
    {
        g_bLoggedIn = TRUE;
        SetTimer(hWnd, MAIN_TIMER_ID, 10000, NULL);
        WriteToLog(L"Persistent Service [%S %S] has started\n", __DATE__, __TIME__);
    }
    else
    {
        WriteToLog(L"CreateWindow failed\n");
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    hWnd = NULL;
    return (DWORD)msg.wParam;
}

LRESULT CALLBACK S_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static BOOL is_running = FALSE;

    switch (message)
    {
    case WM_ENDSESSION:
    case WM_QUERYENDSESSION:
        WriteToLog(L"Logging off\n");
        KillTimer(hWnd, MAIN_TIMER_ID);
        g_bLoggedIn = FALSE;
        return 0;

    case WM_TIMER:
        if (is_running)
            break;
        WriteToLog(L"Timer event");
        is_running = TRUE;

        wchar_t szPath[MAX_PATH] = L"";

        if (readStringFromRegistry(HKEY_LOCAL_MACHINE, (PWCHAR)SERVICE_REG_KEY, (PWCHAR)SERVICE_KEY_NAME, szPath, MAX_PATH))
        {
            wchar_t *lastSlash = wcsrchr(szPath, L'\\');
            wchar_t *lastForward = wcsrchr(szPath, L'/');
            wchar_t *baseName = (lastSlash > lastForward) ? lastSlash : lastForward;
            if (baseName)
                wcsncpy_s(m_szExeToFind, MAX_PATH, baseName + 1, _TRUNCATE);
            else
                wcsncpy_s(m_szExeToFind, MAX_PATH, szPath, _TRUNCATE);

            wcsncpy_s(m_szExeToRun, MAX_PATH, szPath, _TRUNCATE);
        }
        else
        {
            WriteToLog(L"Error reading ExeToFind from the Registry");
            is_running = FALSE;
            return 1;
        }

        WriteToLog(L"Checking if '%s' is running...", m_szExeToFind);
        BOOL programRunning = FALSE;

        HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE)
        {
            WriteToLog(L"Failed to call CreateToolhelp32Snapshot(). Error code %d", GetLastError());
            is_running = FALSE;
            return 1;
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hProcessSnap, &pe32))
        {
            do
            {
                if (_wcsicmp(m_szExeToFind, pe32.szExeFile) == 0)
                {
                    WriteToLog(L"%s is running", m_szExeToFind);
                    programRunning = TRUE;
                    break;
                }
            } while (Process32Next(hProcessSnap, &pe32));
        }
        CloseHandle(hProcessSnap);

        if (!programRunning)
        {
            WriteToLog(L"'%s' is not running. Need to start it", m_szExeToFind);

            if (wcslen(m_szExeToRun) > 0)
            {
                if (!g_bLoggedIn)
                {
                    WriteToLog(L"WatchDog isn't starting '%s' because user isn't logged on", m_szExeToFind);
                    is_running = FALSE;
                    return 1;
                }

                ImpersonateActiveUserAndRun();
            }
        }

        is_running = FALSE;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

DWORD WINAPI CtrlHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID pEventData, LPVOID pUserData)
{
    switch (dwControl)
    {
    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
        if (dwControl == SERVICE_CONTROL_SHUTDOWN)
            WriteToLog(L"SERVICE_CONTROL_SHUTDOWN");
        else
            WriteToLog(L"SERVICE_CONTROL_STOP");

        ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        SetEvent(ghSvcStopEvent);
        ReportServiceStatus(serviceStatus.dwCurrentState, NO_ERROR, 0);
        break;
    case SERVICE_CONTROL_SESSIONCHANGE:
        if (dwEventType == WTS_SESSION_LOGON)
        {
            WriteToLog(L"WTS_SESSION_LOGON");
            ImpersonateActiveUserAndRun();
        }
        else if (dwEventType == WTS_SESSION_LOGOFF)
        {
            WriteToLog(L"WTS_SESSION_LOGOFF");
            hPrevAppProcess = NULL;
        }
        break;
    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
    return NO_ERROR;
}