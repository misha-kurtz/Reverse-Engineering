# A06.5 System Startup RDP Foothold Persistence

## Summary

Creates persistent execution on the Windows analysis VM by copying
itself into the system-wide Startup folder and, upon subsequent
startup-triggered execution, configuring a controlled Remote Desktop
Protocol (RDP) foothold.

The specimen demonstrates a persistence-and-access foothold pattern
commonly associated with backdoors that attempt to maintain access
across reboots by combining startup-folder persistence with local
account creation, Remote Desktop enablement, login-screen account
concealment, and firewall rule modification.

Unlike scheduled-task or WMI-based persistence techniques, this
sample leverages the All Users Startup folder:

```text
C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup
```

The specimen uses a two-phase execution model:

* Phase 1 copies the executable into the system-wide Startup folder
* Phase 2 executes from Startup and configures the RDP foothold

The persisted executable is staged as:

```text
A06_5_RDP_backdoor.exe
```

The persistence and foothold behavior is intentionally controlled
for malware-analysis laboratory use.

---

## Payload Summary

The payload behavior is the execution of:

```text
A06_5_RDP_backdoor.exe
```

from the system-wide Startup folder after reboot or user logon.

When executed from its original location, the specimen:

* Resolves its current executable path
* Resolves the All Users Startup folder path
* Copies itself into the system-wide Startup folder
* Exits after staging the persisted copy

When executed from the Startup folder, the specimen:

* Creates or verifies a local user account
* Adds the account to the local Administrators group
* Hides the account from the Windows Welcome/Login UI
* Enables Remote Desktop connections
* Disables Network Level Authentication for RDP
* Enables the Windows Firewall Remote Desktop rule group

The controlled account configured by the sample is:

```text
Username: backdoor
Password: P@ssw0rd123!
```

The persisted executable path is expected to be:

```text
C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup\A06_5_RDP_backdoor.exe
```

No payloads are downloaded, injected, or unpacked.

---

## To Execute A06_5_SystemStartup_RDP_Foothold

### Stage the Startup Persistence Copy

Run from an elevated PowerShell prompt on the Windows analysis VM:

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_5\bin\A06_5_SystemStartup_RDP_Foothold.exe
```

During the first execution, the specimen copies itself to:

```text
C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup\A06_5_RDP_backdoor.exe
```

---

### Trigger Startup Execution

Reboot the Windows analysis VM or log off and log back in.

When the copied executable runs from the Startup folder, it executes
the RDP foothold configuration logic.

---

### Verify Startup Persistence

Check the All Users Startup folder:

```powershell
dir "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup"
```

Expected persisted file:

```text
A06_5_RDP_backdoor.exe
```

---

### Verify Local Account Creation

```powershell
Get-LocalUser backdoor
```

Verify local group membership:

```powershell
net localgroup Administrators
```

Expected account:

```text
backdoor
```

---

### Verify Hidden Login Account Registry Artifact

Inspect:

```powershell
reg query "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\SpecialAccounts\UserList"
```

Expected value:

```text
backdoor    REG_DWORD    0x0
```

---

### Verify Remote Desktop Configuration

Check whether Remote Desktop connections are enabled:

```powershell
reg query "HKLM\SYSTEM\CurrentControlSet\Control\Terminal Server" /v fDenyTSConnections
```

Expected value:

```text
fDenyTSConnections    REG_DWORD    0x0
```

Check Network Level Authentication configuration:

```powershell
reg query "HKLM\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp" /v UserAuthentication
```

Expected value:

```text
UserAuthentication    REG_DWORD    0x0
```

---

### Verify Firewall Rule Group

```powershell
netsh advfirewall firewall show rule group="Remote Desktop"
```

The Remote Desktop firewall rule group should be enabled.

---

## Expected Persistence Artifacts

### Startup Folder Persistence

```text
C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup\A06_5_RDP_backdoor.exe
```

### Local User Account

```text
Username: backdoor
Privilege: Local user
Group Membership: Administrators
Password Expiration: Disabled
```

### Hidden Account Registry Location

```text
HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\SpecialAccounts\UserList
```

### Hidden Account Registry Value

```text
backdoor = 0
```

### Remote Desktop Enablement

```text
HKLM\SYSTEM\CurrentControlSet\Control\Terminal Server\fDenyTSConnections = 0
```

### NLA Configuration

```text
HKLM\SYSTEM\CurrentControlSet\Control\Terminal Server\WinStations\RDP-Tcp\UserAuthentication = 0
```

### Firewall Rule Group

```text
Remote Desktop
```

---

#########################################################################

## High-Level System Startup RDP Foothold Persistence Flow

1. Resolve current executable path using:

   ```text
   GetModuleFileNameW()
   ```

2. Resolve All Users Startup folder using:

   ```text
   SHGetFolderPathW()
   CSIDL_COMMON_STARTUP
   ```

3. Determine whether execution is occurring from the Startup folder

4. If not running from Startup, copy the executable to:

   ```text
   C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup\A06_5_RDP_backdoor.exe
   ```

5. Initial staging execution completes

6. System reboots or user logs on

7. Windows launches executable from the system-wide Startup folder

8. Specimen detects Startup-folder execution context

9. Create or verify local user account:

   ```text
   backdoor
   ```

10. Configure account password to never expire

11. Add account to local Administrators group

12. Create or modify hidden account registry path:

```text
HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\SpecialAccounts\UserList
```

13. Set hidden account value:

```text
backdoor = 0
```

14. Enable Remote Desktop by setting:

```text
fDenyTSConnections = 0
```

15. Disable Network Level Authentication by setting:

```text
UserAuthentication = 0
```

16. Initialize COM runtime

17. Instantiate Windows Firewall policy object:

```text
INetFwPolicy2
```

18. Enable firewall rule group:

```text
Remote Desktop
```

19. RDP foothold configuration completes

20. Persistence survives:

* Reboots
* User logoff
* User logon
* Startup folder processing

21. Foothold remains until the startup artifact, local account,
    registry changes, and firewall/RDP settings are removed

