// A05_1b_https_beacon_polling.cpp
// Controlled HTTPS beacon with benign command polling.
// Supports commands: noop, ping, sleep|seconds

#include "C2Client.h"
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

C2Client *g_c2Client = nullptr;

DWORD WINAPI c2BeaconThread(LPVOID lpParam)
{
    C2Client *c2 = (C2Client *)lpParam;
    c2->setActive(true);

    while (c2->isRunning())
    {
        try
        {
            vector<wstring> commands = c2->checkIn();

            if (commands.empty())
            {
                c2->sendResult(L"checkin", L"success", L"CHECKIN_OK|No task returned");
            }

            for (const wstring &cmdLine : commands)
            {
                size_t pos = cmdLine.find(L'|');
                wstring cmd = (pos == wstring::npos) ? cmdLine : cmdLine.substr(0, pos);
                wstring args = (pos == wstring::npos) ? L"" : cmdLine.substr(pos + 1);
                wstring result;

                if (cmd == L"noop")
                {
                    result = L"SUCCESS|NOOP";
                }
                else if (cmd == L"ping")
                {
                    result = L"SUCCESS|PONG";
                }
                else if (cmd == L"sleep")
                {
                    DWORD seconds = (DWORD)_wtoi(args.c_str());

                    if (seconds < 5)
                        seconds = 5;

                    if (seconds > 300)
                        seconds = 300;

                    c2->setBeaconIntervalSeconds(seconds);
                    result = L"SUCCESS|Beacon interval set to " + to_wstring(seconds) + L" seconds";
                }
                else
                {
                    result = L"IGNORED|Unsupported benign lab command: " + cmd;
                }

                c2->sendResult(cmd, L"success", result);
            }

            Sleep(c2->getBeaconIntervalMs());
        }
        catch (...)
        {
            Sleep(30000);
        }
    }

    return 0;
}

int main()
{
    // Prefer domain for DNS + HTTPS artifacts.
    wstring c2ServerUrl = L"https://c2.lab.local";

    try
    {
        g_c2Client = new C2Client(c2ServerUrl);

        // Startup connectivity test.
        g_c2Client->testConnection();

        HANDLE hBeaconThread = CreateThread(NULL, 0, c2BeaconThread, g_c2Client, 0, NULL);

        if (hBeaconThread)
        {
            WaitForSingleObject(hBeaconThread, INFINITE);
            CloseHandle(hBeaconThread);
        }
        else
        {
            delete g_c2Client;
            g_c2Client = nullptr;
            return 1;
        }

        delete g_c2Client;
        g_c2Client = nullptr;
    }
    catch (...)
    {
        return 1;
    }

    return 0;
}