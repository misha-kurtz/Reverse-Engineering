# A06_3b Event-Triggered Scheduled Task Persistence

## Summary

Creates persistent execution on the Windows analysis VM by
registering a Windows Scheduled Task configured to trigger
when a specific system event occurs.

The specimen demonstrates event-driven scheduled-task persistence
commonly associated with backdoors, loaders, remote access trojans,
and long-term foothold malware that avoids relying solely on simple
startup triggers such as Run-keys or standard logon execution.

Unlike traditional scheduled-task persistence that executes at boot
or user logon, this sample leverages Task Scheduler event subscriptions
to launch a payload only after a targeted behavioral condition occurs.

In this controlled specimen, the scheduled task is configured to
trigger when Mozilla Firefox is launched, simulating malware that
activates only after specific user activity or environmental conditions
are observed.

The scheduled task launches the following payload:

```text id="tv7qhl"
C:\Users\Public\A06_3b_SampleApp.exe
```

This demonstrates a persistence pattern commonly used to reduce
visibility, delay execution, evade automated analysis, or synchronize
malicious activity with expected user behavior.

The persistence payload itself is intentionally benign and exists
only to generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of:

```text id="j3q11x"
A06_3b_SampleApp.exe
```

after the configured event-trigger condition is satisfied.

In this sample, the trigger condition is the launch of Firefox,
which causes the Windows Task Scheduler service to automatically
execute the payload application.

When executed, the payload application:

* Creates a persistent beacon-style logging loop
* Runs continuously within the interactive user session
* Writes periodic timestamped status messages to:

```text id="72aj6z"
C:\Users\Public\A06_3b_Scheduled_Task_SampleApp_log.txt
```

The log entries confirm:

* Successful event-triggered execution
* Task Scheduler activation
* User-session payload launch
* Continued execution over time

Example log messages include:

```text id="mjlwm7"
SampleApp initialized successfully via A06_3b Event-Triggered (Firefox Launch) Scheduled Task.
Beacon: A06_3b Event-Triggered Task process is alive and looping.
```

No additional payloads are downloaded, injected, staged, or executed.

---

## To Execute A06_3b_event_trigger_schtask_persist

### Register the Scheduled Task

```powershell id="z5bdha"
.\A06-Backdoor-Persistence-Service-Foothold\A06_3b\bin\A06_3b_event_trigger_schtask_persist.exe
```

### Verify Scheduled Task Registration

```powershell id="e7mmlk"
schtasks /query /tn "A06_3b_EventTriggeredPersistence"
```

### Trigger Persistence

Launch Mozilla Firefox on the Windows VM to trigger the scheduled task.

---

## Expected Persistence Artifacts

### Scheduled Task Registration

```text id="3vr9d8"
Task Name: A06_3b_EventTriggeredPersistence
Trigger Type: Event-Based Trigger
Activation Event: Firefox Launch
```

### Scheduled Payload

```text id="7g6i7y"
C:\Users\Public\A06_3b_SampleApp.exe
```

### Dynamic Analysis Log Artifact

```text id="k9j4zs"
C:\Users\Public\A06_3b_Scheduled_Task_SampleApp_log.txt
```

---

#########################################################################

# High-Level Event-Triggered Scheduled Task Persistence Flow

1. Initialize Task Scheduler persistence workflow

2. Configure the scheduled task name:

   ```text
   A06_3b_EventTriggeredPersistence
   ```

3. Configure the payload executable path:

   ```text
   C:\Users\Public\A06_3b_SampleApp.exe
   ```

4. Configure an event-driven trigger condition based on:

   * Firefox process launch activity
   * Windows event subscription logic
   * Task Scheduler event monitoring

5. Register the scheduled task with the Windows Task Scheduler service

6. Store the task definition and event filter configuration

7. Exit the installer process after successful task registration

8. Wait for the monitored trigger event to occur

9. Detect Firefox launch activity through the configured event trigger

10. Activate the scheduled task automatically

11. Launch:

    ```text
    C:\Users\Public\A06_3b_SampleApp.exe
    ```

12. Execute the payload application inside the interactive desktop session

13. Begin persistent background beacon loop inside the payload application

14. Write periodic timestamped execution logs to:

    ```text
    C:\Users\Public\A06_3b_Scheduled_Task_SampleApp_log.txt
    ```

15. Continue execution indefinitely until terminated

16. Preserve persistence across:

    * User logoff/logon cycles
    * System reboots
    * Task Scheduler service restarts

17. Maintain dormant behavior until the monitored trigger event reoccurs

18. Exit cleanly when the payload application is terminated
