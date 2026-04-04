#include <windows.h>
#include <stdio.h>

int main()
{
    // Mock payload bytes: inert data, not executable code
    unsigned char payload[] = {
        0x90, 0x90, 0x90, 0x90,
        0x41, 0x42, 0x43, 0x44,
        0xCC, 0xCC, 0x00, 0xFF};

    SIZE_T payloadSize = sizeof(payload);

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
    printf("Safe specimen: execution step intentionally omitted.\n");

    // Keep process alive briefly so tools can observe the state
    Sleep(5000);

    VirtualFree(mem, 0, MEM_RELEASE);
    return 0;
}