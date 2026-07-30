# A04.3 System and Host Recon

## Summary

Collects local host, user, process, network, software, routing, and
patch information from a Windows system and consolidates the results
into a single text file.

The sample demonstrates host reconnaissance behavior commonly
associated with information stealers, remote-access trojans,
post-exploitation frameworks, loaders, and malware that surveys an
infected system before selecting additional actions.

The reconnaissance workflow performs the following operations:

* Retrieves the current user account
* Collects processor and memory-address information
* Enumerates local network configuration
* Enumerates MAC addresses
* Attempts to resolve the system public IP address
* Collects the local routing table
* Enumerates local user accounts
* Enumerates local security groups
* Collects active network connections
* Enumerates running processes
* Queries installed 32-bit and 64-bit software
* Enumerates installed Windows hotfixes
* Writes all collected information to a consolidated log file

The sample combines native Windows API calls with built-in Windows
command-line utilities.

The implementation is intentionally non-destructive and exists solely
to generate controlled host-discovery and reconnaissance artifacts for
reverse engineering and dynamic malware analysis.

---

## Payload Summary

The payload behavior consists of collecting system and host metadata
and writing the results to:

```text
C:\Users\Public\A04_3_System_Host_Recon_OK.txt
```

During execution, the sample:

* Hides its console window
* Retrieves the current user name
* Retrieves basic processor and architecture information
* Executes multiple built-in Windows discovery commands
* Redirects command output into a single report
* Organizes the report into labeled sections
* Terminates after all reconnaissance routines complete

The report may contain:

* Execution timestamp
* Current user account
* Processor count
* Processor type
* Memory page size
* Application address boundaries
* IP address information
* DNS configuration
* Network adapter details
* MAC addresses
* Public IP lookup results
* IPv4 and IPv6 routing tables
* Local user accounts
* Local groups
* Active TCP and UDP sockets
* Process IDs and process names
* Installed applications
* Installed security patches and hotfixes

No persistence, credential extraction, privilege escalation,
data exfiltration, or destructive functionality is performed.

---

## To Execute A04_3

### Step 1 — Launch the reconnaissance sample

```powershell
.\A04-Credential-Theft-Spyware-Keylogger\A04_3\bin\A04_3_System_Host_Recon.exe
```

The sample does not require command-line arguments.

The console window is hidden during execution, so output is written
directly to the reconnaissance log.

---

## Expected Reconnaissance Artifact

### Output File

```text
C:\Users\Public\A04_3_System_Host_Recon_OK.txt
```

### Example Contents

```text
==================================================
===       A04_3 HOST RECONNAISSANCE LOG        ===
==================================================
Execution Timestamp : Thu Jul 30 13:15:42 2026
System User Account : lab-user
OEM ID              : 0
Number of Processors: 8
Page Size           : 4096 bytes
Processor Type      : 8664
Min App Address     : 0000000000010000
Max App Address     : 00007FFFFFFEFFFF
Active Proc Mask    : 255
==================================================

--- NETWORK CONFIGURATION (IPCONFIG) ---

Windows IP Configuration

   Host Name . . . . . . . . . . . . : WIN11-LAB
   IPv4 Address. . . . . . . . . . . : 192.168.130.100
   Default Gateway . . . . . . . . . : 192.168.130.1

--- MAC ADDRESSES ---

Physical Address    Transport Name
=================== ==========================================================
00-11-22-33-44-55   \Device\Tcpip_{EXAMPLE-GUID}

--- LOCAL USER ACCOUNTS ---

User accounts for \\WIN11-LAB

-------------------------------------------------------------------------------
Administrator            lab-user                 DefaultAccount

--- ACTIVE NETWORK CONNECTIONS (NETSTAT) ---

Proto  Local Address          Foreign Address        State           PID
TCP    192.168.130.100:49721  192.168.130.1:53       ESTABLISHED     2480

--- RUNNING PROCESSES (TASKLIST) ---

Image Name                     PID Session Name        Session#    Mem Usage
========================= ======== ================ =========== ============
explorer.exe                  4120 Console                    1    128,940 K

--- INSTALLED SECURITY PATCHES & HOTFIXES ---

Description      HotFixID       InstalledOn
Update           KB5030000      7/15/2026
==================================================
```

