
# A01.3 `C` Hidden-Tear Variant: Encryption + Recovery Inhibition

## Summary

Encrypts user files located within a controlled laboratory directory using
AES-256 encryption while demonstrating recovery inhibition techniques
commonly employed by modern ransomware.

The sample demonstrates the third stage of ransomware behavior:

* Local file encryption
* Recovery inhibition

Unlike earlier ransomware variants that only encrypt files or display a
ransom note, this specimen attempts to reduce the victim's ability to
recover encrypted data by targeting Windows backup and recovery
mechanisms.

The recovery inhibition workflow performs the following operations:

* Generates a random encryption key
* Stores the generated key locally
* Enumerates mounted storage volumes
* Identifies potential backup volumes
* Encrypts files located on detected backup volumes
* Executes Windows recovery-inhibition commands
* Attempts to disable backup-related services
* Encrypts the primary laboratory dataset
* Terminates after completion

The encryption key is intentionally written to disk to support controlled
laboratory validation and recovery of encrypted test files.

---

# Payload Summary

The payload behavior consists of encrypting files within the controlled
laboratory dataset:

```text
C:\Users\Public\A01_TestData
```

Prior to encrypting the primary dataset, the specimen enumerates local
storage devices in search of potential backup volumes.

Volumes whose labels indicate backup storage are identified and processed
before encryption continues.

Each eligible file is:

* Read into memory
* Encrypted using AES-256
* Overwritten with encrypted data
* Renamed with the extension:

```text
.locked
```

The specimen then attempts to inhibit recovery by executing several
administrative recovery-management utilities.

A randomly generated encryption key is written to:

```text
C:\Users\Public\A01_3_Lab_Encryption_Key.txt
```

The key exists solely for controlled laboratory validation and recovery
of encrypted test files.

No persistence, network communication, credential theft,
or command-and-control functionality is performed.

---

# To Execute A01_3

### Step 1 — Prepare the laboratory dataset

Create the controlled input directory:

```text
C:\Users\Public\A01_TestData
```

Populate the directory with representative files such as:

* `.txt`
* `.docx`
* `.xlsx`
* `.pptx`
* `.jpg`
* `.png`
* `.csv`

Optionally create a secondary drive or mounted volume with a label
containing:

```text
Backup
```

to demonstrate backup volume discovery and encryption.

### Step 2 — Execute the sample

```powershell
.\A01-Ransomware-Cryptor\A01_3\bin\A01_3_Ransomware_Recovery_Inhibition.exe
```

The sample executes automatically and exits after encryption and
recovery-inhibition activities complete.

---

# Expected Artifacts

## Encryption Key

```text
C:\Users\Public\A01_3_Lab_Encryption_Key.txt
```

## Encrypted Files

Original files become:

```text
example.docx
```

↓

```text
example.docx.locked
```

The original plaintext file is replaced by its encrypted counterpart.

## Recovery-Inhibition Activity

During execution, the sample launches several native Windows recovery
management utilities including:

```text
vssadmin.exe
bcdedit.exe
wbadmin.exe
wmic.exe
cmd.exe
```

If a backup volume is detected, files on that volume are also encrypted.

---


# High-Level Encryption and Recovery Inhibition Flow

1. Launch the ransomware specimen

2. Generate a random 15-character encryption key

3. Write the generated key to:

   ```text
   C:\Users\Public\A01_3_Lab_Encryption_Key.txt
   ```

4. Enumerate mounted logical drives

5. Inspect each drive using:

   ```text
   GetVolumeInformationA()
   ```

6. Identify potential backup volumes by examining the volume label

7. If a backup volume is discovered:

   * Recursively enumerate its files
   * Encrypt eligible files
   * Append:

     ```text
     .locked
     ```

8. Execute Windows recovery-inhibition commands:

   ```text
   vssadmin.exe delete shadows /all /quiet
   ```

9. Disable Windows Recovery Environment:

   ```text
   bcdedit.exe /set {default} recoveryenabled No
   ```

10. Suppress boot recovery behavior:

    ```text
    bcdedit.exe /set {default} bootstatuspolicy ignoreallfailures
    ```

11. Remove Windows Backup catalog:

    ```text
    wbadmin.exe delete catalog -quiet
    ```

12. Delete shadow copies using:

    ```text
    wmic.exe shadowcopy delete
    ```

13. Attempt to stop and disable backup-related services

14. Begin recursive traversal of:

    ```text
    C:\Users\Public\A01_TestData
    ```

15. Encrypt eligible files using:

    ```text
    AES-256
    ```

16. Rename encrypted files by appending:

    ```text
    .locked
    ```

17. Securely erase the in-memory encryption key

18. Exit cleanly after artifact generation

---

**Purpose**

Demonstrates the ransomware transition from simple file encryption to active recovery inhibition by targeting backup media, Windows shadow copies, recovery configuration, and backup services before encrypting the laboratory dataset. The specimen intentionally omits persistence, key exfiltration, and network communication while generating controlled static and dynamic artifacts suitable for reverse engineering, malware analysis, and retrieval-augmented generation (RAG) evaluation. 
