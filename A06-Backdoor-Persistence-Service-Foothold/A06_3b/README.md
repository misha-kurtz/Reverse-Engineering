# A06.3b COM-Based Event-Triggered Task Persistence

## Summary

Creates persistent execution on the Windows analysis VM by
registering a Windows Scheduled Task using the **Task Scheduler 2.0 COM API**,
configured to trigger based on a **specific system event condition**.

The specimen demonstrates event-driven scheduled-task persistence commonly
associated with advanced malware, backdoors, and stealth persistence mechanisms
that avoid predictable execution patterns such as logon or time-based triggers.

Unlike logon-triggered or recurring tasks, this sample leverages the Windows
Event Log subsystem to define a trigger condition using an XML query.
The task is configured to execute when a **process creation event (Event ID 4688)**
matches a specific target:

```text
C:\Program Files\Mozilla Firefox\firefox.exe
```

The scheduled task launches the following payload:

```text
C:\Users\Public\A06_3b_SampleApp.exe
```

The task executes under the **SYSTEM account**, providing elevated execution
context while remaining conditionally triggered by system activity.

This technique demonstrates how attackers can achieve **highly targeted and
low-noise persistence execution** tied to user behavior.

The persistence payload itself is intentionally benign and exists only to
generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of:

```text
A06_3b_SampleApp.exe
```

after the configured event trigger condition is satisfied.

When executed, the payload application:

* Creates a persistent beacon-style logging loop
* Runs continuously after event-triggered execution
* Writes periodic timestamped status messages to:

```text
C:\Users\Public\A06_3b_COM_Event_Triggered_Scheduled_Task_SampleApp_log.txt
```

The log entries confirm:

* Successful event-triggered execution
* COM-based scheduled task persistence activation
* Continued execution over time
* Trigger-based persistence behavior

Example log messages include:

```text
SampleApp initialized successfully via A06_3b COM Event-Triggered (Firefox Launch) Scheduled Task.
Beacon: A06_3b COM Event-Triggered Task process is alive and looping.
```

No additional payloads are downloaded, injected, staged, or executed.

---

## To Execute A06_3b_COM_event_persist

### Register the Scheduled Task

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_3b\bin\A06_3b_COM_event_persist.exe
```

### Verify Task Registration

```powershell
schtasks /query /tn "A06_3b_EventTriggerTask"
```

### Trigger Persistence

Launch Firefox on the Windows VM:

```powershell
"C:\Program Files\Mozilla Firefox\firefox.exe"
```

This generates a **Security Event ID 4688 (Process Creation)**, which satisfies
the task’s event trigger condition and causes execution of the payload.

---

## Expected Persistence Artifacts

### Scheduled Task Registration

```text
Task Name: A06_3b_EventTriggerTask
Trigger Type: Event Trigger (Event ID 4688)
Execution Context: SYSTEM
Implementation: Task Scheduler COM API
```

### Event Trigger Condition

```text
Event Log: Security
Event ID: 4688 (Process Creation)
Filter: NewProcessName = C:\Program Files\Mozilla Firefox\firefox.exe
```

### Scheduled Payload

```text
C:\Users\Public\A06_3b_SampleApp.exe
```

### Dynamic Analysis Log Artifact

```text
C:\Users\Public\A06_3b_COM_Event_Triggered_Scheduled_Task_SampleApp_log.txt
```

---

#########################################################################

# High-Level COM Event-Triggered Scheduled Task Persistence Flow

1. Initialize COM library and Task Scheduler interface

2. Instantiate Task Scheduler service via COM

3. Connect to local Task Scheduler instance

4. Open root task folder (`\`)

5. Remove any existing task with the same name

6. Create a new task definition

7. Configure registration metadata (author, description)

8. Create an **event trigger**:

   ```text
   TASK_TRIGGER_EVENT
   ```

9. Define XML-based event subscription:

   * Event Log: Security
   * Event ID: 4688 (process creation)
   * Filter: firefox.exe execution

10. Configure execution action:

```text
C:\Users\Public\A06_3b_SampleApp.exe
```

11. Configure execution context:

* Run as SYSTEM
* Service account logon type

12. Register task with Task Scheduler

13. Exit installer process

14. Wait for matching system event

15. User launches Firefox

16. Windows logs Event ID 4688

17. Task Scheduler evaluates event subscription

18. Trigger condition is satisfied

19. Scheduled task executes payload

20. Payload begins persistent beacon loop

21. Write execution telemetry to:

```text
C:\Users\Public\A06_3b_COM_Event_Triggered_Scheduled_Task_SampleApp_log.txt
```

22. Continue execution until terminated

