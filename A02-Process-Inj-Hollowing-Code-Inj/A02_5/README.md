# A02_5 Process Hollowing

## Summary

Executes a benign replacement executable inside the address space
of a newly created suspended process using a process hollowing
workflow.

The sample demonstrates process hollowing behavior commonly associated
with malware loaders, evasive execution, masquerading, process
replacement, and payload staging techniques where a legitimate-looking
process is created and then replaced with attacker-controlled code.

Unlike remote thread injection samples that inject shellcode or DLLs
into an already-running process, this specimen creates a new process
in a suspended state, replaces or maps executable content into that
process, adjusts execution context, and resumes the process so the
replacement executable begins running.

The hollowed payload is a benign marker executable:

```text
A02_5_loaded_exe.exe
```

When executed successfully through the hollowed process, it writes
a controlled confirmation artifact to disk.

The payload itself is intentionally benign and exists only to
generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of `A02_5_loaded_exe.exe`
after successful process hollowing.

When executed, the loaded executable:

* Determines its own process path
* Retrieves its process identifier
* Captures execution timestamp information
* Writes a confirmation artifact to:

```text
C:\Users\Public\A02_5_Process_Hollowing_OK.txt
```

The artifact contains:

* Timestamp
* Process ID
* Process path
* Process hollowing confirmation string

The payload additionally emits a debug string using:

```text
OutputDebugStringA()
```

No persistence, credential theft, network communication,
or destructive functionality is performed.

---

## To Execute A02_5

Run the process hollowing sample directly:

```powershell
.\A02-Process-Inj-Hollowing-Code-Inj\A02_5\bin\A02_5_process_hollowing.exe
```

Make sure the replacement payload executable is present where the
hollowing sample expects it:

```text
A02_5_loaded_exe.exe
```

---

## Expected Injection Artifact

### Output File

```text
C:\Users\Public\A02_5_Process_Hollowing_OK.txt
```

### Example Contents

```text
THESIS_A02_5_EXE_LOADED_VIA_PROCESS_HOLLOWING
Timestamp: 2026-05-25 14:10:33
PID: 1234
ProcessPath: C:\Path\To\Hollowed\Process.exe
```

---

#########################################################################

# High-Level Process Hollowing Flow

1. Start the process hollowing loader:

   ```text
   A02_5_process_hollowing.exe
   ```

2. Create a new target process in a suspended state using:

   ```text
   CreateProcess(..., CREATE_SUSPENDED, ...)
   ```

3. Load or read the replacement executable:

   ```text
   A02_5_loaded_exe.exe
   ```

4. Parse the replacement executable headers

5. Identify the preferred image base, entry point, section table,
   and image size

6. Obtain the suspended process context using:

   ```text
   GetThreadContext()
   ```

7. Locate the target process image base from the suspended process
   environment

8. Unmap or replace the original image where applicable using an
   image-unmapping workflow

9. Allocate memory inside the suspended process for the replacement
   image using:

   ```text
   VirtualAllocEx()
   ```

10. Write the replacement executable headers into the remote process
    using:

    ```text
    WriteProcessMemory()
    ```

11. Write each replacement executable section into the remote process

12. Update the remote process image base or context metadata as needed

13. Redirect the suspended thread context to the replacement executable
    entry point using:

    ```text
    SetThreadContext()
    ```

14. Resume the suspended process thread using:

    ```text
    ResumeThread()
    ```

15. Begin execution of the replacement payload inside the hollowed
    process

16. Execute:

    ```text
    A02_5_loaded_exe.exe
    ```

17. Collect execution metadata:

    * PID
    * Timestamp
    * Process path

18. Write process hollowing confirmation artifact to:

    ```text
    C:\Users\Public\A02_5_Process_Hollowing_OK.txt
    ```

19. Emit a debug confirmation string using:

    ```text
    OutputDebugStringA()
    ```

20. Exit cleanly after artifact generation
