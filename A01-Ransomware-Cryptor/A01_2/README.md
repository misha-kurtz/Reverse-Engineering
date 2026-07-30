# A01.2 `C++` Hidden-Tear Variant: Encryption + Coercion

## Summary

Encrypts user files located within a controlled laboratory directory using
AES-256 encryption before displaying coercive behavior by generating a
ransom note on the user's Desktop.

The sample demonstrates the two core behaviors commonly associated with
modern ransomware:

* Encryption of victim data
* Victim coercion through a ransom message

Unlike later ransomware variants, this specimen performs **local file
encryption and ransom note generation only**. It does **not**:

* Exfiltrate encryption keys
* Communicate with a command-and-control server
* Delete backups or shadow copies
* Establish persistence
* Perform double extortion

The encryption workflow performs the following operations:

* Generates a random encryption key
* Stores the generated key locally
* Recursively enumerates files within a predefined directory
* Filters files based on selected document and image extensions
* Encrypts eligible files using AES-256
* Renames each encrypted file by appending the `.locked` extension
* Creates a ransom note on the current user's Desktop
* Terminates after completion

The encryption key is intentionally written to disk to support controlled
laboratory validation and recovery of encrypted test files.

---

# Payload Summary

The payload behavior consists of recursively encrypting files located
within the controlled laboratory dataset:

```text
C:\Users\Public\A01_TestData
```

Only files matching predefined extensions are processed, including:

* Documents
* Spreadsheets
* Presentations
* Images
* Source code
* Database files
* Web content

Each eligible file is:

* Read into memory
* Encrypted using AES-256
* Overwritten with encrypted data
* Renamed with the extension:

```text
.locked
```

After encryption completes, a ransom note is written to the current
user's Desktop:

```text
C:\Users\<username>\Desktop\READ_IT.txt
```

A randomly generated encryption key is written to:

```text
C:\Users\Public\A01_2_Lab_Encryption_Key.txt
```

The key exists solely for controlled laboratory validation and recovery
of encrypted test files.

No persistence, network communication, credential theft,
or destructive recovery inhibition is performed.

---

# To Execute A01_2

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

Subdirectories may also be included to demonstrate recursive encryption.

### Step 2 — Execute the sample

```powershell
.\A01-Ransomware-Cryptor\A01_2\bin\A01_2_Ransomware_Encrypt_and_Coerce.exe
```

The sample executes automatically and exits after encryption and ransom
note creation complete.

---

# Expected Artifacts

## Encryption Key

```text
C:\Users\Public\A01_2_Lab_Encryption_Key.txt
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

## Ransom Note

```text
C:\Users\<username>\Desktop\READ_IT.txt
```

### Example Contents

```text
A01_2 controlled ransomware/coercion sample.

Files in C:\Users\Public\A01_TestData have been encrypted.

This note is generated for malware reverse-engineering dataset analysis.

No payment is required. Use the lab key file to decrypt test files.
```

---


# High-Level Local Encryption and Coercion Flow

1. Launch the ransomware specimen

2. Determine the currently logged-on user

3. Generate a random 15-character encryption key

4. Write the generated key to:

   ```text
   C:\Users\Public\A01_2_Lab_Encryption_Key.txt
   ```

5. Begin recursive traversal of:

   ```text
   C:\Users\Public\A01_TestData
   ```

6. Enumerate files within each directory

7. Filter files using the supported extension list

8. Read each eligible file into memory

9. Derive an AES-256 encryption key using the Windows CryptoAPI

10. Encrypt the file contents using:

    ```text
    AES-256
    ```

11. Overwrite the original file with encrypted data

12. Rename the encrypted file by appending:

    ```text
    .locked
    ```

13. Continue recursively through child directories

14. Skip files already ending with:

    ```text
    .locked
    ```

15. Ignore inaccessible files or directories and continue processing

16. Create the ransom note:

    ```text
    C:\Users\<username>\Desktop\READ_IT.txt
    ```

17. Inform the user that laboratory files have been encrypted and reference the recovery key

18. Exit cleanly after artifact generation

---

**Purpose**

Demonstrates the two foundational stages of ransomware operation: local encryption of victim files followed by user coercion through a ransom note. The specimen intentionally omits key exfiltration, persistence, and destructive behaviors while producing controlled static and dynamic artifacts suitable for reverse engineering, malware analysis, and retrieval-augmented generation (RAG) evaluation.
