# A03_1 Embedded Payload Dropper

## Summary

Drops and executes an embedded payload executable from within the
loader process.

The sample demonstrates classic dropper behavior commonly associated
with malware installers, staged payload deployment, embedded resource
execution, and first-stage delivery mechanisms where a secondary
payload is stored inside the primary executable and written to disk
during runtime.

Unlike fileless loaders that execute payloads entirely in memory,
this specimen intentionally performs an explicit disk drop operation
prior to execution in order to generate clear forensic artifacts for
dynamic analysis.

The embedded payload is intentionally benign and exists solely to
generate controlled execution artifacts.

After extraction, the payload executable is written to disk and
executed as a child process.

The payload itself performs only non-malicious confirmation behavior.

---

## Payload Summary

The payload behavior is the extraction and execution of an embedded
payload executable.

When executed successfully, the loader:

* Locates the embedded payload bytes inside the executable
* Writes the payload executable to disk
* Launches the dropped executable using a child process
* Generates controlled forensic artifacts for dynamic analysis

The dropped payload executable then:

* Determines its own process path
* Retrieves its process identifier
* Captures execution timestamp information
* Writes a confirmation artifact to:

```text
C:\Users\Public\A03_1_Embedded_Dropper_OK.txt
```

The artifact contains:

* Timestamp
* Process ID
* Process path
* Embedded payload execution confirmation string

The payload additionally emits a debug confirmation string using:

```text
OutputDebugStringA()
```

No persistence, credential theft, privilege escalation,
network communication, or destructive functionality is performed.

---

## To Execute A03_1

Run the embedded payload dropper directly:

```powershell
.\A03-Dropper-Loader-Stager\A03_1\bin\A03_1_embedded_payload_dropper.exe
```

The sample automatically extracts and executes the embedded payload.

No external network connectivity is required.

---

## Expected Dropper Artifact

### Dropped Payload

Example output location:

```text
C:\Users\Public\A03_1_payload.exe
```

### Confirmation Artifact

```text
C:\Users\Public\A03_1_Embedded_Dropper_OK.txt
```

### Example Contents

```text
THESIS_A03_1_EMBEDDED_PAYLOAD_EXECUTED
Timestamp: 2026-05-25 14:10:33
PID: 1234
ProcessPath: C:\Users\Public\A03_1_payload.exe
```

---

#########################################################################

# High-Level Embedded Payload Dropper Flow

1. Start the embedded payload dropper:

   ```text
   A03_1_embedded_payload_dropper.exe
   ```

2. Locate the embedded payload bytes stored inside the executable

3. Parse or identify the embedded payload resource or byte array

4. Prepare the output path for the dropped payload

5. Create the output file on disk using:

   ```text
   CreateFile()
   ```

6. Write the embedded payload bytes to disk using:

   ```text
   WriteFile()
   ```

7. Close the dropped payload file handle

8. Verify successful payload extraction

9. Launch the dropped payload executable using:

   ```text
   CreateProcess()
   ```

10. Begin execution of the dropped payload executable

11. Collect execution metadata:

    * PID
    * Timestamp
    * Process path

12. Write confirmation artifact to:

    ```text
    C:\Users\Public\A03_1_Embedded_Dropper_OK.txt
    ```

13. Emit a debug confirmation string using:

    ```text
    OutputDebugStringA()
    ```

14. Exit cleanly after artifact generation and payload execution
