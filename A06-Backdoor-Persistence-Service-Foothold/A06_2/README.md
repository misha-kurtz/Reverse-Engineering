# A06_2 Service-Based Persistence

## Summary

Creates persistent execution on the Windows analysis VM by
registering and running a custom Windows service named:

```text
A06_2_Persistent_Service
```

The specimen demonstrates classic Windows service persistence
commonly associated with backdoors, remote access trojans,
loaders, and long-term foothold malware that requires durable
background execution across system reboots and user logon events.

Unlike simple Run-key persistence, this sample leverages the
Windows Service Control Manager (SCM) to establish persistence
through a managed background service process capable of surviving
user logoff events and automatically starting with the operating system.

The service additionally monitors for active user logon sessions
and launches a secondary user-context application inside the
interactive desktop session using impersonation and token-handling APIs.

The service launches the following user-session payload:

```text
C:\Users\Public\A06_2_SampleApp.exe
```

This demonstrates a common persistence pattern in which a privileged
background service acts as a watchdog, launcher, broker, or session
bridge capable of spawning user-interactive payloads from a SYSTEM-level
service context.

The persistence payload itself is intentionally benign and exists
only to generate controlled forensic artifacts for dynamic analysis.

---

## Payload Summary

The payload behavior is the execution of the user-session application:

```text
A06_2_SampleApp.exe
```

The sample application is launched from the persistent service
after an interactive user logon session becomes available.

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

No additional payloads are downloaded, injected, staged, or executed.

---

## To Execute A06_2_persistent_service

### Install the Service

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_2\bin\A06_2_persistent_service.exe install
```

### Start the Service

```powershell
Start-Service A06_2_Persistent_Service
```

### Stop the Service

```powershell
Stop-Service A06_2_Persistent_Service
```

### Remove the Service

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_2\bin\A06_2_persistent_service.exe uninstall
```

---

## Expected Persistence Artifacts

### Registered Windows Service

```text
Service Name: A06_2_Persistent_Service
Startup Type: Automatic
```

### Service Payload

```text
C:\Users\Public\A06_2_SampleApp.exe
```

### Dynamic Analysis Log Artifact

```text
C:\Users\Public\A06_2_Persistent_Service_SampleApp_log.txt
```

---

#########################################################################

# High-Level Service Persistence Flow

1. Register a Windows service with the Service Control Manager (SCM)

2. Configure the service name:

   ```text
   A06_2_Persistent_Service
   ```

3. Configure automatic startup persistence behavior

4. Start the service process under the Windows service framework

5. Initialize service control dispatcher and status handlers

6. Transition the service into a running state

7. Monitor for active user logon sessions

8. Detect logged-in users through session enumeration mechanisms

9. Obtain the active user security token

10. Impersonate the logged-on user context

11. Create a user-session process using:

    ```text
    CreateProcessAsUserW()
    ```

12. Launch:

    ```text
    C:\Users\Public\A06_2_SampleApp.exe
    ```

13. Execute the payload application inside the interactive desktop session

14. Begin persistent background beacon loop inside the payload application

15. Write periodic timestamped execution logs to:

    ```text
    C:\Users\Public\A06_2_Persistent_Service_SampleApp_log.txt
    ```

16. Continue monitoring and maintaining payload execution

17. Respond to SCM stop and shutdown control events

18. Clean up service handles, process handles, and impersonation state

19. Exit cleanly when the service is stopped
