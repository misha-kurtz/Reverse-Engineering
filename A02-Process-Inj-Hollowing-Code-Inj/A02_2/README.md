# A02_2 Remote Thread Injection

## Summary

Injects raw shellcode into a separate running process using
remote memory allocation, memory protection modification,
and remote thread creation.

The sample demonstrates process injection behavior commonly associated
with malware loaders, shellcode launchers, post-exploitation tooling,
remote-access trojans, offensive frameworks, and in-memory execution
techniques used to avoid dropping executable payloads to disk.

Unlike DLL injection workflows that rely on:

```text id="vxqdr4"
LoadLibraryA()
```

this specimen directly injects executable shellcode into the
remote process memory space and begins execution through a
newly created remote thread.

The injection workflow performs the following operations:

* Enumerates running processes
* Locates a target process by executable name
* Opens the target process with full access rights
* Allocates writable memory inside the remote process
* Writes shellcode into remote process memory
* Changes memory permissions from:

```text id="1nq7ow"
PAGE_READWRITE
```

to:

```text id="e8m9iw"
PAGE_EXECUTE_READ
```

* Creates a remote thread beginning execution at the injected
  shellcode entry point

The injected payload is a benign proof-of-execution shellcode
generated using:

```text id="y3svt9"
msfvenom -p windows/x64/exec CMD=calc.exe EXITFUNC=thread -f c
```

The payload simply launches:

```text id="gb5g0x"
calc.exe
```

for controlled validation during dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of injected shellcode
inside the remote target process.

The shellcode:

* Executes entirely from remote process memory
* Does not require a DLL on disk
* Resolves required Windows APIs dynamically
* Launches:

```text id="9bg4c0"
calc.exe
```

* Exits cleanly using:

```text id="dbmq0u"
EXITFUNC=thread
```

No persistence, credential theft, network communication,
or destructive functionality is performed.

The payload exists solely to generate observable forensic
artifacts associated with remote shellcode execution.

---

## To Execute A02_2

Launch a target process first, such as:

```powershell id="g7kkmx"
notepad.exe
```

Then execute the injector:

```powershell id="fnyd9k"
.\A02-Process-Inj-Hollowing-Code-Inj\A02_2\bin\A02_2_remote_thread_inject.exe notepad.exe
```

---

## Expected Injection Behavior

### Remote Memory Operations

The injector performs:

```text id="g5kg5o"
VirtualAllocEx()
WriteProcessMemory()
VirtualProtectEx()
CreateRemoteThread()
```

against the target process.

### Expected Result

Successful execution causes the remote target process to
launch:

```text id="6dy4wx"
calc.exe
```

through execution of injected shellcode.

---

#########################################################################

# High-Level Remote Thread Injection Flow

1. Launch a target process such as:

   ```text
   notepad.exe
   ```

2. Enumerate running processes using:

   ```text
   CreateToolhelp32Snapshot()
   ```

3. Locate the target process identifier (PID) using:

   ```text
   Process32First()
   Process32Next()
   ```

4. Open the target process using:

   ```text
   OpenProcess(PROCESS_ALL_ACCESS)
   ```

5. Allocate writable memory inside the remote process using:

   ```text
   VirtualAllocEx()
   ```

6. Write shellcode into remote process memory using:

   ```text
   WriteProcessMemory()
   ```

7. Change remote memory permissions using:

   ```text
   VirtualProtectEx()
   ```

8. Transition memory permissions from:

   ```text
   PAGE_READWRITE
   ```

   to:

   ```text
   PAGE_EXECUTE_READ
   ```

9. Create a remote execution thread using:

   ```text
   CreateRemoteThread()
   ```

10. Begin execution at the injected shellcode address

11. Dynamically resolve required Windows APIs from memory

12. Launch:

    ```text
    calc.exe
    ```

13. Exit the remote shellcode thread cleanly

14. Leave observable forensic artifacts associated with:

    * Cross-process memory allocation
    * Remote memory writing
    * Executable memory creation
    * Remote thread execution
    * Child process creation from injected execution context
