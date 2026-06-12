# A06_3c COM-Based Recurring Scheduled Task Persistence

## Summary

Creates persistent execution on the Windows analysis VM by
registering a Windows Scheduled Task using the Task Scheduler 2.0
COM API, configured to execute on a recurring time interval.

The specimen demonstrates time-based scheduled-task persistence
commonly associated with backdoors, loaders, and long-term foothold
malware that requires periodic execution independent of user activity.

Unlike logon-triggered or event-driven persistence, this sample
leverages a time-triggered execution model, ensuring consistent,
predictable execution at fixed intervals.

The scheduled task is configured to:

- Execute under the SYSTEM account  
- Trigger automatically on a time-based schedule  
- Repeat execution every 1 minute  
- Run regardless of user logon state  

The task launches the following payload:

```text
C:\Users\Public\A06_3c_SampleApp.exe
````

The persistence payload is intentionally benign and exists only to
generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of:

```text
A06_3c_SampleApp.exe
```

on a recurring 1-minute interval defined by the Task Scheduler.

When executed, the payload application:

* Writes a timestamped log entry upon each execution
* Confirms repeated scheduled execution behavior
* Runs in a continuous loop once triggered
* Writes telemetry to:

```text
C:\Users\Public\A06_3c_COM_Recurring_Scheduled_Task_SampleApp_log.txt
```

Example log messages include:

```text
[2026-06-12 14:00:00] [A06_3c] COM Recurring Scheduled Task execution active (interval=1m).
```

No additional payloads are downloaded, injected, or executed.

---

## To Execute A06_3c_COM_recur_persist

### Register the Scheduled Task

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_3c\bin\A06_3c_COM_recur_persist.exe
```

### Verify Task Registration

```powershell
schtasks /query /tn "A06_3c_RecurTask"
```

### Observe Execution

Wait approximately 1 minute for the first execution cycle, then monitor:

```text
C:\Users\Public\A06_3c_COM_Recurring_Scheduled_Task_SampleApp_log.txt
```

---

## Expected Persistence Artifacts

### Scheduled Task Registration

```text
Task Name: A06_3c_RecurTask
Trigger Type: Time-Based
Repetition Interval: 1 Minute
Execution Context: SYSTEM
```

### Scheduled Payload

```text
C:\Users\Public\A06_3c_SampleApp.exe
```

### Dynamic Analysis Log Artifact

```text
C:\Users\Public\A06_3c_COM_Recurring_Scheduled_Task_SampleApp_log.txt
```

---

#########################################################################

## High-Level COM-Based Recurring Scheduled Task Persistence Flow

1. Initialize COM runtime and security context

2. Instantiate Task Scheduler service via:

   ```text
   CLSID_TaskScheduler
   ```

3. Connect to the local Task Scheduler instance

4. Access the root task folder:

   ```text
   \\
   ```

5. Remove any existing task:

   ```text
   A06_3c_RecurTask
   ```

6. Create a new task definition object

7. Configure registration metadata:

   * Author
   * Description

8. Configure execution principal:

   * User: SYSTEM
   * Logon type: Service account
   * Run level: Highest privileges

9. Define time-based trigger:

   * Trigger type: TASK_TRIGGER_TIME
   * Start boundary: Current time + 1 minute
   * Repetition interval: PT1M (1 minute)

10. Define execution action:

```text
C:\Users\Public\A06_3c_SampleApp.exe
```

11. Configure task settings:

* Hidden task
* Execution allowed on battery power

12. Register task with Task Scheduler service

13. Wait for scheduled trigger activation

14. Task Scheduler launches payload automatically

15. Payload writes timestamped execution logs

16. Execution repeats every minute

17. Persistence survives:

* Reboots
* User logoff
* Service restarts

18. Continues indefinitely until task is removed



