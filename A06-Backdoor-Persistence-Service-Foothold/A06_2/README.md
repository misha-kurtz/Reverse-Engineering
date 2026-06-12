# A06.2 Service-Based Persistence

## Summary

Creates persistent execution on the Windows analysis VM by
installing and running a Windows service named:

```text
A06_2_Persistent_Service
```

The specimen demonstrates service-based persistence commonly
associated with backdoors, remote access trojans, loaders,
and long-term foothold malware that require durable execution
across system reboots and user logon events.

Unlike simple Run-key persistence mechanisms, this sample
leverages the Windows Service Control Manager (SCM) to
establish a persistent background process that automatically
starts during system boot.

The service additionally performs user-session discovery,
token impersonation, and user-context process creation
through native Windows session-management APIs.

Once an active interactive user session is identified,
the service launches a secondary user-session watchdog
instance of itself using:

```text
CreateProcessAsUserW()
```

The watchdog continuously monitors a configured payload
application and automatically restarts it if the process
terminates.

The configured payload application is:

```text
C:\Users\Public\A06_2_SampleApp.exe
```

This demonstrates a common persistence pattern in which
a privileged service acts as a broker between SYSTEM-level
execution and user-interactive processes.

The persistence payload itself is intentionally benign and
exists solely to generate controlled forensic artifacts for
dynamic analysis.

---

## Persistence Components

### Service Component

The primary persistence component is:

```text
A06_2_persistent_service.exe
```

Responsibilities include:

* Service installation
* SCM registration
* Automatic startup persistence
* Session change monitoring
* User token acquisition
* User-context process creation
* Payload watchdog functionality

---

### Payload Component

The monitored payload is:

```text
A06_2_SampleApp.exe
```

Responsibilities include:

* User-session execution
* Continuous beacon-style logging
* Generation of dynamic analysis artifacts
* Validation of watchdog recovery behavior

---

## Payload Summary

The payload behavior is the execution of the user-session
application:

```text
A06_2_SampleApp.exe
```

The sample application is launched from the persistent
service after an interactive user session becomes available.

When executed, the payload application:

* Creates a persistent beacon-style logging loop
* Runs continuously in the logged-on user session
* Writes periodic timestamped status messages to:

```text
C:\Users\Public\A06_2_Persistent_Service_SampleApp_log.txt
```

The log entries confirm:

* Successful launch from the service context
* Continued execution over time
* Persistence watchdog behavior
* User-session process spawning

Example log messages include:

```text
SampleApp initialized successfully under the logged-on user session.
Beacon: SampleApp process is alive and looping.
```

No additional payloads are downloaded, injected, staged,
or executed.

---

## To Execute A06_2_persistent_service

### Step 1 — Prepare Payload

Place:

```text
A06_2_SampleApp.exe
```

at:

```text
C:\Users\Public\A06_2_SampleApp.exe
```

---

### Step 2 — Execute Installer

Run:

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_2\bin\A06_2_persistent_service.exe
```

The sample will:

1. Create:

   ```text
   HKLM\SOFTWARE\A06_2_Persistent_Service
   ```

2. Store the payload path:

   ```text
   C:\Users\Public\A06_2_SampleApp.exe
   ```

3. Register:

   ```text
   A06_2_Persistent_Service
   ```

   with the Service Control Manager

4. Configure automatic startup persistence

5. Start the service

---

### Step 3 — Verify Service Installation

```powershell
Get-Service A06_2_Persistent_Service
```

Expected output:

```text
Status   Name                       DisplayName
------   ----                       -----------
Running  A06_2_Persistent_Service   A06_2_Persistent_Service
```

---

### Step 4 — Verify Payload Execution

Inspect:

```text
C:\Users\Public\A06_2_Persistent_Service_SampleApp_log.txt
```

Expected contents:

```text
[2026-01-01 12:00:00] SampleApp initialized successfully under the logged-on user session.
[2026-01-01 12:00:05] Beacon: SampleApp process is alive and looping.
[2026-01-01 12:00:10] Beacon: SampleApp process is alive and looping.
```

---

## Expected Persistence Artifacts

### Registered Windows Service

```text
Service Name: A06_2_Persistent_Service
Startup Type: Automatic
Service Type: Win32 Own Process
```

---

### Service Configuration Registry Key

```text
HKLM\SOFTWARE\A06_2_Persistent_Service
```

Value:

```text
Path
```

Data:

```text
C:\Users\Public\A06_2_SampleApp.exe
```

---

### Service Payload

```text
C:\Users\Public\A06_2_SampleApp.exe
```

---

### Service Log

```text
C:\Users\Public\A06_2_Persistent_Service_log.txt
```

---

### Payload Log

```text
C:\Users\Public\A06_2_Persistent_Service_SampleApp_log.txt
```

---

## Watchdog Behavior

After a user logs on, the service launches a user-session
watchdog instance of itself.

The watchdog:

1. Reads the configured payload path from:

   ```text
   HKLM\SOFTWARE\A06_2_Persistent_Service
   ```

2. Enumerates running processes using:

   ```text
   CreateToolhelp32Snapshot()
   ```

3. Searches for:

   ```text
   A06_2_SampleApp.exe
   ```

4. Launches the payload if it is not running

5. Repeats the monitoring process every:

   ```text
   10 seconds
   ```

This models persistence mechanisms commonly used by malware
families that automatically recover terminated payloads.

---

## User Session Bridging Behavior

The service operates within Session 0 and cannot directly
interact with a user's desktop session.

To bridge this isolation boundary, the service:

1. Enumerates active sessions using:

   ```text
   WTSEnumerateSessions()
   ```

2. Obtains the active user's token using:

   ```text
   WTSQueryUserToken()
   ```

3. Duplicates the token using:

   ```text
   DuplicateTokenEx()
   ```

4. Builds a user environment block using:

   ```text
   CreateEnvironmentBlock()
   ```

5. Launches a user-session watchdog process using:

   ```text
   CreateProcessAsUserW()
   ```

This creates a process running within the logged-on user's
interactive session while the service itself remains running
under the Service Control Manager.

---

#########################################################################

# High-Level Service Persistence Flow

1. Execute the installer binary

2. Create:

   ```text
   HKLM\SOFTWARE\A06_2_Persistent_Service
   ```

3. Store the configured payload path

4. Register:

   ```text
   A06_2_Persistent_Service
   ```

   with the Service Control Manager

5. Configure automatic startup persistence

6. Start the service

7. Initialize service control dispatcher

8. Register service control handlers

9. Transition the service into a running state

10. Detect active interactive user sessions

11. Obtain active-user access token

12. Duplicate the user token

13. Build a user environment block

14. Launch a user-session watchdog instance using:

    ```text
    CreateProcessAsUserW()
    ```

15. Create launcher mutex:

    ```text
    Local\A06_2_Launcher_Instance_Mutex
    ```

16. Read payload configuration from:

    ```text
    HKLM\SOFTWARE\A06_2_Persistent_Service
    ```

17. Start watchdog timer loop

18. Enumerate running processes

19. Determine whether:

    ```text
    A06_2_SampleApp.exe
    ```

    is already running

20. Launch the payload if it is not running

21. Execute payload within the interactive user session

22. Write periodic timestamped status messages

23. Continue monitoring every 10 seconds

24. Restart the payload if it terminates

25. Respond to SCM stop, shutdown, and session-change events

26. Clean up service handles, process handles, and impersonation state

27. Exit cleanly when the service is stopped