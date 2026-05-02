#include <windows.h>

unsigned char payload[] = {0x41, 0x41, 0x41, 0x41}; // small test bytes "AAAA"

int main() {
    HANDLE hFile = CreateFileA("payload.exe", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written;
    WriteFile(hFile, payload, sizeof(payload), &written, NULL);
    CloseHandle(hFile);

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    CreateProcessA("payload.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    return 0;
}