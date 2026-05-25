# A06_4 WMI Event Subscription Persistence

## Summary

Creates persistent execution on the Windows analysis VM by
registering a permanent Windows Management Instrumentation (WMI)
event subscription that automatically launches a payload when a
configured system event condition is satisfied.

The specimen demonstrates WMI-based persistence commonly associated
with advanced backdoors, remote access trojans, loaders, and stealthy
long-term foothold malware that attempts to avoid traditional startup
artifacts such as registry Run-keys, services, or visible scheduled tasks.

The sample leverages the WMI permanent event subscription framework
through the creation of:

* An event filter
* An event consumer
* A filter-to-consumer binding

Together, these components create a persistent event-driven execution
chain managed internally by the WMI infrastructure.

When the configured trigger condition occurs, WMI automatically launches
the following payload executable:

```text id="xt6r8q"
C:\Users\Public\A06_4_marker.exe
```

This demonstrates a persistence pattern frequently used for stealth,
delayed execution, event-driven activation, and reduced visibility
within traditional startup persistence monitoring tools.

The persistence payload itself is intentionally benign and exists
only to generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of:

```text id="q2f6hy"
A06_4_marker.exe
```

after the configured WMI event trigger condition is satisfied.

When executed, the marker application:

* Collects execution metadata
* Determines its own process path
* Captures execution timestamp information
* Writes a persistence confirmation artifact to:

```text id="g7m4pk"
C:\Users\Public\A06_4_WMI_PERSIST_OK.txt
```

The artifact contains:

* Timestamp
* Process ID
* Process path
* WMI execution confirmation marker

Example artifact contents include:

```text id="h2t7mv"
THESIS_A06_4_WMI_TRIGGERED
Timestamp: 2026-05-25 18:42:10
PID: 1234
ProcessPath: C:\Users\Public\A06_4_marker.exe
```

The marker additionally emits a debug confirmation string using:

```text id="d1x8sq"
OutputDebugStringA()
```

No additional payloads are downloaded, injected, staged, or executed.

---

## To Execute A06_4_wmi_persistence

### Register the WMI Persistence Components

```powershell id="m4t2qj"
.\A06-Backdoor-Persistence-Service-Foothold\A06_4\bin\A06_4_wmi_persistence.exe
```

### Verify WMI Subscription Components

```powershell id="v8s0fp"
Get-WmiObject -Namespace root\subscription -Class __EventFilter
```

```powershell id="n5d4uk"
Get-WmiObject -Namespace root\subscription -Class CommandLineEventConsumer
```

```powershell id="r1w9ec"
Get-WmiObject -Namespace root\subscription -Class __FilterToConsumerBinding
```

### Trigger Persistence

Trigger the configured WMI event condition based on the implementation
logic used within the sample.

---

## Expected Persistence Artifacts

### WMI Permanent Event Subscription Components

```text id="k3v7an"
Namespace: root\subscription
Components:
- __EventFilter
- CommandLineEventConsumer
- __FilterToConsumerBinding
```

### WMI Payload

```text id="u5h9zr"
C:\Users\Public\A06_4_marker.exe
```

### Dynamic Analysis Artifact

```text id="c6p2ls"
C:\Users\Public\A06_4_WMI_PERSIST_OK.txt
```

---

#########################################################################

# High-Level WMI Event Subscription Persistence Flow

1. Initialize the WMI persistence workflow

2. Connect to the WMI management infrastructure

3. Access the WMI subscription namespace:

   ```text
   root\subscription
   ```

4. Create a permanent WMI event filter object:

   ```text
   __EventFilter
   ```

5. Configure the WMI event query and trigger condition

6. Create a WMI command execution consumer:

   ```text
   CommandLineEventConsumer
   ```

7. Configure the payload executable path:

   ```text
   C:\Users\Public\A06_4_marker.exe
   ```

8. Create a filter-to-consumer binding:

   ```text
   __FilterToConsumerBinding
   ```

9. Associate the event filter with the execution consumer

10. Register all persistence components within the WMI repository

11. Exit the installer process after successful WMI registration

12. Wait for the configured WMI trigger condition

13. Detect the subscribed event through the WMI event subsystem

14. Automatically launch:

    ```text
    C:\Users\Public\A06_4_marker.exe
    ```

15. Execute the payload application under the WMI event execution context

16. Collect execution metadata:

    * PID
    * Timestamp
    * Process path

17. Write persistence confirmation artifact to:

    ```text
    C:\Users\Public\A06_4_WMI_PERSIST_OK.txt
    ```

18. Emit a debug confirmation string using:

    ```text
    OutputDebugStringA()
    ```

19. Preserve persistence across:

    * User logoff/logon cycles
    * System reboots
    * WMI service restarts

20. Maintain dormant behavior until the monitored WMI event reoccurs

21. Exit cleanly after artifact generation
