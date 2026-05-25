# A02_3 Thread Hijacking

## Summary

Executes raw shellcode by creating a suspended thread,
modifying its instruction pointer, and resuming execution
from attacker-controlled memory.

The sample demonstrates thread hijacking behavior commonly associated
with process injection, in-memory shellcode execution, malware loaders,
post-exploitation tooling, and execution redirection techniques.

Unlike `A02_2`, which creates a remote thread inside another process,
this specimen demonstrates the core mechanics of thread-context
manipulation in a controlled local process. The sample creates a
suspended thread, captures its CPU context, redirects its `RIP`
register to shellcode, and resumes the thread.

The hijacking workflow performs the following operations:

* Creates a suspended thread
* Allocates memory for shellcode
* Copies shellcode into the allocated buffer
* Changes memory permissions from:

```text
PAGE_READWRITE
```

to:

```text
PAGE_EXECUTE_READ
```

* Captures the suspended thread context
* Modifies the thread instruction pointer:

```text
RIP
```

* Applies the modified context
* Resumes the hijacked thread

The injected shellcode is generated from the included `calc.asm`
source and launches:

```text
calc.exe
```

as a benign proof-of-execution artifact.

---

## Payload Summary

The payload behavior is the execution of custom x64 shellcode
through a hijacked thread context.

The shellcode:

* Executes from locally allocated process memory
* Walks the Process Environment Block / loader structures
* Resolves the base address of `kernel32.dll`
* Parses the export table to locate:

```text
WinExec
```

* Pushes the command string:

```text
calc.exe
```

onto the stack

* Calls `WinExec` to launch Calculator
* Returns cleanly after execution

No persistence, credential theft, network communication,
or destructive functionality is performed.

The payload exists solely to generate observable forensic artifacts
associated with thread-context manipulation and in-memory execution.

---

## To Execute A02_3

Run the thread hijacking sample directly:

```powershell
.\A02-Process-Inj-Hollowing-Code-Inj\A02_3\bin\A02_3_thread_hijack.exe
```

No external target process is required because this sample creates
and hijacks its own suspended thread.

---

## Expected Injection Behavior

### Thread Context Operations

The sample performs:

```text
CreateThread(CREATE_SUSPENDED)
VirtualAlloc()
RtlCopyMemory()
VirtualProtect()
GetThreadContext()
SetThreadContext()
ResumeThread()
```

### Expected Result

Successful execution causes the hijacked thread to execute shellcode
and launch:

```text
calc.exe
```

---

#########################################################################

# High-Level Thread Hijacking Flow

1. Start the local thread hijacking sample:

   ```text
   A02_3_thread_hijack.exe
   ```

2. Create a suspended thread using:

   ```text
   CreateThread(..., CREATE_SUSPENDED, ...)
   ```

3. Set the original suspended thread start routine to:

   ```text
   DummyFunction()
   ```

4. Allocate local process memory using:

   ```text
   VirtualAlloc()
   ```

5. Copy shellcode into the allocated memory region using:

   ```text
   RtlCopyMemory()
   ```

6. Change memory permissions using:

   ```text
   VirtualProtect()
   ```

7. Transition memory permissions from:

   ```text
   PAGE_READWRITE
   ```

   to:

   ```text
   PAGE_EXECUTE_READ
   ```

8. Retrieve the suspended thread context using:

   ```text
   GetThreadContext()
   ```

9. Read the original instruction pointer value from:

   ```text
   CTX.Rip
   ```

10. Redirect the instruction pointer to the shellcode buffer:

    ```text
    CTX.Rip = Buffer
    ```

11. Apply the modified thread context using:

    ```text
    SetThreadContext()
    ```

12. Resume the suspended thread using:

    ```text
    ResumeThread()
    ```

13. Begin execution at the injected shellcode address

14. Walk the PEB to locate loaded module information

15. Resolve the base address of:

    ```text
    kernel32.dll
    ```

16. Parse the export table to locate:

    ```text
    WinExec
    ```

17. Execute:

    ```text
    calc.exe
    ```

18. Wait for the hijacked thread to finish execution

19. Free the allocated shellcode buffer using:

    ```text
    VirtualFree()
    ```

20. Close the thread handle and exit cleanly
