A05_5 TCP Bind Shell

Summary
Creates a TCP bind shell on the Windows analysis VM by 
opening a listening socket on port 8080, accepting an inbound
client connection, and spawning cmd.exe with its standard input, 
output, and error handles redirected directly over the accepted socket. 
Unlike the reverse shell sample, this specimen does not initiate an 
outbound connection. Instead, it waits for a remote operator or test 
listener to connect inbound to the infected host.

This demonstrates classic bind shell behavior commonly associated 
with backdoors, remote access trojans, and post-compromise command 
execution utilities. The sample exposes an interactive command channel 
over TCP and uses native Windows process creation and handle inheritance 
to route shell I/O through the connected socket.

Payload Summary
The payload behavior is the interactive cmd.exe process spawned 
after a client connects to the listening socket. No additional binary 
payload is downloaded, staged, or executed. The sample provides 
controlled proof of bind shell behavior by allowing a lab client to 
connect to the Windows VM and interact with the spawned command 
shell over TCP.

To execute A05_5_tcp_bind_shell:
.\A05-Beaconing-C2-Networking\A05_5\bin\A05_5_tcp_bind_shell.exe

#########################################################################

High-Level Bind Shell Flow:

1. Initialize Winsock using WSAStartup
2. Create IPv4 TCP listening socket using WSASocketA
3. Configure socket address:
    Address: INADDR_ANY
    Port: 8080
4. Bind socket to local port 8080
5. Begin listening with listen
6. Accept inbound client connection with accept
7. Close the original listening socket after connection is accepted
8. Configure STARTUPINFOA with inherited standard handles
9. Map cmd.exe standard input, output, and error to the accepted socket
10. Launch C:\Windows\System32\cmd.exe using CreateProcessA
11. Wait for the shell process to exit
12. Clean up process handles, socket handles, and Winsock state

#########################################################################

Client Connection Setup from Ubuntu / INetSim Server

Start the bind shell sample on the Windows analysis VM first.

Then connect to the Windows VM from the Ubuntu INetSim VM:
nc -nv 192.168.67.3 8080

Example benign validation commands:
whoami
hostname
cd
echo A05_5_TCP_BIND_SHELL_TEST