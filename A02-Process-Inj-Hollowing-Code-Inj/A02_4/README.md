# A02_4 Asynchronous Procedure Call Injection

## Summary

Loads a benign marker DLL into a separate running process using
Asynchronous Procedure Call, or APC, injection.

The sample demonstrates process injection behavior commonly associated
with malware loaders, stealthier code execution techniques,
post-exploitation tooling, and execution methods that rely on
existing thread behavior rather than immediately creating a new
remote execution thread.

Unlike `A02_1`, which uses:

```text
CreateRemoteThread()
```

to directly call:

```text
LoadLibraryA()
```

this specimen queues an APC routine to a thread inside the target
process. The target process contains a controlled alertable thread
that repeatedly enters an alertable wait state using:

```text
SleepEx(1000, TRUE)
```

This allows the queued user-mode APC to execute when the thread
becomes alertable.

The injection workflow performs the following operations:

* Locates the target process
* Opens the target process
* Allocates memory inside the remote process
* Writes the DLL path into remote memory
* Locates or opens a target thread
* Queues an APC routine to the target thread
* Executes the APC when the thread enters an alertable state
* Loads the marker DLL through:

```text
LoadLibraryA()
```

The injected DLL payload itself is intentionally benign and exists
only to generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of the injected marker DLL
`A02_4_marker.dll` inside the APC target process.

When injected, the DLL:

* Executes from `DllMain`
* Spawns a worker thread to avoid loader-lock instability
* Determines the target process path
* Captures execution timestamp information
* Retrieves the target process identifier
* Writes a confirmation artifact to:

```text
C:\Users\Public\A02_4_APC_Injection_OK.txt
```

The artifact contains:

* Timestamp
* Process ID
* Remote process path
* APC injection confirmation string

The DLL additionally emits a debug string using:

```text
OutputDebugStringA()
```

No persistence, credential theft, network communication,
or destructive functionality is performed.

---

## To Execute A02_4

### Step 1 — Launch the APC target process

```powershell
.\A02-Process-Inj-Hollowing-Code-Inj\A02_4\bin\A02_4_target_process.exe
```

The target process creates an internal alertable thread that waits
using:

```text
SleepEx(1000, TRUE)
```

### Step 2 — Launch the APC injector

```powershell
.\A02-Process-Inj-Hollowing-Code-Inj\A02_4\bin\A02_4_APC_injector.exe
```

---

## Expected Injection Artifact

### Output File

```text
C:\Users\Public\A02_4_APC_Injection_OK.txt
```

### Example Contents

```text
THESIS_A02_4_DLL_LOADED_VIA_APC_INJECTION
Timestamp: 2026-05-25 14:10:33
PID: 1234
ProcessPath: C:\Path\To\A02_4_target_process.exe
```

---

#########################################################################

# High-Level APC Injection Flow

1. Launch the controlled APC target process:

   ```text
   A02_4_target_process.exe
   ```

2. Start the target process alertable worker thread using:

   ```text
   CreateThread()
   ```

3. Place the worker thread into an alertable wait loop using:

   ```text
   SleepEx(1000, TRUE)
   ```

4. Launch the APC injector:

   ```text
   A02_4_APC_injector.exe
   ```

5. Locate the target process associated with:

   ```text
   A02_4_apc_target_process
   ```

6. Open the target process using a process handle

7. Allocate writable memory inside the remote process using:

   ```text
   VirtualAllocEx()
   ```

8. Write the marker DLL path into remote process memory using:

   ```text
   WriteProcessMemory()
   ```

9. Locate or open a thread belonging to the target process

10. Resolve the address of:

    ```text
    LoadLibraryA()
    ```

11. Queue a user-mode APC to the target thread using:

    ```text
    QueueUserAPC()
    ```

12. Wait for the target thread to enter an alertable wait state

13. Execute the queued APC routine inside the target process

14. Load the marker DLL:

    ```text
    A02_4_marker.dll
    ```

15. Execute the DLL initialization routine inside the target process

16. Spawn a worker thread from `DllMain` to avoid loader-lock issues

17. Collect execution metadata:

    * PID
    * Timestamp
    * Remote process path

18. Write APC injection confirmation artifact to:

    ```text
    C:\Users\Public\A02_4_APC_Injection_OK.txt
    ```

19. Emit a debug confirmation string using:

    ```text
    OutputDebugStringA()
    ```

20. Exit cleanly after artifact generation
