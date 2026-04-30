// A02_4_apc_target_process.cpp
// Custom target process for APC-based DLL loading

#include <windows.h>

#pragma comment(lib, "User32.lib")

const char *CLASS_NAME = "A02_4_APC_Target_Process_Class";
const char *WINDOW_TITLE = "A02_4_apc_target_process";

DWORD WINAPI AlertableThread(LPVOID)
{
    OutputDebugStringA("A02_4 alertable APC thread started");

    while (true)
    {
        // TRUE = alertable state
        // Queued user-mode APCs can execute here.
        SleepEx(1000, TRUE);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    HANDLE hAlertableThread = CreateThread(
        NULL,
        0,
        AlertableThread,
        NULL,
        0,
        NULL);

    if (hAlertableThread != NULL)
    {
        CloseHandle(hAlertableThread);
    }

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 200,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (hwnd == NULL)
        return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}