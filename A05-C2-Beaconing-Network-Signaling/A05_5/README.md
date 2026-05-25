# A05_5 TCP Bind Shell

## Summary

Opens a TCP listener on the Windows analysis VM and binds an interactive
`cmd.exe` process to the accepted client connection.

The sample demonstrates bind shell behavior commonly associated with
interactive command-and-control, inbound remote access, socket-based
standard-handle redirection, and exposed shell access over TCP.

Unlike `A05_4`, which initiates an outbound reverse connection to a
remote listener, this specimen listens locally for an inbound TCP
connection. After a client connects, the sample launches `cmd.exe` and
redirects its standard input, output, and error handles over the
accepted socket.

The controlled bind port is:

```text
8080
```

This sample is intended only for isolated lab execution and controlled
dynamic-analysis artifact generation.

---

## Behavior Summary

The bind shell behavior is implemented using:

```text
WSAStartup()
WSASocketA()
bind()
listen()
accept()
CreateProcessA()
WaitForSingleObject()
```

When executed, the sample:

* Initializes Winsock
* Creates a TCP listening socket
* Binds to all local interfaces using `INADDR_ANY`
* Listens on TCP port `8080`
* Accepts one inbound client connection
* Launches `cmd.exe`
* Redirects standard handles over the accepted socket
* Waits for the interactive shell process to exit
* Cleans up sockets and process handles

No persistence, credential theft, file dropping, privilege escalation,
or destructive functionality is performed.

---

## To Execute A05_5

Run the bind shell sample on the Windows analysis VM:

```powershell
.\A05-Beaconing-Interactive-C2\A05_5\bin\A05_5_tcp_bind_shell.exe
```

The sample will listen on:

```text
0.0.0.0:8080
```

From the controlled lab client, connect to the Windows analysis VM on
TCP port `8080`.

Example lab connection:

```bash
nc <windows_vm_ip> 8080
```

Successful connection should provide an interactive `cmd.exe` session
over the TCP stream.

---

## Expected Network Artifacts

### TCP Listener

```text
Local Address: 0.0.0.0
Local Port: 8080
Protocol: TCP
Direction: Inbound
```

### Expected Dynamic Signals

Dynamic analysis tools may observe:

* Listening TCP socket on port `8080`
* Inbound TCP connection from the lab client
* Child process creation of:

```text
C:\Windows\System32\cmd.exe
```

* Standard handle redirection to a socket
* Interactive command-response traffic over TCP
* No DNS dependency
* No HTTP or HTTPS traffic
* No persistence behavior

---

## Expected Process Artifacts

### Child Process

```text
cmd.exe
```

### Parent Process

```text
A05_5_tcp_bind_shell.exe
```

### StartupInfo Standard Handle Redirection

```text
STARTF_USESTDHANDLES
STARTF_USESHOWWINDOW
SW_HIDE
bInheritHandles = TRUE
```

The socket is assigned to:

```text
hStdInput
hStdOutput
hStdError
```

This causes the spawned shell process to route input and output through
the accepted TCP connection.

---

## Firewall / Lab Access Note

Because this sample listens for inbound TCP traffic, the Windows
analysis VM must allow inbound connections to TCP port `8080` from the
controlled lab client.

Example restricted Windows Firewall rule:

```powershell
New-NetFirewallRule `
  -DisplayName "Allow A05_5 Bind Shell TCP 8080" `
  -Direction Inbound `
  -Protocol TCP `
  -LocalPort 8080 `
  -RemoteAddress 192.168.67.5 `
  -Action Allow
```

Restricting the source IP keeps the sample scoped to the isolated lab
sink host rather than exposing the listener broadly. Tiny firewall
rules, huge sanity gains.

---

#########################################################################

# High-Level TCP Bind Shell Flow

1. Start the TCP bind shell sample:

   ```text
   A05_5_tcp_bind_shell.exe
   ```

2. Initialize Winsock using:

   ```text
   WSAStartup()
   ```

3. Create a TCP socket using:

   ```text
   WSASocketA(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0)
   ```

4. Configure the local listener address:

   ```text
   INADDR_ANY
   ```

5. Configure the listener port:

   ```text
   8080
   ```

6. Bind the socket using:

   ```text
   bind()
   ```

7. Start listening for inbound connections using:

   ```text
   listen()
   ```

8. Accept one inbound client connection using:

   ```text
   accept()
   ```

9. Close the original listening socket after the client connects

10. Prepare `STARTUPINFOA` for child process handle redirection

11. Enable standard-handle inheritance using:

    ```text
    STARTF_USESTDHANDLES
    ```

12. Assign the accepted socket to:

    ```text
    hStdInput
    hStdOutput
    hStdError
    ```

13. Launch the shell process using:

    ```text
    CreateProcessA()
    ```

14. Spawn:

    ```text
    C:\Windows\System32\cmd.exe
    ```

15. Inherit the socket-backed handles into the child process

16. Route command input and output over the TCP connection

17. Wait for the shell process to exit using:

    ```text
    WaitForSingleObject()
    ```

18. Close process and thread handles

19. Close the accepted client socket

20. Clean up Winsock using:

    ```text
    WSACleanup()
    ```

21. Exit cleanly after the bind shell session ends.

## Client Connection Setup from Ubuntu / INetSim Server

Start the bind shell sample on the Windows analysis VM first:

```powershell
.\A05-Beaconing-Interactive-C2\A05_5\bin\A05_5_tcp_bind_shell.exe
```

The sample should begin listening on:

```text
0.0.0.0:8080
```

---

### Connect from the Ubuntu INetSim VM

From the Ubuntu / INetSim server, connect to the Windows analysis VM
using Netcat:

```bash
nc -nv 192.168.67.3 8080
```

Example successful connection output:

```text
Connection to 192.168.67.3 8080 port [tcp/*] succeeded!
```

After the connection is established, the remote `cmd.exe` session
should become interactive over the TCP stream.

---

### Example Benign Validation Commands

Example validation commands:

```text
whoami
hostname
cd
echo A05_5_TCP_BIND_SHELL_TEST
```

These commands provide controlled interactive artifacts for dynamic
analysis without modifying system configuration or performing
destructive actions.

---

### Expected Interactive Output

Example session:

```text
C:\Windows\system32>whoami
win11lab\analyst

C:\Windows\system32>hostname
WIN11-LAB

C:\Windows\system32>echo A05_5_TCP_BIND_SHELL_TEST
A05_5_TCP_BIND_SHELL_TEST
```

---

### Expected Dynamic Signals During Interaction

Dynamic analysis tools may observe:

* Established inbound TCP session from:

  ```text
  192.168.67.5 → 192.168.67.3:8080
  ```

* Interactive command traffic over the socket

* Console process activity from `cmd.exe`

* Standard input/output redirected to the network socket

* No HTTP, HTTPS, or DNS traffic associated with the shell session

