# A03_4 Obfuscated Loader

## Summary

Decodes and executes an obfuscated shellcode payload entirely from
memory.

The sample demonstrates obfuscated loader behavior commonly associated
with payload concealment, runtime decoding, staged shellcode execution,
and fileless malware workflows where the executable payload is not
stored in plaintext inside the loader.

Unlike `A03_3`, where the raw shellcode bytes are embedded directly,
this specimen stores the payload as a Base64-encoded blob whose decoded
bytes are XOR-obfuscated with:

```text
0x5A
```

At runtime, the loader decodes the blob, reverses the XOR operation,
allocates memory, changes permissions, and executes the recovered
shellcode in a new thread.

The embedded payload is a benign x64 shellcode routine derived from:

```text
calc.asm
```

When executed successfully, the shellcode launches:

```text
calc.exe
```

---

## Payload Summary

The payload behavior is the runtime deobfuscation and in-memory
execution of x64 shellcode.

When executed, the loader:

* Stores the payload as a Base64 string
* Decodes the Base64 blob into bytes
* XOR-decodes each byte using `0x5A`
* Allocates writable memory using `VirtualAlloc()`
* Copies the decoded shellcode into memory using `Marshal.Copy()`
* Changes memory permissions using `VirtualProtect()`
* Starts a thread at the decoded shellcode address using `CreateThread()`

The shellcode itself:

* Accesses the Process Environment Block
* Locates `kernel32.dll`
* Parses the PE export table
* Resolves:

```text
WinExec()
```

* Executes:

```text
calc.exe
```

No persistence, credential theft, network communication,
file dropping, privilege escalation, or destructive functionality is
performed.

---

## Assembly Payload Source

The obfuscated shellcode was generated from:

```text
calc.asm
```

The assembly payload demonstrates:

* PEB traversal
* Manual export table parsing
* Dynamic API resolution
* Position-independent shellcode execution
* Direct invocation of `WinExec()` without a static import

The loader stores the compiled shellcode in an obfuscated form rather
than embedding the raw byte array directly.

---

## To Execute A03_4

Run the obfuscated loader directly:

```powershell
.\A03-Dropper-Loader-Stager\A03_4\bin\A03_4_obfuscated_loader.exe
```

Successful execution should result in:

```text
calc.exe
```

launching from the decoded shellcode thread.

---

## Expected Loader Artifacts

### Child Process

```text
calc.exe
```

### Expected Console Output

```text
[*] Target Architecture: x64
[*] Payload Size: 138 bytes
[*] Launching thread at 0x<address>...
[*] Successfully started. If no calc appears, check Windows Defender history.
```

### Expected Obfuscation Indicators

Static analysis should reveal:

```text
Convert.FromBase64String()
XOR 0x5A decoding loop
Base64 payload blob
DllImport("kernel32.dll")
VirtualAlloc()
VirtualProtect()
CreateThread()
WaitForSingleObject()
```

### Expected Dynamic Signals

Dynamic analysis tools may observe:

* Runtime payload decoding
* RW → RX memory permission transition
* Thread execution from dynamically allocated memory
* Local process creation of `calc.exe`
* Absence of a dropped payload executable
* No outbound network traffic

---

#########################################################################

# High-Level Obfuscated Loader Flow

1. Start the obfuscated loader:

   ```text
   A03_4_obfuscated_loader.exe
   ```

2. Load the Base64-encoded payload blob from the managed loader

3. Decode the Base64 blob using:

   ```text
   Convert.FromBase64String()
   ```

4. Allocate a decoded payload buffer

5. XOR-decode each byte using:

   ```text
   decoded[i] = encoded[i] ^ 0x5A
   ```

6. Print the decoded payload size

7. Allocate writable memory for the decoded shellcode using:

   ```text
   VirtualAlloc(..., PAGE_READWRITE)
   ```

8. Copy the decoded shellcode into the allocated memory region using:

   ```text
   Marshal.Copy()
   ```

9. Change the staged memory protection from writable to executable
   using:

   ```text
   VirtualProtect(..., PAGE_EXECUTE_READ, ...)
   ```

10. Create a new thread beginning at the decoded shellcode buffer using:

    ```text
    CreateThread()
    ```

11. Begin execution inside the decoded in-memory payload

12. Shellcode accesses the PEB through the `GS` segment

13. Shellcode walks loaded modules to locate:

    ```text
    kernel32.dll
    ```

14. Shellcode parses the export table manually

15. Shellcode resolves the runtime address of:

    ```text
    WinExec()
    ```

16. Shellcode constructs the command string:

    ```text
    calc.exe
    ```

17. Shellcode invokes:

    ```text
    WinExec("calc.exe", SW_SHOWNORMAL)
    ```

18. Launch:

    ```text
    calc.exe
    ```

19. Wait for the shellcode thread using:

    ```text
    WaitForSingleObject()
    ```

20. Exit cleanly after in-memory shellcode execution
