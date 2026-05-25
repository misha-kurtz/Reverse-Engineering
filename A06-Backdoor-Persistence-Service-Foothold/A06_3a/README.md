# A06_3a Logon-Triggered Scheduled Task Persistence

## Summary

Creates persistent execution on the Windows analysis VM by
registering a Windows Scheduled Task configured to trigger
automatically when a user logs into the system.

The specimen demonstrates scheduled-task persistence behavior
commonly associated with backdoors, loaders, remote access trojans,
and long-term foothold malware that requires reliable execution
without relying solely on registry Run-keys or Windows services.

The sample leverages the Windows Task Scheduler infrastructure
to establish persistence through a logon-triggered execution model.
When a user logs in, the Task Scheduler service automatically launches
the configured payload application inside the interactive user session.

The scheduled task launches the following payload:

```text
C:\Users\Public\A06_3a_SampleApp.exe
```

This demonstrates a common persistence pattern in which malware
uses Task Scheduler to obtain durable execution across reboots
and user logons while blending into legitimate administrative
automation activity already common on Windows systems.

The persistence payload itself is intentionally benign and exists
only to generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of:

```text
A06_3a_SampleApp.exe
```

after a successful user logon event triggers the scheduled task.

When executed, the payload application:

* Creates a persistent beacon-style logging loop
* Runs continuously within the interactive user session
* Writes periodic timestamped status messages to:

```text
C:\Users\Public\A06_3a_Logon_Triggered_Scheduled_Task_SampleApp_log.txt
```

The log entries confirm:

* Successful scheduled-task execution
* User-session payload launch
* Continued execution over time
* Persistence watchdog behavior

Example log messages include:

```text
SampleApp initialized successfully via A06_3a Logon-Triggered Scheduled Task.
Beacon: A06_3a Logon-Triggered Scheduled Task process is alive and looping.
```

No additional payloads are downloaded, injected, staged, or executed.

---

## To Execute A06_3a_schtask_logon_persistence

### Register the Scheduled Task

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_3a\bin\A06_3a_schtask_logon_persistence.exe
```

### Alternative Manual Verification

```powershell
schtasks /query /tn "A06_3a_LogonPersistence"
```

### Trigger Persistence

Log off and log back into the Windows VM to trigger the task.

---

## Expected Persistence Artifacts

### Scheduled Task Registration

```text
Task Name: A06_3a_LogonPersistence
Trigger Type: User Logon
```

### Scheduled Payload

```text
C:\Users\Public\A06_3a_SampleApp.exe
```

### Dynamic Analysis Log Artifact

```text
C:\Users\Public\A06_3a_Logon_Triggered_Scheduled_Task_SampleApp_log.txt
```

---

#########################################################################

# High-Level Logon-Triggered Scheduled Task Persistence Flow

1. Initialize Task Scheduler persistence workflow

2. Configure the scheduled task name:

   ```text
   A06_3a_LogonPersistence
   ```

3. Configure the payload executable path:

   ```text
   C:\Users\Public\A06_3a_SampleApp.exe
   ```

4. Create a scheduled task configured for:

   * Logon-triggered execution
   * Persistent automatic launch
   * Interactive user-session execution

5. Register the task with the Windows Task Scheduler service

6. Store the task definition within the Task Scheduler subsystem

7. Exit the installer process after successful task registration

8. Wait for a user logon event

9. Detect user authentication through the Windows logon process

10. Trigger automatic scheduled task execution

11. Launch:

    ```text
    C:\Users\Public\A06_3a_SampleApp.exe
    ```

12. Execute the payload application inside the interactive desktop session

13. Begin persistent background beacon loop inside the payload application

14. Write periodic timestamped execution logs to:

    ```text
    C:\Users\Public\A06_3a_Logon_Triggered_Scheduled_Task_SampleApp_log.txt
    ```

15. Continue execution indefinitely until terminated

16. Preserve persistence across:

    * User logoff/logon cycles
    * System reboots
    * Task Scheduler service restarts

17. Exit cleanly when the payload application is terminated
