# A03.5 Network-to-Memory Loader

## Summary

Downloads a raw payload blob from a controlled lab HTTP server,
stores it only in memory, marks it executable, and calls it directly.

The sample demonstrates network-to-memory loader behavior commonly
associated with fileless staging, payload retrieval, memory-only
execution, and loader handoff workflows.

Unlike `A03_2`, this specimen does **not** write the downloaded payload
to disk before execution. The downloaded bytes are stored in a memory
buffer, copied into dynamically allocated memory, changed from RW to RX,
and invoked as a callable payload.

The downloaded payload is a raw shellcode-style binary generated from:

```text
file_artifact.asm
```

The hosted payload file is:

```text
file_artifact.bin
```

When executed successfully, the in-memory payload writes a controlled
confirmation artifact to disk.

---

## Payload Summary

The payload behavior is the download and in-memory execution of:

```text
file_artifact.bin
```

The loader retrieves the payload from:

```text
http://192.168.67.5/file_artifact.bin
```

The payload is never staged to disk.

When executed, the in-memory payload:

* Receives a `PARAMS` structure from the loader
* Uses loader-provided WinAPI function pointers
* Retrieves its process identifier
* Captures execution timestamp information
* Determines the current process path
* Writes a confirmation artifact to:

```text
C:\Users\Public\A03_5_IN_MEMORY_PAYLOAD_EXECUTED.txt
```

The artifact contains:

* Timestamp
* Process ID
* Process path
* Execution mode
* Confirmation that the payload was not staged to disk

The payload additionally emits a debug string using:

```text
OutputDebugStringA()
```

No persistence, credential theft, privilege escalation,
or destructive functionality is performed.

---

## To Execute A03_5

Make sure the controlled lab HTTP server is reachable at:

```text
192.168.67.5:80
```

Host the raw payload at:

```text
/file_artifact.bin
```

Then run the network-to-memory loader directly:

```powershell
.\A03-Dropper-Loader-Stager\A03_5\bin\A03_5_network_to_mem_loader.exe
```

The sample will download the raw payload, copy it into memory, mark the
memory executable, and call it directly.

---

## Expected Loader Artifacts

### Network Request

```text
GET /file_artifact.bin HTTP/1.1
Host: 192.168.67.5
User-Agent: Mozilla/5.0
```

### Output File

```text
C:\Users\Public\A03_5_IN_MEMORY_PAYLOAD_EXECUTED.txt
```

### Example Contents

```text
THESIS_A03_5_NETWORK_TO_MEMORY_PAYLOAD_EXECUTED
Timestamp: 2026-05-25 14:10:33
PID: 1234
ProcessPath: C:\Path\To\A03_5_network_to_mem_loader.exe
ExecutionMode: downloaded_to_memory_and_called_as_raw_payload
PayloadStagedToDisk: false
```

---

## Expected Memory Behavior

The loader should exhibit the following memory-staging pattern:

```text
WinHttpOpen()
WinHttpConnect()
WinHttpOpenRequest()
WinHttpSendRequest()
WinHttpReceiveResponse()
WinHttpQueryDataAvailable()
WinHttpReadData()
VirtualAlloc()
memcpy()
VirtualProtect()
direct function-pointer call
VirtualFree()
```

---

#########################################################################

# High-Level Network-to-Memory Loader Flow

1. Start the network-to-memory loader:

   ```text
   A03_5_network_to_mem_loader.exe
   ```

2. Open a WinHTTP session using:

   ```text
   WinHttpOpen()
   ```

3. Connect to the controlled lab HTTP server:

   ```text
   192.168.67.5:80
   ```

4. Create an HTTP GET request for:

   ```text
   /file_artifact.bin
   ```

5. Send the request using:

   ```text
   WinHttpSendRequest()
   ```

6. Receive the HTTP response using:

   ```text
   WinHttpReceiveResponse()
   ```

7. Query available response data using:

   ```text
   WinHttpQueryDataAvailable()
   ```

8. Read the raw payload bytes using:

   ```text
   WinHttpReadData()
   ```

9. Store the downloaded bytes in an in-memory buffer

10. Close WinHTTP handles after download

11. Allocate writable memory using:

    ```text
    VirtualAlloc(..., PAGE_READWRITE)
    ```

12. Copy the downloaded payload bytes into allocated memory using:

    ```text
    memcpy()
    ```

13. Change memory protection from RW to RX using:

    ```text
    VirtualProtect(..., PAGE_EXECUTE_READ, ...)
    ```

14. Populate a `PARAMS` structure with WinAPI function pointers:

    ```text
    CreateFileA
    WriteFile
    CloseHandle
    GetCurrentProcessId
    GetModuleFileNameA
    GetLocalTime
    wsprintfA
    OutputDebugStringA
    ```

15. Cast the allocated memory region to a callable function pointer

16. Call the downloaded payload directly:

    ```text
    Run(&pParams)
    ```

17. In-memory payload collects:

    * PID
    * Timestamp
    * Process path

18. In-memory payload writes confirmation artifact to:

    ```text
    C:\Users\Public\A03_5_IN_MEMORY_PAYLOAD_EXECUTED.txt
    ```

19. Emit debug confirmation string using:

    ```text
    OutputDebugStringA()
    ```

20. Release the staged memory region using:

    ```text
    VirtualFree()
    ```

21. Exit cleanly after memory-only payload execution.


## INetSim Configuration for A03_5

This sample expects `file_artifact.bin` to be hosted by the Ubuntu
INetSim server over HTTP.

---

### Copy Payload to INetSim Fakefile Directory

```bash
sudo cp file_artifact.bin /var/lib/inetsim/http/fakefiles/file_artifact.bin
sudo chown inetsim:inetsim /var/lib/inetsim/http/fakefiles/file_artifact.bin
```

---

### Configure Static Fakefile Mapping

Edit:

```text
/etc/inetsim/inetsim.conf
```

Add:

```text
http_static_fakefile /file_artifact.bin file_artifact.bin application/octet-stream
```

This serves the raw payload when the loader requests:

```text
/file_artifact.bin
```

---

### Restart INetSim

```bash
sudo systemctl restart inetsim
```

---

### Expected INetSim Log Activity

```text
[http_80_tcp] recv: GET /file_artifact.bin HTTP/1.1
[http_80_tcp] send: HTTP/1.1 200 OK
```
