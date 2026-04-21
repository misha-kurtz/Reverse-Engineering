// A02_1_target_process.cpp
// Custom target process for DLL injection
#include <windows.h>

#pragma comment(lib, "User32.lib")

const char *CLASS_NAME = "A02_1_Target_Process_Class";
const char *WINDOW_TITLE = "A02_1_target_process";

// Basic window procedure
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
    // Register window class
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassA(&wc);

    // Create window
    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        WINDOW_TITLE, // <-- IMPORTANT (used by injector)
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

    // Message loop (keeps process alive)
    MSG msg = {};
    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}