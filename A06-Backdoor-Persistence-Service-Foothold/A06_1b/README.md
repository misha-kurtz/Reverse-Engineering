# A06_1b Offline Registry Hive Save/Restore Persistence

## Summary

Creates user-level persistence on the Windows analysis VM by
modifying registry persistence artifacts through an offline-style
registry hive workflow rather than directly creating the final
Run-key value during normal execution flow.

The sample demonstrates persistence behavior commonly associated
with malware installers, recovery mechanisms, stealth persistence
utilities, and forensic-evasion workflows that manipulate registry
data indirectly through hive save/restore operations.

Rather than relying solely on a straightforward Run-key creation
sequence, the specimen emulates an offline registry modification
workflow in which registry configuration data is prepared, written,
or restored through intermediate hive operations before becoming
active within the Windows registry environment.

The persistence mechanism ultimately configures automatic execution
of the following marker executable during user logon:

```text
C:\Users\Public\A06_1b_persistence_marker.exe
```

This specimen demonstrates how attackers may establish persistence
through indirect registry manipulation workflows that can complicate
forensic attribution and reduce visibility compared to traditional
live registry editing alone.

The persistence payload itself is intentionally benign and exists
only to generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of the marker executable
`A06_1b_persistence_marker.exe` after the persistence configuration
is restored and activated during user logon or reboot.

When triggered, the marker executable:

* Waits briefly to simulate realistic delayed persistence execution
* Determines its own process path
* Enumerates its parent process identifier (PPID)
* Captures execution timestamp information
* Writes a confirmation artifact to:

```text
C:\Users\Public\A06_1b_OfflineHiveSaveRestore_Persistence_OK.txt
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

## To Execute A06_1b_offline_persistence

Make sure offreg.dll is placed in same directory as sample before running

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_1b\bin\A06_1b_offline_persistence.exe
```

---

## Expected Persistence Artifact

### Registry Location

```text
HKCU\Software\Microsoft\Windows\CurrentVersion\Run
```

### Example Value

```text
Name: OfflineHivePersistence
Type: REG_SZ
Data: "C:\Users\Public\A06_1b_persistence_marker.exe"
```

---

#########################################################################

# High-Level Offline Hive Save/Restore Persistence Flow

1. Initialize registry hive manipulation workflow

2. Prepare persistence configuration targeting:

   ```text
   HKCU\Software\Microsoft\Windows\CurrentVersion\Run
   ```

3. Configure marker executable path:

   ```text
   C:\Users\Public\A06_1b_persistence_marker.exe
   ```

4. Create or modify persistence registry data within an offline-style
   registry workflow

5. Save or stage modified registry hive data

6. Restore or apply the modified hive configuration back into the
   active registry environment

7. Activate persistence configuration for subsequent user logon

8. Close registry handles and terminate the installer process

9. Wait for user logon or reboot event

10. Automatically launch the marker executable through the restored
    Run-key persistence entry

11. Delay execution briefly to simulate realistic persistence timing

12. Collect execution metadata:

    * PID
    * Parent PID
    * Timestamp
    * Process path

13. Write persistence confirmation artifact to:

    ```text
    C:\Users\Public\A06_1b_OfflineHiveSaveRestore_Persistence_OK.txt
    ```

14. Emit a debug confirmation string using:

    ```text
    OutputDebugStringA()
    ```

15. Exit cleanly after artifact generation
