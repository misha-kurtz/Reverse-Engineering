# A01_1 `C#` Hidden-Tear Variant: Local File Encryption Only

## Summary

Encrypts user files located within a controlled laboratory directory using
AES-256 encryption before renaming each encrypted file with a
`.locked` extension.

The sample demonstrates the core encryption behavior commonly associated
with ransomware families whose primary objective is rendering victim
data inaccessible through symmetric cryptography.

Unlike later ransomware variants, this specimen performs **local file
encryption only**. It does **not**:

* Display a ransom note
* Exfiltrate encryption keys
* Communicate with a command-and-control server
* Delete backups or shadow copies
* Establish persistence

The encryption workflow performs the following operations:

* Generates a random encryption key
* Stores the generated key locally
* Recursively enumerates files within a predefined directory
* Filters files based on selected document and image extensions
* Encrypts eligible files using AES-256 in CBC mode
* Renames each encrypted file by appending the `.locked` extension
* Terminates after encryption completes

The encryption key is intentionally written to disk to support controlled
laboratory validation and recovery of encrypted test files.

---

## Payload Summary

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

A randomly generated encryption key is written to:

```text
C:\Users\Public\A01_1_Lab_Encryption_Key.txt
```

This key exists solely for controlled laboratory validation and decryption
of the encrypted test dataset.

No persistence, network communication, credential theft,
or destructive recovery inhibition is performed.

---

## To Execute A01_1

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
.\A01-Ransomware-Cryptor\A01_1\bin\A01_1_Ransomware_LocalEncryption.exe
```

The sample executes automatically and exits after encryption completes.

---

## Expected Encryption Artifacts

### Encryption Key

```text
C:\Users\Public\A01_1_Lab_Encryption_Key.txt
```

### Encrypted Files

Original files become:

```text
example.docx
```

↓

```text
example.docx.locked
```

The original plaintext file is replaced by its encrypted counterpart.

---

#########################################################################

# High-Level Local Encryption Flow

1. Launch the ransomware specimen

2. Generate a random 15-character encryption key

3. Write the generated key to:

   ```text
   C:\Users\Public\A01_1_Lab_Encryption_Key.txt
   ```

4. Begin recursive traversal of:

   ```text
   C:\Users\Public\A01_TestData
   ```

5. Enumerate files within the current directory

6. Filter files using the supported extension list

7. Read each eligible file into memory

8. Hash the generated password using:

   ```text
   SHA-256
   ```

9. Derive the AES encryption key and initialization vector (IV) using:

   ```text
   PBKDF2 (Rfc2898DeriveBytes)
   ```

10. Encrypt file contents using:

    ```text
    AES-256
    CBC Mode
    ```

11. Overwrite the original file with encrypted data

12. Rename the encrypted file by appending:

    ```text
    .locked
    ```

13. Continue recursively into child directories

14. Skip files already ending with:

    ```text
    .locked
    ```

15. Ignore inaccessible files or directories and continue processing

16. Exit cleanly after all eligible files have been processed

---

**Purpose**

Demonstrates the foundational ransomware capability of locally encrypting victim files while producing controlled static and dynamic artifacts suitable for reverse engineering, malware analysis, and retrieval-augmented generation (RAG) evaluation.