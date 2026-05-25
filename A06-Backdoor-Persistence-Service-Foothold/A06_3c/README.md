# A06_3c COM API Scheduled Task Persistence

## Summary

Creates persistent execution on the Windows analysis VM by
registering a scheduled task through the modern Windows Task Scheduler
2.0 COM API interface rather than relying on legacy command-line task
registration utilities.

The specimen demonstrates COM-based scheduled-task persistence
commonly associated with advanced backdoors, loaders, remote access
trojans, and long-term foothold malware that interacts directly with
Windows management infrastructure through native COM interfaces.

Unlike basic `schtasks.exe` usage, this sample communicates directly
with the Task Scheduler service using COM object instantiation and
Task Scheduler 2.0 interfaces, closely mirroring how modern malware
and administrative tooling programmatically create persistence tasks.

The scheduled task launches the following payload:

```text id="9f2w7j"
C:\Users\Public\A06_3c_SampleApp.exe
```

This demonstrates a persistence pattern commonly used to avoid
simple command-line telemetry, blend into legitimate Windows
administrative behavior, and interact directly with the underlying
Task Scheduler COM framework.

The persistence payload itself is intentionally benign and exists
only to generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of:

```text id="3c8m4s"
A06_3c_SampleApp.exe
```

after the COM-registered scheduled task is triggered.

When executed, the payload application:

* Creates a continuous beacon-style telemetry loop
* Runs persistently in the background
* Writes periodic timestamped execution records to:

```text id="h6q4dr"
C:\Users\Public\A06_3c_COM_API_Scheduled_Task_SampleApp_log.txt
```

The log entries confirm:

* Successful COM-based task execution
* Task Scheduler 2.0 activation
* SYSTEM or administrative execution context
* Continued persistence activity over time

Example log entries include:

```text id="e8v2yp"
[A06_3c] Modern Task Scheduler 2.0 COM API Task execution active. Context: SYSTEM/Admin.
```

No additional payloads are downloaded, injected, staged, or executed.

---

## To Execute A06_3c_com_api_task

### Register the COM-Based Scheduled Task

```powershell id="g0s7ja"
.\A06-Backdoor-Persistence-Service-Foothold\A06_3c\bin\A06_3c_com_api_task.exe
```

### Verify Task Registration

```powershell id="2h53du"
schtasks /query /tn "A06_3c_COM_API_Task"
```

### Trigger Persistence

Trigger the configured task condition or wait for the configured
schedule interval depending on the test configuration.

---

## Expected Persistence Artifacts

### Scheduled Task Registration

```text id="r1e4tz"
Task Name: A06_3c_COM_API_Task
Registration Method: Task Scheduler 2.0 COM API
```

### Scheduled Payload

```text id="v2dk7q"
C:\Users\Public\A06_3c_SampleApp.exe
```

### Dynamic Analysis Log Artifact

```text id="s6uj4h"
C:\Users\Public\A06_3c_COM_API_Scheduled_Task_SampleApp_log.txt
```

---

#########################################################################

# High-Level COM API Scheduled Task Persistence Flow

1. Initialize the COM subsystem using:

   ```text
   CoInitializeEx()
   ```

2. Instantiate the Task Scheduler COM service interfaces

3. Connect to the Windows Task Scheduler service through COM

4. Create a new task definition object

5. Configure task registration metadata:

   * Task name
   * Author information
   * Execution settings
   * Trigger configuration

6. Configure the payload executable path:

   ```text
   C:\Users\Public\A06_3c_SampleApp.exe
   ```

7. Create a scheduled task action object

8. Associate the payload executable with the task action

9. Configure the trigger type:

   * Time-based trigger
   * Logon trigger
   * Immediate test trigger
   * Administrative execution context

10. Register the task definition with the Task Scheduler service
    through COM interfaces

11. Store the task within the Windows scheduled-task subsystem

12. Exit the installer process after successful registration

13. Wait for the configured trigger condition

14. Trigger automatic task execution through Task Scheduler 2.0

15. Launch:

    ```text
    C:\Users\Public\A06_3c_SampleApp.exe
    ```

16. Execute the payload application under the configured execution context

17. Begin continuous beacon-style telemetry loop

18. Write periodic timestamped execution logs to:

    ```text
    C:\Users\Public\A06_3c_COM_API_Scheduled_Task_SampleApp_log.txt
    ```

19. Continue execution indefinitely until terminated

20. Preserve persistence across:

    * User logoff/logon cycles
    * System reboots
    * Task Scheduler service restarts

21. Clean up COM objects and exit cleanly after registration
