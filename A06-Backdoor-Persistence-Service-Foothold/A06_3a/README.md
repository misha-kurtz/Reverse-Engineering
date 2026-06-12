# A06.3a COM Logon-Triggered Scheduled Task Persistence

## Summary

Creates persistent execution on the Windows analysis VM by
registering a Windows Scheduled Task using the **Task Scheduler 2.0 COM API**,
configured to trigger automatically upon user logon.

The specimen demonstrates scheduled-task persistence behavior commonly
associated with backdoors, loaders, and long-term foothold malware that
requires reliable execution across user logon events without relying on
traditional Run-key or service-based persistence mechanisms.

Unlike command-line task creation (e.g., `schtasks.exe`), this sample uses
native COM interfaces (`ITaskService`, `ITaskDefinition`, etc.) to directly
interact with the Windows Task Scheduler subsystem. This approach is commonly
used by more advanced malware to avoid simple command-line detection and to
blend with legitimate system automation.

The scheduled task launches the following payload:

```text
C:\Users\Public\A06_3a_SampleApp.exe
```

The task is registered to run under the **SYSTEM account**, providing elevated
execution context while still triggering based on user logon activity.

The persistence payload itself is intentionally benign and exists only to
generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of:

```text
A06_3a_SampleApp.exe
```

after a successful user logon event triggers the scheduled task.

When executed, the payload application:

* Creates a persistent beacon-style logging loop
* Runs continuously within the system-triggered execution context
* Writes periodic timestamped status messages to:

```text
C:\Users\Public\A06_3a_COM_Logon_Triggered_Scheduled_Task_SampleApp_log.txt
```

The log entries confirm:

* Successful COM-based task execution
* Logon-triggered persistence activation
* Continued execution over time
* Scheduled task–driven process persistence

Example log messages include:

```text
SampleApp initialized via A06_3a COM Logon-Triggered Scheduled Task.
Beacon: A06_3a COM Logon-Triggered Scheduled Task process is alive and looping.
```

No additional payloads are downloaded, injected, staged, or executed.

---

## To Execute A06_3a_COM_logon_persist

### Register the Scheduled Task

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_3a\bin\A06_3a_COM_logon_persist.exe
```

### Verify Task Registration

```powershell
schtasks /query /tn "A06_3a_LogonTask"
```

### Trigger Persistence

Log off and log back into the Windows VM to trigger execution.

---

## Expected Persistence Artifacts

### Scheduled Task Registration

```text
Task Name: A06_3a_LogonTask
Trigger Type: Logon Trigger
Execution Context: SYSTEM
Implementation: Task Scheduler COM API
```

### Scheduled Payload

```text
C:\Users\Public\A06_3a_SampleApp.exe
```

### Dynamic Analysis Log Artifact

```text
C:\Users\Public\A06_3a_COM_Logon_Triggered_Scheduled_Task_SampleApp_log.txt
```

---

#########################################################################

# High-Level COM Logon-Triggered Scheduled Task Persistence Flow

1. Initialize COM library and security context

2. Instantiate Task Scheduler service via COM interface

3. Connect to local Task Scheduler service

4. Open root task folder (`\`)

5. Remove any existing task with the same name

6. Create a new task definition object

7. Configure registration metadata (author, task identity)

8. Create a **logon trigger**:

   ```text
   TASK_TRIGGER_LOGON
   ```

9. Define execution action:

   ```text
   C:\Users\Public\A06_3a_SampleApp.exe
   ```

10. Configure execution context:

* Run as SYSTEM
* Service account logon type

11. Register task definition with Task Scheduler

12. Exit installer process

13. Wait for user logon event

14. Task Scheduler detects user authentication event

15. Automatically launch payload application

16. Execute payload in scheduled-task context

17. Begin persistent beacon loop

18. Write execution telemetry to:

```text
C:\Users\Public\A06_3a_COM_Logon_Triggered_Scheduled_Task_SampleApp_log.txt
```

19. Continue execution until terminated

20. Persistence survives:

* User logoff/logon cycles
* System reboots
* Task Scheduler restarts

