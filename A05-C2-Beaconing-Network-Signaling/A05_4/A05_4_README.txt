A05_4 TCP Reverse Shell

Summary
Establishes an outbound TCP connection from the Windows 
analysis VM to a controlled listener at 192.168.67.5:8080, 
then launches cmd.exe locally and redirects its standard input, 
output, and error streams over the established socket. Commands 
received from the remote listener are written into the shell’s 
standard input, while command output is asynchronously read and 
sent back over the TCP stream.

This demonstrates interactive reverse shell behavior commonly 
associated with backdoors, remote access trojans, and 
post-compromise command-and-control tooling. Unlike beacon-only 
samples, this specimen provides an interactive command channel 
where the controlled listener drives execution after the implant 
connects outbound.

Payload Summary
The payload behavior is the interactive cmd.exe process spawned 
by the sample. No additional binary payload is downloaded or staged. 
The sample redirects cmd.exe I/O over the TCP socket to provide 
controlled proof of remote command execution behavior inside the lab.

To execute A05_4_reverse_tcp_shell:
.\A05-Beaconing-C2-Networking\A05_4\bin\A05_4_reverse_tcp_shell.exe

#########################################################################

High-Level Reverse Shell Flow:

1. Configure remote listener IP as 192.168.67.5
2. Configure remote listener port as 8080
3. Create a .NET TcpClient
4. Connect outbound to the listener
5. Retrieve the socket NetworkStream
6. Wrap the stream with StreamReader and StreamWriter
7. Launch C:\Windows\System32\cmd.exe
8. Redirect standard input, output, and error streams
9. Read commands from the TCP socket
10. Write received commands into cmd.exe standard input
11. Send command output and errors back over the TCP socket
12. Clean up socket and process resources when the connection closes

#########################################################################

Listener Setup on Ubuntu / INetSim Server

Start a listener on the Ubuntu INetSim VM:
nc -lvnp 8080

Then execute the sample from the Windows analysis VM.

Once connected, the listener should receive an interactive command shell. 

Example benign validation commands:
whoami
hostname
cd
echo A05_4_REVERSE_TCP_SHELL_TEST