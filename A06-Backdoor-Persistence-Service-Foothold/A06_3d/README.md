# A06_3d Logon-Triggered Scheduled Task Persistence

## Summary

Creates persistent execution on the Windows analysis VM by
registering a Windows Scheduled Task using the native
`schtasks.exe` utility, configured to execute automatically
whenever a user logs on to the system.

The specimen demonstrates logon-triggered scheduled-task
persistence commonly associated with backdoors, loaders,
and long-term foothold malware seeking automatic execution
during user authentication events.

Unlike recurring time-based scheduled tasks, this sample
leverages a user logon trigger, causing execution whenever
a logon event occurs.

The scheduled task is configured to:

* Execute under the SYSTEM account
* Trigger automatically upon user logon
* Launch a predefined payload executable
* Persist across reboots and user sessions

The task launches the following payload:

```text
C:\Users\Public\A06_3d_SampleApp.exe
```

The persistence payload is intentionally benign and exists
only to generate controlled forensic artifacts for dynamic
analysis.

---

## Payload Summary

The payload behavior is the execution of:

```text
A06_3d_SampleApp.exe
```

whenever the scheduled task is triggered by a system logon
event.

When executed, the payload application:

* Writes a timestamped initialization log entry
* Enters a continuous execution loop
* Generates recurring beacon-style log entries every 5 seconds
* Confirms successful scheduled-task execution
* Writes telemetry to:

```text
C:\Users\Public\A06_3d_Scheduled_Task_SampleApp_log.txt
```

Example log messages include:

```text
[2026-06-12 14:00:00] SampleApp initialized successfully via A06_3d Logon-Triggered Scheduled Task.

[2026-06-12 14:00:05] Beacon: A06_3d Logon-Triggered Task process is alive and looping.
```

No additional payloads are downloaded, injected, or executed.

---

## To Execute A06_3d_schtask_onlogon

### Register the Scheduled Task

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_3d\bin\A06_3d_schtask_onlogon.exe
```

The specimen will create the following task:

```text
A06_3d_Task
```

---

### Verify Task Registration

```powershell
schtasks /query /tn "A06_3d_Task"
```

Expected output should indicate:

```text
TaskName: A06_3d_Task
Schedule Type: At logon
Run As User: SYSTEM
```

---

### Trigger Execution

Log off and log back into the Windows analysis VM.

Alternatively, reboot the system and perform a normal user logon.

---

### Observe Payload Activity

Monitor:

```text
C:\Users\Public\A06_3d_Scheduled_Task_SampleApp_log.txt
```

for evidence of task execution.

---

## Expected Persistence Artifacts

### Scheduled Task Registration

```text
Task Name: A06_3d_Task
Trigger Type: On Logon
Execution Context: SYSTEM
```

### Scheduled Payload

```text
C:\Users\Public\A06_3d_SampleApp.exe
```

### Dynamic Analysis Log Artifact

```text
C:\Users\Public\A06_3d_Scheduled_Task_SampleApp_log.txt
```

### Task Scheduler Artifact Locations

```text
C:\Windows\System32\Tasks\A06_3d_Task
```

Task metadata may also be visible through:

```text
Task Scheduler Library
```

within the Windows Task Scheduler management console.

---

#########################################################################

## High-Level Logon-Triggered Scheduled Task Persistence Flow

1. Build scheduled-task registration command

2. Launch:

   ```text
   schtasks.exe
   ```

   using:

   ```text
   CreateProcessW()
   ```

3. Register scheduled task:

   ```text
   A06_3d_Task
   ```

4. Configure trigger type:

   ```text
   ONLOGON
   ```

5. Configure execution context:

   ```text
   SYSTEM
   ```

6. Configure task action:

   ```text
   C:\Users\Public\A06_3d_SampleApp.exe
   ```

7. Task Scheduler stores registration data

8. Persistence survives:

   * Reboots
   * User logoff
   * User logon
   * System restart

9. User performs a logon operation

10. Task Scheduler evaluates configured triggers

11. ONLOGON trigger condition is satisfied

12. Task Scheduler launches payload automatically

13. Payload writes initialization telemetry

14. Payload enters continuous execution loop

15. Beacon entries are written every 5 seconds

16. Dynamic-analysis artifacts accumulate on disk

17. Persistence remains active until the task is removed

18. Future logon events continue triggering execution