Actual contents depend on the system configuration, active processes,
network state, installed software, and available Windows utilities.

---

## Reconnaissance Commands

The sample invokes the following built-in Windows commands.

### Network Configuration

```text
ipconfig /all
```

Collects:

* Host name
* DNS suffixes
* Network adapter names
* IPv4 and IPv6 addresses
* Default gateway
* DHCP status
* DNS servers

### MAC Address Enumeration

```text
getmac
```

Collects physical network adapter addresses and transport identifiers.

### Public IP Resolution

```text
nslookup myip.opendns.com resolver1.opendns.com
```

Attempts to determine the system public IP address using an external
DNS resolver.

This command generates outbound DNS traffic and may fail in an
isolated laboratory environment unless the request is emulated or
forwarded.

### Routing Table Enumeration

```text
route print
```

Collects:

* Interface list
* IPv4 routes
* IPv6 routes
* Network destinations
* Gateways
* Route metrics

### Local User Enumeration

```text
net user
```

Enumerates local Windows user accounts.

### Local Group Enumeration

```text
net localgroup
```

Enumerates local Windows security groups.

### Active Connection Enumeration

```text
netstat -ano
```

Collects:

* Active TCP connections
* Listening TCP ports
* UDP endpoints
* Local and remote addresses
* Connection states
* Associated process IDs

### Process Enumeration

```text
tasklist /v
```

Collects:

* Process names
* Process IDs
* Session names
* Session IDs
* Memory usage
* Process status
* User context
* Window titles

### Installed Software Enumeration

```text
reg query HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall /s /v DisplayName
```

Queries installed 64-bit applications.

```text
reg query HKLM\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall /s /v DisplayName
```

Queries installed 32-bit applications.

### Hotfix Enumeration

```text
wmic qfe get HotFixID,Description,InstalledOn
```

Collects installed Windows updates, security patches, and hotfix
identifiers.

---

#########################################################################

# High-Level System Reconnaissance Flow

1. Start the reconnaissance executable

   ```text
   A04_3_System_Host_Recon.exe
   ```

2. Hide the console window using:

   ```text
   ShowWindow(SW_HIDE)
   ```

3. Retrieve the current user account using:

   ```text
   GetUserNameA()
   ```

4. Retrieve basic system information using:

   ```text
   GetSystemInfo()
   ```

5. Collect system fields including:

   * OEM identifier
   * Processor count
   * Memory page size
   * Processor type
   * Minimum application address
   * Maximum application address
   * Active processor mask

6. Create or overwrite the consolidated output file:

   ```text
   C:\Users\Public\A04_3_System_Host_Recon_OK.txt
   ```

7. Write the report header and execution timestamp

8. Execute:

   ```text
   ipconfig /all
   ```

9. Append network configuration results to the report

10. Execute:

    ```text
    getmac
    ```

11. Append local MAC address information

12. Execute:

    ```text
    nslookup myip.opendns.com resolver1.opendns.com
    ```

13. Append public IP lookup results

14. Execute:

    ```text
    route print
    ```

15. Append IPv4 and IPv6 routing information

16. Execute:

    ```text
    net user
    ```

17. Append local user account information

18. Execute:

    ```text
    net localgroup
    ```

19. Append local security group information

20. Execute:

    ```text
    netstat -ano
    ```

21. Append active socket and network connection information

22. Execute:

    ```text
    tasklist /v
    ```

23. Append running process information

24. Query the 64-bit uninstall registry key using:

    ```text
    reg query
    ```

25. Append installed 64-bit application names

26. Query the 32-bit uninstall registry key using:

    ```text
    reg query
    ```

27. Append installed 32-bit application names

28. Execute:

    ```text
    wmic qfe get HotFixID,Description,InstalledOn
    ```

29. Append installed patch and hotfix information

30. Close the consolidated reconnaissance report

31. Exit after all discovery routines complete
