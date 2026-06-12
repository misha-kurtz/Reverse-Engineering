# A06_4 WMI Event Subscription Persistence

## Summary

Creates persistent execution on the Windows analysis VM by
registering a permanent Windows Management Instrumentation (WMI)
event subscription.

The specimen demonstrates event-triggered persistence commonly
associated with advanced malware, backdoors, and long-term
foothold mechanisms that seek to execute payloads without relying
on traditional startup folders, Run keys, services, or scheduled
tasks.

Unlike logon-triggered or time-based persistence techniques,
this sample leverages a permanent WMI event subscription that
monitors a specific registry value and launches a payload when
the configured event condition is satisfied.

The WMI subscription consists of:

* A permanent `__EventFilter`
* A `CommandLineEventConsumer`
* A `__FilterToConsumerBinding`

The event filter monitors:

```text
HKLM\SOFTWARE\A06_4_Test
```

for modifications to the registry value:

```text
Trigger
```

When the monitored value changes, the WMI subsystem launches:

```text
C:\Users\Public\A06_4_marker.exe
```

The persistence payload is intentionally benign and exists only
to generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of:

```text
A06_4_marker.exe
```

when the configured WMI event subscription is triggered.

When executed, the payload application:

* Collects execution metadata
* Records the current timestamp
* Records the executing process identifier (PID)
* Records the executable path
* Writes telemetry to:

```text
C:\Users\Public\A06_4_WMI_PERSIST_OK.txt
```

Example output:

```text
THESIS_A06_4_WMI_TRIGGERED

Timestamp: 2026-06-12 14:00:00
PID: 4321
ProcessPath: C:\Users\Public\A06_4_marker.exe
```

The payload additionally emits:

```text
THESIS_A06_4_WMI_TRIGGERED
```

through:

```text
OutputDebugStringA()
```

No additional payloads are downloaded, injected, or executed.

---

## To Execute A06_4_wmi_persistence

### Register the WMI Subscription

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_4\bin\A06_4_wmi_persistence.exe
```

During installation the specimen will:

1. Create the registry test key

```text
HKLM\SOFTWARE\A06_4_Test
```

2. Initialize:

```text
Trigger = INITIAL
```

3. Create the WMI Event Filter

4. Create the CommandLineEventConsumer

5. Create the Filter-To-Consumer Binding

6. Automatically modify:

```text
Trigger = A06_4_FIRE
```

to validate the persistence mechanism.

---

### Verify Persistence Artifacts

Open PowerShell as Administrator and inspect the registered
WMI objects:

#### Event Filter

```powershell
Get-WmiObject -Namespace root\subscription -Class __EventFilter
```

#### Event Consumer

```powershell
Get-WmiObject -Namespace root\subscription -Class CommandLineEventConsumer
```

#### Filter Binding

```powershell
Get-WmiObject -Namespace root\subscription -Class __FilterToConsumerBinding
```

Expected object names:

```text
A06_4_RegistryFilter
A06_4_RegistryConsumer
```

---

### Verify Payload Execution

Inspect:

```text
C:\Users\Public\A06_4_WMI_PERSIST_OK.txt
```

Successful execution indicates that the WMI event was received
and the payload launched correctly.

---

## Cleanup

The specimen supports automated cleanup.

Execute:

```powershell
.\A06_4_wmi_persistence.exe /cleanup
```

or:

```powershell
.\A06_4_wmi_persistence.exe -u
```

Cleanup removes:

* `__FilterToConsumerBinding`
* `__EventFilter`
* `CommandLineEventConsumer`
* Registry test key:

```text
HKLM\SOFTWARE\A06_4_Test
```

---

## Expected Persistence Artifacts

### WMI Event Filter

```text
Name: A06_4_RegistryFilter
Class: __EventFilter
Namespace: root\subscription
```

### WMI Event Consumer

```text
Name: A06_4_RegistryConsumer
Class: CommandLineEventConsumer
```

### WMI Binding

```text
Class: __FilterToConsumerBinding
```

### Registry Trigger Location

```text
HKLM\SOFTWARE\A06_4_Test
```

### Registry Value

```text
Trigger
```

### Payload Executable

```text
C:\Users\Public\A06_4_marker.exe
```

### Dynamic Analysis Artifact

```text
C:\Users\Public\A06_4_WMI_PERSIST_OK.txt
```

---

#########################################################################

## High-Level WMI Event Subscription Persistence Flow

1. Connect to:

   ```text
   root\subscription
   ```

2. Create permanent WMI Event Filter:

   ```text
   A06_4_RegistryFilter
   ```

3. Configure WQL query:

   ```text
   SELECT * FROM RegistryValueChangeEvent
   WHERE Hive='HKEY_LOCAL_MACHINE'
   AND KeyPath='SOFTWARE\\A06_4_Test'
   AND ValueName='Trigger'
   ```

4. Create CommandLineEventConsumer:

   ```text
   A06_4_RegistryConsumer
   ```

5. Configure consumer action:

   ```text
   C:\Users\Public\A06_4_marker.exe
   ```

6. Create:

   ```text
   __FilterToConsumerBinding
   ```

7. Persistence registration completes

8. WMI stores subscription objects

9. Registry value changes:

   ```text
   HKLM\SOFTWARE\A06_4_Test\Trigger
   ```

10. WMI detects RegistryValueChangeEvent

11. Event Filter evaluates the trigger condition

12. Bound CommandLineEventConsumer activates

13. WMI launches:

```text
C:\Users\Public\A06_4_marker.exe
```

14. Payload executes

15. Marker telemetry is written to disk

16. Persistence remains active

17. Future registry modifications trigger additional executions

18. Persistence survives:

* Reboots
* User logoff
* User logon
* Service restarts

19. Continues until subscription objects are removed

