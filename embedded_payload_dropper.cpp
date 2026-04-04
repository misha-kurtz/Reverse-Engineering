#include <windows.h>

unsigned char payload[] = { /* small test exe bytes */ };

int main() {
    HANDLE hFile = CreateFile("payload.exe", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written;
    WriteFile(hFile, payload, sizeof(payload), &written, NULL);
    CloseHandle(hFile);

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    CreateProcess("payload.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    return 0;
}