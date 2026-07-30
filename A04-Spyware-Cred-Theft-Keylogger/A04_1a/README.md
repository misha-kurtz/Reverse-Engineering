# A04.1a Hook-Based Keylogger

## Summary

Captures keyboard input using a global low-level Windows keyboard
hook based on the Windows Hooking API.

The sample demonstrates user input interception behavior commonly
associated with credential theft malware, spyware, information
stealers, surveillance implants, and remote-access trojans.

The keylogging workflow performs the following operations:

* Installs a global low-level keyboard hook
* Monitors all keyboard input events
* Identifies the currently active application window
* Converts virtual key codes into human-readable characters
* Tracks window focus changes
* Writes captured keystrokes to a local log file

The keyboard interception mechanism is implemented using:

```text
SetWindowsHookEx(WH_KEYBOARD_LL)
```

Captured keystrokes are grouped according to the foreground
application in order to preserve typing context.

The implementation is intentionally benign and exists solely
to generate controlled forensic artifacts for reverse engineering
and dynamic malware analysis.

---

## Payload Summary

The payload behavior consists of continuous keyboard event
monitoring and local keystroke logging.

During execution, the sample:

* Installs a low-level keyboard hook
* Processes `WM_KEYDOWN` events
* Determines the active foreground window
* Converts virtual key codes using the active keyboard layout
* Records printable characters and special keys
* Detects application focus changes
* Groups captured keystrokes beneath application headers
* Writes all captured data to:

```text
C:\Users\Public\A04_1a_hookbased_keylog.txt
```

Application errors and unexpected exceptions are written to:

```text
C:\Users\Public\A04_1a_hookbased_keylogger_error_log.txt
```

No persistence, privilege escalation, network communication,
credential exfiltration, or destructive functionality is performed.

---

## To Execute A04_1a

### Step 1 — Launch the keylogger

```powershell
.\A04-Credential-Theft-Spyware-Keylogger\A04_1a\bin\A04_1a_hookbased_keylogger.exe
```

### Step 2 — Generate keyboard activity

Interact with several Windows applications, for example:

* Notepad
* File Explorer
* Command Prompt
* PowerShell

Type various characters while switching between windows to
generate representative forensic artifacts.

---

## Expected Logging Artifact

### Output File

```text
C:\Users\Public\A04_1a_hookbased_keylog.txt
```

### Example Contents

```text
### Notepad ###
Hello World!

### Windows PowerShell ###
whoami

### File Explorer ###
documents
```

### Error Log

```text
C:\Users\Public\A04_1a_hookbased_keylogger_error_log.txt
```

---


# High-Level Hook-Based Keylogging Flow

1. Start the keylogger executable

   ```text
   A04_1a_hookbased_keylogger.exe
   ```

2. Register global exception handlers

3. Install a low-level keyboard hook using:

   ```text
   SetWindowsHookEx(WH_KEYBOARD_LL)
   ```

4. Enter the Windows message loop using:

   ```text
   Application.Run()
   ```

5. Receive keyboard events through the hook callback

6. Process:

   ```text
   WM_KEYDOWN
   ```

   keyboard messages

7. Read the intercepted virtual key code

8. Determine:

   * Shift state
   * Caps Lock state

9. Translate the virtual key into a printable character using:

   ```text
   ToUnicodeEx()
   ```

10. Identify the active foreground application using:

    ```text
    GetForegroundWindow()
    ```

11. Resolve the active process and window title

12. Detect foreground window changes

13. Insert a new application header whenever focus changes

14. Append captured keystrokes to:

    ```text
    C:\Users\Public\A04_1a_hookbased_keylog.txt
    ```

15. Record any runtime exceptions to:

    ```text
    C:\Users\Public\A04_1a_hookbased_keylogger_error_log.txt
    ```

16. Remove the keyboard hook during program shutdown using:

    ```text
    UnhookWindowsHookEx()
    ```

17. Exit cleanly after releasing hook resources