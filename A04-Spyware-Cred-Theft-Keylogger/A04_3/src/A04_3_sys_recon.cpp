/*
   A04_3: System/Host Reconnaissance Control Sample (Complete Consolidated Log)
   Behavioral Scope: Local host, accounts, processes, network sockets, registry software, routing, and hotfixes.
   Logs output natively to a single public text file.
*/

#include <iostream>
#include <windows.h>
#include <winuser.h>
#include <ctime>
#include <fstream>
#include <string>
#include <Lmcons.h>
#include <stdio.h>

// Add these explicit linker directives to resolve LNK2019 errors
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "User32.lib")

using namespace std;

// Global Configuration Elements
const string logPath = "C:\\Users\\Public\\A04_3_System_Host_Recon_log.txt";
string systemuser;

void getusername(void)
{
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameA(username, &username_len))
    {
        systemuser = username;
    }
    else
    {
        systemuser = "Unknown_User";
    }
}

void showsysteminfo(void)
{
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);

    ofstream sysfile;
    sysfile.open(logPath.c_str(), ios::out); // Initial write overwrites/resets the file

    if (sysfile.is_open())
    {
        time_t now = time(0);
        char *dt = ctime(&now);

        sysfile << "==================================================\n";
        sysfile << "===       A04_3 HOST RECONNAISSANCE LOG        ===\n";
        sysfile << "==================================================\n";
        if (dt)
        {
            sysfile << "Execution Timestamp : " << dt;
        }
        sysfile << "System User Account : " << systemuser << "\n";
        sysfile << "OEM ID              : " << siSysInfo.dwOemId << "\n";
        sysfile << "Number of Processors: " << siSysInfo.dwNumberOfProcessors << "\n";
        sysfile << "Page Size           : " << siSysInfo.dwPageSize << " bytes\n";
        sysfile << "Processor Type      : " << siSysInfo.dwProcessorType << "\n";
        sysfile << "Min App Address     : " << siSysInfo.lpMinimumApplicationAddress << "\n";
        sysfile << "Max App Address     : " << siSysInfo.lpMaximumApplicationAddress << "\n";
        sysfile << "Active Proc Mask    : " << siSysInfo.dwActiveProcessorMask << "\n";
        sysfile << "==================================================\n\n";
        sysfile.close();
    }
}

void storemacaddress(void)
{
    ofstream sysfile(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "--- MAC ADDRESSES ---\n";
        sysfile.close();
    }

    system("getmac >> C:\\Users\\Public\\A04_3_System_Host_Recon_log.txt");

    sysfile.open(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "\n";
        sysfile.close();
    }
}

void storeipaddress(void)
{
    ofstream sysfile(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "--- NETWORK CONFIGURATION (IPCONFIG) ---\n";
        sysfile.close();
    }

    system("ipconfig /all >> C:\\Users\\Public\\A04_3_System_Host_Recon_log.txt");

    sysfile.open(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "\n";
        sysfile.close();
    }
}

void storeexternalip(void)
{
    ofstream sysfile(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "--- PUBLIC IP RESOLUTION ---\n";
        sysfile.close();
    }

    system("nslookup myip.opendns.com resolver1.opendns.com >> C:\\Users\\Public\\A04_3_System_Host_Recon_log.txt");

    sysfile.open(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "\n";
        sysfile.close();
    }
}

void storeroutingtable(void)
{
    ofstream sysfile(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "--- NETWORK ROUTING TABLE ---\n";
        sysfile.close();
    }

    system("route print >> C:\\Users\\Public\\A04_3_System_Host_Recon_log.txt");

    sysfile.open(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "\n";
        sysfile.close();
    }
}

void storeuseraccounts(void)
{
    ofstream sysfile(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "--- LOCAL USER ACCOUNTS ---\n";
        sysfile.close();
    }

    system("net user >> C:\\Users\\Public\\A04_3_System_Host_Recon_log.txt");

    sysfile.open(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "\n";
        sysfile.close();
    }
}

void storelocalgroups(void)
{
    ofstream sysfile(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "--- LOCAL SECURITY GROUPS ---\n";
        sysfile.close();
    }

    system("net localgroup >> C:\\Users\\Public\\A04_3_System_Host_Recon_log.txt");

    sysfile.open(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "\n";
        sysfile.close();
    }
}

void storenetworkconnections(void)
{
    ofstream sysfile(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "--- ACTIVE NETWORK CONNECTIONS (NETSTAT) ---\n";
        sysfile.close();
    }

    system("netstat -ano >> C:\\Users\\Public\\A04_3_System_Host_Recon_log.txt");

    sysfile.open(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "\n";
        sysfile.close();
    }
}

void storeprocesslist(void)
{
    ofstream sysfile(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "--- RUNNING PROCESSES (TASKLIST) ---\n";
        sysfile.close();
    }

    system("tasklist /v >> C:\\Users\\Public\\A04_3_System_Host_Recon_log.txt");

    sysfile.open(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "\n";
        sysfile.close();
    }
}

void storeinstalledapps(void)
{
    ofstream sysfile(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "--- INSTALLED SOFTWARE INVENTORY (REGISTRY) ---\n";
        sysfile << "[64-Bit Applications]\n";
        sysfile.close();
    }

    system("reg query HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall /s /v DisplayName 2>nul | findstr DisplayName >> C:\\Users\\Public\\A04_3_System_Host_Recon_log.txt");

    sysfile.open(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "\n[32-Bit Applications]\n";
        sysfile.close();
    }

    system("reg query HKLM\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall /s /v DisplayName 2>nul | findstr DisplayName >> C:\\Users\\Public\\A04_3_System_Host_Recon_log.txt");

    sysfile.open(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "\n";
        sysfile.close();
    }
}

void storehotfixes(void)
{
    ofstream sysfile(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "--- INSTALLED SECURITY PATCHES & HOTFIXES ---\n";
        sysfile.close();
    }

    // Queries the Quick Fix Engineering management framework for installed patch data
    system("wmic qfe get HotFixID,Description,InstalledOn >> C:\\Users\\Public\\A04_3_System_Host_Recon_log.txt 2>nul");

    sysfile.open(logPath.c_str(), ios::app);
    if (sysfile.is_open())
    {
        sysfile << "==================================================\n";
        sysfile.close();
    }
}

int main()
{
    // Hide the console window when executing
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);

    // Collect variables and build the monolithic tracking structure sequentially
    getusername();
    showsysteminfo();
    storeipaddress();
    storemacaddress();
    storeexternalip();
    storeroutingtable();
    storeuseraccounts();
    storelocalgroups();
    storenetworkconnections();
    storeprocesslist();
    storeinstalledapps();

    // Execute Newly Added Patch Audit routine
    storehotfixes();

    return 0;
}