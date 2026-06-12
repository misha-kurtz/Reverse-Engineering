# A02.1 DLL Injection via LoadLibrary Remote Thread

## Summary

Injects a benign marker DLL into a separate running process using
a classic Windows DLL injection workflow based on remote memory
allocation and remote thread creation.

The sample demonstrates process injection behavior commonly associated
with malware loaders, credential theft tooling, remote-access
trojans, post-exploitation frameworks, and modular malware families
that dynamically load payloads into external processes.

The injection workflow performs the following operations:

* Locates a target process window
* Retrieves the target process identifier (PID)
* Opens the target process with full access rights
* Allocates memory inside the remote process
* Writes a DLL path into remote process memory
* Creates a remote thread that calls:

```text
LoadLibraryA()
```

The target process then loads the supplied DLL into its address
space and executes the DLL initialization routine.

The injected DLL payload itself is intentionally benign and exists
only to generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of the injected marker DLL
`A02_1_marker.dll` inside the remote target process.

When injected, the DLL:

* Executes from `DllMain`
* Spawns a worker thread to avoid loader-lock instability
* Determines the target process path
* Captures execution timestamp information
* Retrieves the remote process identifier (PID)
* Writes a confirmation artifact to:

```text
C:\Users\Public\A02_1_Injected_OK.txt
```

The artifact contains:

* Timestamp
* Process ID
* Remote process path
* Injection confirmation string

The DLL additionally emits a debug string using:

```text
OutputDebugStringA()
```

No persistence, network communication, credential theft,
or destructive functionality is performed.

---

## To Execute A02_1

### Step 1 — Launch the target process

```powershell
.\A02-Process-Inj-Hollowing-Code-Inj\A02_1\bin\A02_1_target_process.exe
```

### Step 2 — Launch the injector

```powershell
.\A02-Process-Inj-Hollowing-Code-Inj\A02_1\bin\A02_1_dll_injector.exe
```

---

## Expected Injection Artifact

### Output File

```text
C:\Users\Public\A02_1_Injected_OK.txt
```

### Example Contents

```text
THESIS_A02_1_DLL_LOADED
Timestamp: 2026-05-25 14:10:33
PID: 1234
ProcessPath: C:\Path\To\A02_1_target_process.exe
```

---

#########################################################################

# High-Level DLL Injection Flow

1. Launch the controlled target process:

   ```text
   A02_1_target_process.exe
   ```

2. Locate the target process window using:

   ```text
   FindWindowA()
   ```

3. Retrieve the target process identifier (PID) using:

   ```text
   GetWindowThreadProcessId()
   ```

4. Open the target process using:

   ```text
   OpenProcess(PROCESS_ALL_ACCESS)
   ```

5. Allocate writable memory inside the remote process using:

   ```text
   VirtualAllocEx()
   ```

6. Write the DLL path into remote process memory using:

   ```text
   WriteProcessMemory()
   ```

7. Resolve the address of:

   ```text
   LoadLibraryA()
   ```

8. Create a remote thread inside the target process using:

   ```text
   CreateRemoteThread()
   ```

9. Execute the remote loader thread which loads:

   ```text
   A02_1_marker.dll
   ```

10. Execute the DLL initialization routine inside the target process

11. Spawn a worker thread from `DllMain` to avoid loader-lock issues

12. Collect execution metadata:

    * PID
    * Timestamp
    * Remote process path

13. Write injection confirmation artifact to:

    ```text
    C:\Users\Public\A02_1_Injected_OK.txt
    ```

14. Emit a debug confirmation string using:

    ```text
    OutputDebugStringA()
    ```

15. Exit cleanly after artifact generation
