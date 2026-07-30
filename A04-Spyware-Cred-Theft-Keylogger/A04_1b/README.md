# A04.1b Polling-Based Keylogger

## Summary

Captures keyboard input by continuously polling keyboard state using
the Windows asynchronous keyboard API rather than installing a
keyboard hook.

The sample demonstrates keyboard polling behavior commonly associated
with credential theft malware, spyware, information stealers, and
lightweight surveillance implants that periodically inspect keyboard
state to capture user input.

The keylogging workflow performs the following operations:

* Continuously polls keyboard state
* Detects newly pressed keys
* Determines the currently active application window
* Tracks foreground window changes
* Converts virtual key codes into readable characters
* Records printable characters and special keys
* Writes captured keystrokes to a local log file

The keyboard polling mechanism is implemented using:

```text
GetAsyncKeyState()
```

Captured keystrokes are grouped according to the foreground
application to preserve typing context.

The implementation is intentionally benign and exists solely
to generate controlled forensic artifacts for reverse engineering
and dynamic malware analysis.

---

## Payload Summary

The payload behavior consists of continuously monitoring keyboard
state and recording user input to a local log.

During execution, the sample:

* Hides its console window
* Polls keyboard state approximately every 10 milliseconds
* Detects newly pressed keys
* Determines the active foreground window
* Converts alphanumeric keys according to Shift and Caps Lock state
* Records common special keys
* Detects application focus changes
* Groups captured keystrokes beneath application headers
* Writes all captured data to:

```text
C:\Users\Public\A04_1b_polling_keylog.txt
```

No persistence, privilege escalation, network communication,
credential exfiltration, or destructive functionality is performed.

---

## To Execute A04_1b

### Step 1 — Launch the polling keylogger

```powershell
.\A04-Credential-Theft-Spyware-Keylogger\A04_1b\bin\A04_1b_polling_keylogger.exe
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
C:\Users\Public\A04_1b_polling_keylog.txt
```

### Example Contents

```text
### Window: Untitled - Notepad ###
Hello World!

### Window: Windows PowerShell ###
whoami

### Window: File Explorer ###
documents

### Window: Command Prompt ###
ipconfig
```

---

# High-Level Polling-Based Keylogging Flow

1. Start the polling keylogger executable

   ```text
   A04_1b_polling_keylogger.exe
   ```

2. Hide the console window using:

   ```text
   ShowWindow(SW_HIDE)
   ```

3. Enter an infinite monitoring loop

4. Pause briefly between polling cycles using:

   ```text
   Sleep(10)
   ```

5. Determine the active foreground window using:

   ```text
   GetForegroundWindow()
   ```

6. Retrieve the active window title using:

   ```text
   GetWindowTextW()
   ```

7. Detect foreground window changes

8. Write a new application header whenever focus changes

9. Iterate through virtual key codes:

   ```text
   8–190
   ```

10. Poll keyboard state using:

    ```text
    GetAsyncKeyState()
    ```

11. Detect newly pressed keys

12. Determine Shift and Caps Lock state using:

    ```text
    GetAsyncKeyState(VK_SHIFT)
    GetKeyState(VK_CAPITAL)
    ```

13. Translate virtual key codes into printable characters

14. Convert special keys into readable labels, including:

    * Enter
    * Backspace
    * Tab
    * Shift
    * Control
    * Alt
    * Arrow keys
    * Caps Lock
    * Mouse right-click

15. Append captured keystrokes to:

    ```text
    C:\Users\Public\A04_1b_polling_keylog.txt
    ```

16. Continue polling indefinitely until the process is terminated