#pragma once

#include <windows.h>
#include <tchar.h>

// Customizable values
#define SERVICE_NAME _T("A06_2_Persistent_Service")   // Service name register
#define SERVICE_COMMAND_Launcher L"ServiceIsLauncher" // Still required! SCM instance uses this internally when spawning the user session app

#define MAIN_CLASS_NAME L"ServiceClass" // Window class name for service client
#define MAIN_TIMER_ID 2001

void WriteToLog(LPCTSTR lpText, ...);
void ReportServiceStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
void InstallService(void);
void UninstallService(void);
void ImpersonateActiveUserAndRun(void);
void GetLoggedInUser(wchar_t *outUser, DWORD maxLen);
void WINAPI ServiceMain(DWORD dwArgCount, LPTSTR lpszArgValues[]);
DWORD WINAPI CtrlHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID pEventData, LPVOID pUserData);
DWORD AppMainFunction(void);
LRESULT CALLBACK S_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Procedural event subscription handlers replacing the C++ class
BOOL InitUserLoginListener(HANDLE *phWait, HANDLE *phSubscription);
void WaitForUserToLogIn(HANDLE hSubscription, HANDLE hWait);
void CleanupUserLoginListener(HANDLE hWait, HANDLE hSubscription);