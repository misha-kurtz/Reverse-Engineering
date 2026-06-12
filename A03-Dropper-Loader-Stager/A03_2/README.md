# A03.2 Download-and-Execute Stager

## Summary

Downloads a benign executable payload from a controlled lab HTTP
server, writes it to disk, and executes it as a child process.

The sample demonstrates network stager behavior commonly associated
with malware downloaders, staged payload retrieval, loader handoff,
and first-stage delivery workflows where the initial executable is
responsible for retrieving a secondary payload from a remote location.

Unlike embedded droppers that carry the payload inside the original
binary, this specimen retrieves the payload over HTTP using WinHTTP,
creating clear network, file-system, and process-creation artifacts
for dynamic analysis.

The downloaded payload is a benign marker executable:

```text
A03_2_dropped_exe.exe
```

When executed successfully, the downloaded executable writes a
controlled confirmation artifact to disk.

The payload itself is intentionally benign and exists only to generate
controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the download and execution of
`A03_2_dropped_exe.exe` after successful network staging.

The stager downloads the payload from the controlled lab server:

```text
http://192.168.67.5/A03_2_dropped_exe.exe
```

It saves the downloaded executable to:

```text
C:\Users\Public\A03_2_dropped_exe.exe
```

When executed, the downloaded executable:

* Determines its own process path
* Retrieves its process identifier
* Captures execution timestamp information
* Writes a confirmation artifact to:

```text
C:\Users\Public\A03_2_Network_Stager_OK.txt
```

The artifact contains:

* Timestamp
* Process ID
* Process path
* Network stager execution confirmation string

The payload additionally emits a debug string using:

```text
OutputDebugStringA()
```

No persistence, credential theft, privilege escalation,
or destructive functionality is performed.

---

## To Execute A03_2

Make sure the controlled lab HTTP server is reachable at:

```text
192.168.67.5:80
```

Host the benign payload at:

```text
/A03_2_dropped_exe.exe
```

Then run the network stager directly:

```powershell
.\A03-Dropper-Loader-Stager\A03_2\bin\A03_2_download_and_execute_stager.exe
```

The sample will download the payload, write it to disk, and execute it
with:

```text
CreateProcessA()
```

---

## Expected Stager Artifacts

### Network Request

```text
GET /A03_2_dropped_exe.exe HTTP/1.1
Host: 192.168.67.5
User-Agent: A03_2_WinHTTP/1.0
```

### Dropped Payload

```text
C:\Users\Public\A03_2_dropped_exe.exe
```

### Output File

```text
C:\Users\Public\A03_2_Network_Stager_OK.txt
```

### Example Contents

```text
THESIS_A03_2_EXE_DROPPED_AND_EXECUTED_VIA_NETWORK_STAGER
Timestamp: 2026-05-25 14:10:33
PID: 1234
ProcessPath: C:\Users\Public\A03_2_dropped_exe.exe
```

---

#########################################################################

# High-Level Download-and-Execute Stager Flow

1. Start the network stager:

   ```text
   A03_2_download_and_execute_stager.exe
   ```

2. Prepare the local output path:

   ```text
   C:\Users\Public\A03_2_dropped_exe.exe
   ```

3. Create the local output file using:

   ```text
   CreateFileA()
   ```

4. Open a WinHTTP session using:

   ```text
   WinHttpOpen()
   ```

5. Connect to the controlled lab HTTP server using:

   ```text
   WinHttpConnect()
   ```

6. Create an HTTP GET request for:

   ```text
   /A03_2_dropped_exe.exe
   ```

   using:

   ```text
   WinHttpOpenRequest()
   ```

7. Send the HTTP request using:

   ```text
   WinHttpSendRequest()
   ```

8. Receive the HTTP response using:

   ```text
   WinHttpReceiveResponse()
   ```

9. Read the response body in chunks using:

   ```text
   WinHttpReadData()
   ```

10. Write each downloaded chunk to disk using:

    ```text
    WriteFile()
    ```

11. Close the dropped payload file handle

12. Launch the downloaded payload using:

    ```text
    CreateProcessA()
    ```

13. Begin execution of:

    ```text
    A03_2_dropped_exe.exe
    ```

14. Collect execution metadata:

    * PID
    * Timestamp
    * Process path

15. Write network stager confirmation artifact to:

    ```text
    C:\Users\Public\A03_2_Network_Stager_OK.txt
    ```

16. Emit a debug confirmation string using:

    ```text
    OutputDebugStringA()
    ```

17. Close WinHTTP handles using:

    ```text
    WinHttpCloseHandle()
    ```

18. Exit cleanly after payload download and execution


## INetSim Configuration for A03_2

This sample expects a controlled HTTP payload delivery workflow
using an Ubuntu-based INetSim server.

The benign payload executable:

```text
A03_2_dropped_exe.exe
```

must be hosted by the INetSim HTTP service so the Windows stager
can retrieve it over the isolated malware-analysis network.

---

### Copy Payload to INetSim Fakefile Directory

On the Ubuntu INetSim server:

```bash
sudo cp A03_2_dropped_exe.exe /var/lib/inetsim/http/fakefiles/A03_2_dropped_exe.exe
sudo chown inetsim:inetsim /var/lib/inetsim/http/fakefiles/A03_2_dropped_exe.exe
```

---

### Configure HTTP Fakefile Mapping

Edit the INetSim configuration file:

```text
/etc/inetsim/inetsim.conf
```

Add the following mapping:

```text
http_fakefile exe A03_2_dropped_exe.exe application/octet-stream
```

This instructs INetSim to serve the executable payload when the
stager requests:

```text
/A03_2_dropped_exe.exe
```

over HTTP.

---

### Restart INetSim

After updating the configuration, restart the INetSim service:

```bash
sudo systemctl restart inetsim
```

---

### Verify Payload Hosting

From the Windows analysis VM or another host on the isolated lab
network, verify payload accessibility:

```powershell
curl http://192.168.67.5/A03_2_dropped_exe.exe -o test.exe
```

or from Linux:

```bash
curl -O http://192.168.67.5/A03_2_dropped_exe.exe
```

---

### Expected INetSim Service Log Activity

Successful staging activity should generate HTTP request entries
inside:

```text
/var/log/inetsim/service.log
```

Example:

```text
[http_80_tcp] recv: GET /A03_2_dropped_exe.exe HTTP/1.1
[http_80_tcp] send: HTTP/1.1 200 OK
```

These artifacts provide clear network-delivery telemetry for
dynamic malware-analysis workflows.

