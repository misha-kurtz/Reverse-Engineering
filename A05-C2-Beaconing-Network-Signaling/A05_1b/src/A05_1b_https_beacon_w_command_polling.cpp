// A05_1b_https_beacon_polling.cpp
// Controlled HTTPS beacon with command polling.
// Supports commands: noop, ping, sleep|seconds

#include "C2Client.h"
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

C2Client *g_c2Client = nullptr;

void Log(const wstring &msg)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    wcout << L"["
          << st.wHour << L":"
          << st.wMinute << L":"
          << st.wSecond
          << L"] "
          << msg
          << endl;
}

DWORD WINAPI c2BeaconThread(LPVOID lpParam)
{
    C2Client *c2 = (C2Client *)lpParam;

    c2->setActive(true);

    Log(L"Beacon thread started");

    while (c2->isRunning())
    {
        try
        {
            Log(L"Sending beacon check-in...");

            vector<wstring> commands = c2->checkIn();

            Log(L"Received " + to_wstring(commands.size()) + L" command(s)");

            if (commands.empty())
            {
                Log(L"No commands returned");

                c2->sendResult(
                    L"checkin",
                    L"success",
                    L"CHECKIN_OK|No task returned");
            }

            for (const wstring &cmdLine : commands)
            {
                Log(L"Processing command: " + cmdLine);

                size_t pos = cmdLine.find(L'|');

                wstring cmd =
                    (pos == wstring::npos)
                        ? cmdLine
                        : cmdLine.substr(0, pos);

                wstring args =
                    (pos == wstring::npos)
                        ? L""
                        : cmdLine.substr(pos + 1);

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

                    result =
                        L"SUCCESS|Beacon interval set to " +
                        to_wstring(seconds) +
                        L" seconds";
                }
                else
                {
                    result =
                        L"IGNORED|Unsupported benign lab command: " +
                        cmd;
                }

                Log(L"Sending result: " + result);

                c2->sendResult(cmd, L"success", result);
            }

            DWORD intervalSec =
                c2->getBeaconIntervalMs() / 1000;

            Log(
                L"Sleeping for " +
                to_wstring(intervalSec) +
                L" seconds");

            Sleep(c2->getBeaconIntervalMs());
        }
        catch (...)
        {
            Log(L"Exception occurred during beacon cycle");
            Log(L"Sleeping 30 seconds before retry");

            Sleep(30000);
        }
    }

    Log(L"Beacon thread exiting");

    return 0;
}

int main()
{
    wstring c2ServerUrl = L"https://c2.lab.local";

    try
    {
        Log(L"A05_1b HTTPS beacon initialized");
        Log(L"C2 Server: " + c2ServerUrl);

        g_c2Client = new C2Client(c2ServerUrl);

        Log(L"Testing connectivity via /api/ping");

        if (g_c2Client->testConnection())
        {
            Log(L"Connectivity test succeeded");
        }
        else
        {
            Log(L"Connectivity test failed");
        }

        HANDLE hBeaconThread =
            CreateThread(
                NULL,
                0,
                c2BeaconThread,
                g_c2Client,
                0,
                NULL);

        if (hBeaconThread)
        {
            Log(L"Beacon thread created successfully");

            WaitForSingleObject(
                hBeaconThread,
                INFINITE);

            CloseHandle(hBeaconThread);
        }
        else
        {
            Log(L"Failed to create beacon thread");

            delete g_c2Client;
            g_c2Client = nullptr;

            return 1;
        }

        Log(L"Cleaning up client");

        delete g_c2Client;
        g_c2Client = nullptr;
    }
    catch (...)
    {
        Log(L"Fatal exception occurred");

        return 1;
    }

    Log(L"Program exiting normally");

    return 0;
}