# A05_1b HTTPS Beacon with Command Polling

## Summary

Sends recurring HTTPS beacon check-ins to a controlled lab C2 endpoint
and polls for benign commands.

The sample demonstrates interactive beaconing behavior commonly
associated with malware command-and-control agents, periodic check-ins,
task polling, command dispatch, and result submission.

Unlike `A05_1a`, which only sends simple HTTP beacons, this specimen
adds a command-polling loop over HTTPS. The beacon checks in with the
server, receives controlled lab commands, processes only benign command
types, and submits execution results back to the server.

The controlled C2 endpoint is:

```text
https://c2.lab.local
```

Supported commands are intentionally limited to:

```text
noop
ping
sleep|seconds
```

No shell execution, payload download, persistence, credential theft,
privilege escalation, or destructive functionality is performed.

---

## Beacon Summary

The beacon behavior is implemented as a dedicated thread created with:

```text
CreateThread()
```

The thread repeatedly:

* Sends a beacon check-in
* Polls for tasking
* Parses returned commands
* Executes benign lab-safe command logic
* Sends command results back to the server
* Sleeps for the configured beacon interval

The sample performs an initial connectivity test against:

```text
/api/ping
```

Then polls for commands using:

```text
/api/checkin
```

Command results are submitted to:

```text
/api/result
```

---

## Supported Commands

### noop

```text
noop
```

Returns:

```text
SUCCESS|NOOP
```

### ping

```text
ping
```

Returns:

```text
SUCCESS|PONG
```

### sleep

```text
sleep|15
```

Updates the beacon interval.

The interval is bounded to:

```text
Minimum: 5 seconds
Maximum: 300 seconds
```

Unsupported commands are ignored and reported as benign lab-unsupported
commands.

---

## To Execute A05_1b

Make sure the analysis VM resolves:

```text
c2.lab.local
```

to the INetSim server IP:

```text
192.168.67.5
```

Then run the HTTPS beacon sample directly:

```powershell
.\A05-Beaconing-Interactive-C2\A05_1b\bin\A05_1b_https_beacon_w_command_polling.exe
```

Successful execution should show:

```text
A05_1b HTTPS beacon initialized
Testing connectivity via /api/ping
Beacon thread started
Sending beacon check-in...
Received <n> command(s)
```

---

## Expected Beacon Artifacts

### HTTPS Requests

Expected controlled endpoints:

```text
GET /api/ping
GET /api/checkin
POST /api/result
```

### Expected Console Output

```text
A05_1b HTTPS beacon initialized
C2 Server: https://c2.lab.local
Testing connectivity via /api/ping
Connectivity test succeeded
Beacon thread created successfully
Beacon thread started
Sending beacon check-in...
Received 1 command(s)
Processing command: noop
Sending result: SUCCESS|NOOP
Sleeping for 15 seconds
```

### Expected Dynamic Signals

Dynamic analysis tools may observe:

* DNS lookup for `c2.lab.local`
* HTTPS traffic to INetSim
* Repeated `/api/checkin` requests
* Result submission to `/api/result`
* Beacon interval changes after `sleep|seconds`
* Dedicated beacon thread creation
* No process injection
* No payload download
* No file creation
* No persistence behavior

---

## INetSim Configuration for A05_1b

This sample expects INetSim HTTPS static responses for ping,
check-in, and result submission endpoints.

---

### Create Fake Response Files

Create the ping response:

```bash
sudo nano /var/lib/inetsim/http/fakefiles/ping.json
```

Example contents:

```json
{"status":"ok","message":"PONG"}
```

Create the check-in response:

```bash
sudo nano /var/lib/inetsim/http/fakefiles/checkin.json
```

Example contents:

```json
{"status":"ok","command":"noop","args":""}
```

Alternative benign command examples:

```json
{"command":"sleep","args":"15"}
{"command":"ping","args":""}
```

Create the result response:

```bash
sudo nano /var/lib/inetsim/http/fakefiles/result.json
```

Example contents:

```json
{"status":"ok","message":"result received"}
```

---

### Set Ownership

```bash
sudo chown inetsim:inetsim /var/lib/inetsim/http/fakefiles/ping.json
sudo chown inetsim:inetsim /var/lib/inetsim/http/fakefiles/checkin.json
sudo chown inetsim:inetsim /var/lib/inetsim/http/fakefiles/result.json
```

---

### Configure HTTPS Static Fakefile Mappings

Edit:

```text
/etc/inetsim/inetsim.conf
```

Add:

```text
https_static_fakefile /api/ping ping.json application/json
https_static_fakefile /api/checkin checkin.json application/json
https_static_fakefile /api/result result.json application/json
```

---

### Restart INetSim

```bash
sudo systemctl restart inetsim
```

---

### Expected INetSim Log Activity

Successful command polling should generate entries similar to:

```text
[https_443_tcp] recv: GET /api/ping HTTP/1.1
[https_443_tcp] recv: GET /api/checkin HTTP/1.1
[https_443_tcp] recv: POST /api/result HTTP/1.1
[https_443_tcp] send: HTTP/1.1 200 OK
```

---

#########################################################################

# High-Level HTTPS Beacon Command-Polling Flow

1. Start the HTTPS beacon sample:

   ```text
   A05_1b_https_beacon_w_command_polling.exe
   ```

2. Configure the C2 server URL:

   ```text
   https://c2.lab.local
   ```

3. Initialize the `C2Client`

4. Test connectivity using:

   ```text
   /api/ping
   ```

5. Create the beacon thread using:

   ```text
   CreateThread()
   ```

6. Mark the C2 client active

7. Send a beacon check-in using:

   ```text
   /api/checkin
   ```

8. Receive zero or more command strings

9. If no command is returned, submit:

   ```text
   CHECKIN_OK|No task returned
   ```

10. Parse command strings using the format:

    ```text
    command|arguments
    ```

11. Process supported benign commands:

    ```text
    noop
    ping
    sleep|seconds
    ```

12. Clamp sleep interval values between 5 and 300 seconds

13. Submit command results to:

    ```text
    /api/result
    ```

14. Sleep for the configured beacon interval using:

    ```text
    Sleep()
    ```

15. Repeat while the client remains running

16. Exit cleanly when the beacon loop stops.
