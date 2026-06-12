# A03.3 In-Memory Shellcode Loader

## Summary

Executes an embedded shellcode payload entirely from memory without
writing a secondary executable payload to disk.

The sample demonstrates in-memory loader behavior commonly associated
with fileless malware, shellcode staging, runtime memory protection
changes, and local payload execution workflows where executable bytes
are staged directly inside the current process address space.

Unlike disk-based droppers or network downloaders that write payloads
to the filesystem before execution, this specimen performs all payload
staging directly in memory using dynamically allocated executable pages.

The embedded payload is a benign x64 shellcode routine derived from:

```text
calc.asm
```

The shellcode exists solely to generate controlled execution artifacts
for dynamic malware-analysis workflows.

When executed successfully, the shellcode launches:

```text
calc.exe
```

No persistence or destructive behavior is performed.

---

## Payload Summary

The payload behavior is the in-memory execution of precompiled x64
shellcode embedded directly inside the loader executable.

The shellcode originates from:

```text
calc.asm
```

The assembly payload demonstrates low-level shellcode behavior
commonly associated with staged memory loaders and offensive tooling.

When executed, the loader:

* Stores the raw shellcode bytes internally
* Allocates writable memory using `VirtualAlloc()`
* Copies the shellcode into the allocated region
* Changes memory permissions to executable using `VirtualProtect()`
* Executes the payload inside a new thread using `CreateThread()`

The shellcode itself:

* Accesses the Process Environment Block (PEB)
* Locates `kernel32.dll` dynamically
* Parses the PE export table manually
* Resolves the address of:

```text
WinExec()
```

* Builds the string:

```text
calc.exe
```

* Executes:

```text
WinExec("calc.exe", SW_SHOWNORMAL)
```

The payload performs no:

* Persistence
* Network communication
* File dropping
* Credential theft
* Privilege escalation
* Destructive activity

---

## Assembly Payload Source

The embedded shellcode was precompiled from:

```text
calc.asm
```

The assembly source demonstrates several common shellcode-development
techniques including:

* PEB traversal
* Export table parsing
* Dynamic API resolution
* Position-independent execution
* Direct WinAPI invocation without imports

The shellcode avoids static imports for `WinExec()` and instead
resolves the API dynamically at runtime.

---

## To Execute A03_3

Run the in-memory shellcode loader directly:

```powershell
.\A03-Dropper-Loader-Stager\A03_3\bin\A03_3_in_mem_shellcode_loader.exe
```

Successful execution should result in:

```text
calc.exe
```

launching from a dynamically allocated executable memory region.

---

## Expected Loader Artifacts

### Child Process

```text
calc.exe
```

### Expected Console Output

```text
Allocated <size> bytes at <address> and copied embedded buffer.
Changed memory protection to PAGE_EXECUTE_READ.
Thread created. Waiting for shellcode to finish...
```

### Expected Memory Operations

The loader should exhibit the following memory staging behavior:

```text
VirtualAlloc()
memcpy()
VirtualProtect()
CreateThread()
WaitForSingleObject()
VirtualFree()
```

### Expected Dynamic Signals

Dynamic analysis tools may observe:

* RW → RX memory permission transition
* Thread execution beginning inside dynamically allocated memory
* Memory-backed execution flow
* Local process spawning of `calc.exe`
* Absence of dropped payload executables

---

#########################################################################

# High-Level In-Memory Shellcode Loader Flow

1. Start the in-memory shellcode loader:

   ```text
   A03_3_in_mem_shellcode_loader.exe
   ```

2. Load the embedded shellcode byte array compiled from:

   ```text
   calc.asm
   ```

3. Determine the shellcode size using:

   ```text
   sizeof(payload)
   ```

4. Allocate writable memory for payload staging using:

   ```text
   VirtualAlloc(..., PAGE_READWRITE)
   ```

5. Copy the shellcode bytes into the allocated memory region using:

   ```text
   memcpy()
   ```

6. Change the memory protection from writable to executable using:

   ```text
   VirtualProtect(..., PAGE_EXECUTE_READ, ...)
   ```

7. Create a new thread beginning at the staged shellcode buffer using:

   ```text
   CreateThread()
   ```

8. Begin shellcode execution from dynamically allocated memory

9. Access the Process Environment Block (PEB) through the `GS` segment

10. Walk loaded module structures to locate:

    ```text
    kernel32.dll
    ```

11. Parse the PE export directory manually

12. Search export names for:

    ```text
    WinExec
    ```

13. Resolve the runtime address of:

    ```text
    WinExec()
    ```

14. Construct the command string:

    ```text
    calc.exe
    ```

15. Invoke:

    ```text
    WinExec("calc.exe", SW_SHOWNORMAL)
    ```

16. Launch:

    ```text
    calc.exe
    ```

17. Wait for the shellcode thread using:

    ```text
    WaitForSingleObject()
    ```

18. Close the thread handle

19. Release the staged executable memory using:

    ```text
    VirtualFree()
    ```

20. Exit cleanly after in-memory payload execution
