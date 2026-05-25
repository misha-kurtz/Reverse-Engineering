# A06_1a Registry Run-Key Persistence

## Summary

Creates user-level persistence on the Windows analysis VM by
writing a registry Run-key entry under the current user hive at:

```text
HKCU\Software\Microsoft\Windows\CurrentVersion\Run
```

The sample stores a command that launches a controlled marker executable:

```text
C:\Users\Public\A06_1a_persistence_marker.exe
```

The Run-key value name is dynamically derived from the system
MachineGuid value located in:

```text
HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid
```

The GUID is normalized by removing braces, spaces, and hyphens
before being used as the persistence value name. This produces a
host-specific persistence artifact that appears less obviously tied
to the sample itself.

This specimen demonstrates classic Windows registry persistence
commonly associated with backdoors, loaders, remote access trojans,
and long-term foothold mechanisms. The sample establishes automatic
execution during user logon by leveraging native Windows registry APIs.

The persistence payload itself is intentionally benign and is used
only to generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of the marker executable
`A06_1a_persistence_marker.exe` after user logon or system reboot.

When triggered, the marker executable:

* Waits briefly to simulate realistic delayed persistence execution
* Determines its own process path
* Enumerates its parent process identifier (PPID)
* Captures execution timestamp information
* Writes a confirmation artifact to:

```text
C:\Users\Public\A06_1a_RunKeyPersistence_OK.txt
```

The artifact contains:

* Timestamp
* Process ID
* Parent Process ID
* Executable path
* Persistence type
* Trigger condition
* Confirmation status

The marker additionally emits a debug string using:

```text
OutputDebugStringA()
```

No additional payloads are downloaded, injected, staged, or executed.

---

## To Execute A06_1a_registry_run_persist

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_1a\bin\A06_1a_registry_run_persist.exe
```

---

## Expected Persistence Artifact

### Registry Location

```text
HKCU\Software\Microsoft\Windows\CurrentVersion\Run
```

### Example Value

```text
Name: 1234567890abcdef1234567890abcdef
Type: REG_SZ
Data: "C:\Users\Public\A06_1a_persistence_marker.exe"
```

---

#########################################################################

# High-Level Registry Run-Key Persistence Flow

1. Retrieve the system MachineGuid from:

   ```text
   HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid
   ```

2. Normalize the GUID string by removing:

   * Hyphens
   * Braces
   * Spaces

3. Configure the marker executable path:

   ```text
   C:\Users\Public\A06_1a_persistence_marker.exe
   ```

4. Open the current-user Run-key registry path:

   ```text
   HKCU\Software\Microsoft\Windows\CurrentVersion\Run
   ```

5. Create the Run-key path if it does not already exist

6. Write a REG_SZ persistence value containing the marker executable path

7. Close registry handles and terminate the installer process

8. Wait for a user logon or reboot event to trigger persistence execution

9. Launch the marker executable automatically through the Windows logon process

10. Delay execution briefly to simulate realistic persistence timing

11. Collect execution metadata:

    * PID
    * Parent PID
    * Timestamp
    * Process path

12. Write persistence confirmation artifact to:

    ```text
    C:\Users\Public\A06_1a_RunKeyPersistence_OK.txt
    ```

13. Emit a debug confirmation string using:

    ```text
    OutputDebugStringA()
    ```

14. Exit cleanly after artifact generation
