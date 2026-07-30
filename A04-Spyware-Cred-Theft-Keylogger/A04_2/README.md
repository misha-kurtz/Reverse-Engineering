
# A04.2 Windows Credential Manager Extraction

## Summary

Extracts stored Windows Credential Manager entries by obtaining a
privileged process token, impersonating that security context, and
requesting a backup of credentials associated with a selected user
session.

The sample demonstrates operating-system credential access behavior
commonly associated with credential theft utilities, information
stealers, post-exploitation frameworks, and malware that attempts to
recover credentials stored by Windows.

The credential extraction workflow performs the following operations:

* Enables `SeDebugPrivilege`
* Identifies the user session associated with a supplied process ID
* Locates the `winlogon.exe` process in the same session
* Opens the Winlogon process token
* Duplicates the Winlogon security token
* Enables `SeTrustedCredManAccessPrivilege`
* Impersonates the duplicated privileged token
* Opens the target user process token
* Creates a temporary Credential Manager backup file
* Decrypts the backup using Windows DPAPI
* Parses individual credential records
* Displays recovered credential metadata
* Deletes the temporary backup file
* Reverts to the original process security context

The primary credential backup operation is performed using:

```text
CredBackupCredentials()
```

The resulting backup file is decrypted using:

```text
ProtectedData.Unprotect()
```

The implementation is intended for controlled laboratory analysis
and forensic artifact generation.

---

## Payload Summary

The payload behavior consists of retrieving and parsing Windows
Credential Manager entries associated with a selected logged-on user
session.

During execution, the sample:

* Accepts a target process ID
* Accepts a temporary backup file path
* Determines the target process session ID
* Finds the corresponding `winlogon.exe` process
* Duplicates the Winlogon access token
* Enables credential-management privileges
* Temporarily impersonates the privileged token
* Backs up Credential Manager data for the target user token
* Decrypts the generated backup file with DPAPI
* Parses stored credential records
* Prints recovered fields to the console
* Deletes the temporary credential backup
* Releases handles and reverts impersonation

Parsed credential fields may include:

* Target name
* Target alias
* Comment
* User name
* Credential data

The sample does not establish persistence, communicate over the
network, modify stored credentials, or transmit recovered data.

---

## Requirements

The sample must execute from a sufficiently privileged security
context.

Typical requirements include:

* Administrator or SYSTEM execution
* Access to the target user process
* Access to the matching `winlogon.exe` process
* Availability of `SeDebugPrivilege`
* Availability of `SeTrustedCredManAccessPrivilege`

The supplied process ID should belong to a process running in the
user session whose Credential Manager entries are being examined.

---

## To Execute A04_2

### Step 1 — Identify a process in the target user session

For example, identify the process ID of a user-owned process:

```powershell
Get-Process explorer
```

Example output:

```text
Handles  NPM(K)  PM(K)  WS(K)  CPU(s)   Id  ProcessName
-------  ------  -----  -----  ------   --  -----------
   2451      95  74320  128940   38.42  4120 explorer
```

In this example, the target process ID is:

```text
4120
```

### Step 2 — Launch A04_2 with elevated privileges

```powershell
.\A04-Credential-Theft-Spyware-Keylogger\A04_2\bin\A04_2_OS_cred_extract.exe 4120 C:\Users\Public\A04_2_cred_backup.tmp
```

The command-line arguments are:

```text
A04_2_OS_cred_extract.exe <target-process-PID> <temporary-backup-path>
```

---

## Expected Console Output

### Example Output

```text
[*] SeDebugPrivilege enabled
[*] Targeting process with PID 4120 which runs under session: 1
[*] Found Winlogon process with PID 728 matching session id: 1
[*] Opening Winlogon with PID 728
[*] Cloning token of Winlogon with PID 728
[*] Incoming creds!!!

    TargetName       : LegacyGeneric:target=example
    TargetAlias      :
    Comment          :
    UserName         : lab-user
    Credential       : example-password

[*] Deleting temporary file at C:\Users\Public\A04_2_cred_backup.tmp
[*] Reverting to self
```

Actual output depends on the Credential Manager entries stored for
the selected user.

---

## Temporary Credential Artifact

### Backup File

```text
C:\Users\Public\A04_2_cred_backup.tmp
```

The temporary file is created by:

```text
CredBackupCredentials()
```

The file contains a DPAPI-protected backup of Credential Manager
records.

After successful extraction and parsing, the sample deletes the
temporary file using:

```text
File.Delete()
```

If execution terminates unexpectedly, the backup file may remain
on disk and can appear in filesystem monitoring artifacts.

---


# High-Level Windows Credential Extraction Flow

1. Start the credential extraction executable with:

   * Target user-process PID
   * Temporary backup file path

2. Obtain a handle to the current process using:

   ```text
   GetCurrentProcess()
   ```

3. Open the current process token using:

   ```text
   OpenProcessToken()
   ```

4. Resolve the locally unique identifier for:

   ```text
   SeDebugPrivilege
   ```

5. Enable `SeDebugPrivilege` using:

   ```text
   LookupPrivilegeValue()
   AdjustTokenPrivileges()
   ```

6. Open the process specified by the supplied PID

7. Determine the target process session ID

8. Enumerate instances of:

   ```text
   winlogon.exe
   ```

9. Select the Winlogon process whose session ID matches the target
   user process

10. Open the matching Winlogon process using:

    ```text
    OpenProcess()
    ```

11. Open the Winlogon process token using:

    ```text
    OpenProcessToken()
    ```

12. Duplicate the Winlogon token using:

    ```text
    DuplicateTokenEx()
    ```

13. Resolve the locally unique identifier for:

    ```text
    SeTrustedCredManAccessPrivilege
    ```

14. Enable the credential-management privilege on the duplicated
    token using:

    ```text
    AdjustTokenPrivileges()
    ```

15. Open the target user process token

16. Impersonate the duplicated privileged token using:

    ```text
    ImpersonateLoggedOnUser()
    ```

17. Request a backup of Credential Manager entries using:

    ```text
    CredBackupCredentials()
    ```

18. Write the temporary credential backup to the supplied path

19. Read the temporary backup file into memory

20. Decrypt the backup using Windows DPAPI:

    ```text
    ProtectedData.Unprotect()
    ```

21. Remove the backup header from the decrypted data

22. Split the decrypted data into individual credential records

23. Parse each Credential Manager record

24. Extract fields including:

    * Target name
    * Target alias
    * Comment
    * User name
    * Credential value

25. Determine whether credential data contains Unicode text using:

    ```text
    IsTextUnicode()
    ```

26. Display readable credential data as Unicode text

27. Display non-text credential data as hexadecimal bytes

28. Delete the temporary backup file using:

    ```text
    File.Delete()
    ```

29. Revert to the original security context using:

    ```text
    RevertToSelf()
    ```

30. Close all process and token handles using:

    ```text
    CloseHandle()
    ```

31. Exit after credential parsing and resource cleanup
