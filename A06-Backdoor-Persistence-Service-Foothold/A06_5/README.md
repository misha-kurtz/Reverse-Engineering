# A06_5 System Startup RDP Foothold

## Summary

Establishes persistent remote access capability on the Windows analysis
VM by combining startup-folder persistence with automated Remote Desktop
Protocol (RDP) configuration.

The specimen demonstrates a foothold-establishment persistence mechanism
commonly associated with remote access trojans (RATs), post-exploitation
implants, administrative persistence tooling, and long-term access
operations where an attacker attempts to maintain interactive system
access after an initial compromise.

The sample operates in two distinct phases:

1. Installation into the system-wide startup folder
2. Automated remote access configuration during subsequent execution

The startup persistence location used by the specimen is:

```text
C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup
```

By placing a copy of itself into the global startup directory, the
sample ensures execution whenever a user logs onto the system.

When executed from the persistence location, the specimen performs
multiple system modifications that collectively establish a durable
remote-access foothold.

These actions include:

* Local account creation
* Administrative group membership assignment
* User-account concealment from the Windows logon UI
* Remote Desktop enablement
* Network Level Authentication (NLA) disabling
* Firewall rule activation for Remote Desktop access

No payload downloading, code injection, process hollowing, shellcode
execution, network beaconing, or command-and-control functionality is
performed.

The sample exists solely to generate controlled persistence and system
configuration artifacts for malware analysis and reverse-engineering
research.

---

## Persistence Summary

### Initial Installation Phase

When executed outside the system startup directory, the specimen copies
itself into:

```text
C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup\A06_5_RDP_Foothold.exe
```

This establishes startup-folder persistence for all users of the system.

The initial installer execution then exits.

---

## To Execute A06_5_SystemStartup_RDP_Foothold

### Initial Installation

Execute the specimen with administrative privileges:

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_5\bin\A06_5_SystemStartup_RDP_Foothold.exe
```

The executable should copy itself into:

```text
C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup
```

---

### Verify Startup Persistence

```powershell
Get-ChildItem "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup"
```

Expected output should include:

```text
A06_5_RDP_Foothold.exe
```

---

## Expected Persistence Artifacts

### Startup Persistence

```text
C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup\A06_5_RDP_Foothold.exe
```

---

#########################################################################

# High-Level System Startup RDP Foothold Flow

1. Initialize execution

2. Determine current executable location

3. Resolve the system-wide startup directory:

   ```text
   C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup
   ```

4. Determine whether execution originated from the startup folder

5. If not executing from the startup folder:

   * Copy executable into the startup directory
   * Exit installer process

6. Wait for user logon or system reboot

7. Automatically launch from startup persistence location

8. Create local account:

   ```text
   backdoor
   ```

9. Configure password attributes

10. Add account to:

    ```text
    Administrators
    ```

11. Create hidden-account registry configuration:

    ```text
    SpecialAccounts\UserList
    ```

12. Conceal account from standard Windows logon interfaces

13. Enable Remote Desktop access

14. Configure:

    ```text
    fDenyTSConnections = 0
    ```

15. Disable Network Level Authentication

16. Configure:

    ```text
    UserAuthentication = 0
    ```

17. Initialize COM firewall management interfaces

18. Enable:

    ```text
    Remote Desktop
    ```

    firewall rule group

19. Establish a persistent remote-access foothold across:

    * User logoff/logon cycles
    * System reboots
    * Administrative sessions

20. Exit cleanly after configuration is complete
