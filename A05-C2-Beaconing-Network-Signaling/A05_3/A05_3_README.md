# A05_3 Protocol Mimicry

## Summary

Sends repeated HTTP POST requests that mimic legitimate REST-style
desktop application telemetry.

The sample demonstrates protocol mimicry behavior commonly associated
with command-and-control traffic disguised as normal application API
activity, telemetry submission, heartbeat events, and benign-looking
REST client communication.

Unlike simple beacon samples that send obvious check-in URLs or
counter-based DNS queries, this specimen wraps its periodic signaling
inside structured JSON telemetry messages and browser/application-style
HTTP headers.

The underlying transport is:

```text
HTTP POST
```

The mimicked application pattern is:

```text
Desktop telemetry client heartbeat traffic
```

The controlled telemetry endpoint is:

```text
http://api.lab.local/v1/telemetry
```

No payload download, command execution, persistence, credential theft,
or destructive functionality is performed.

---

## Beacon Summary

The beacon behavior is implemented as repeated HTTP POST requests.

Each request sends a JSON body containing:

* Session ID
* Event type
* UTC timestamp
* Application name
* Application version
* Status value

Example payload:

```json
{
  "session_id": "sess-0001",
  "event_type": "heartbeat",
  "timestamp": "2026-05-25T14:10:33.0000000Z",
  "application": "TelemetryClient",
  "version": "1.4.2",
  "status": "ok"
}
```

The sample uses:

* Iterations: `10`
* Sleep interval: `15000` milliseconds
* Content type: `application/json`
* HTTP method: `POST`

---

## Mimicked Headers

The sample configures browser/app-style headers to make the traffic
look like normal telemetry from a desktop client.

```text
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) TelemetryClient/1.4.2
Accept: application/json
Accept-Language: en-US,en;q=0.9
X-Client-Version: 1.4.2
X-Request-Origin: desktop-client
```

These headers are intended to produce clear static and dynamic
indicators of protocol mimicry for analysis.

---

## To Execute A05_3

Make sure the analysis VM resolves:

```text
api.lab.local
```

to the INetSim server IP:

```text
192.168.67.5
```

Then run the protocol mimicry sample directly:

```powershell
.\A05-Beaconing-Interactive-C2\A05_3\bin\A05_3_protocol_mimicry.exe
```

Successful execution should show repeated HTTP POST requests followed
by:

```text
[*] Finished.
```

---

## Expected Beacon Artifacts

### HTTP Requests

Expected endpoint:

```text
POST /v1/telemetry HTTP/1.1
Host: api.lab.local
Content-Type: application/json
```

### Example Request Body

```json
{
  "session_id": "sess-0001",
  "event_type": "heartbeat",
  "timestamp": "2026-05-25T14:10:33.0000000Z",
  "application": "TelemetryClient",
  "version": "1.4.2",
  "status": "ok"
}
```

### Expected Console Output

```text
[*] Starting A05_3_protocol_mimicry sample...
[*] Target URL: http://api.lab.local/v1/telemetry
[*] Iterations: 10
[*] Sleep: 15000 ms
[*] Sent request 1 | HTTP 200 OK
[*] Sent request 2 | HTTP 200 OK
[*] Finished.
```

### Expected Dynamic Signals

Dynamic analysis tools may observe:

* DNS lookup for `api.lab.local`
* Repeated HTTP POST requests to `/v1/telemetry`
* JSON request bodies
* Application-style telemetry fields
* Browser/app-style headers
* Fixed 15-second interval between posts
* No payload download
* No child process creation
* No persistence behavior

---

## INetSim Configuration for A05_3

This sample expects INetSim to serve a controlled JSON response for:

```text
/v1/telemetry
```

over HTTP.

---

### Create Fake Telemetry Response File

```bash
sudo nano /var/lib/inetsim/http/fakefiles/a05_3_telemetry.json
```

Example contents:

```json
{"status":"ok","message":"A05_3_TELEMETRY_ACK"}
```

---

### Set Ownership

```bash
sudo chown inetsim:inetsim /var/lib/inetsim/http/fakefiles/a05_3_telemetry.json
```

---

### Configure HTTP Static Fakefile Mapping

Edit:

```text
/etc/inetsim/inetsim.conf
```

Add:

```text
http_static_fakefile /v1/telemetry a05_3_telemetry.json application/json
```

---

### Restart INetSim

```bash
sudo systemctl restart inetsim
```

---

### Expected INetSim Log Activity

Successful execution should generate HTTP entries in:

```text
/var/log/inetsim/service.log
```

Example:

```text
[http_80_tcp] recv: POST /v1/telemetry HTTP/1.1
[http_80_tcp] send: HTTP/1.1 200 OK
```

INetSim may also store submitted POST bodies under:

```text
/var/lib/inetsim/http/postdata/
```

These POST bodies provide useful dynamic artifacts showing the generated
telemetry JSON.

---

#########################################################################

# High-Level Protocol Mimicry Flow

1. Start the protocol mimicry sample:

   ```text
   A05_3_protocol_mimicry.exe
   ```

2. Configure the target telemetry endpoint:

   ```text
   http://api.lab.local/v1/telemetry
   ```

3. Configure loop parameters:

   ```text
   iterations = 10
   sleepMs = 15000
   ```

4. Initialize the shared HTTP client:

   ```text
   HttpClient
   ```

5. Configure telemetry-style HTTP headers

6. Build the first JSON heartbeat payload

7. Create the HTTP request body using:

   ```text
   StringContent(..., Encoding.UTF8, "application/json")
   ```

8. Submit the telemetry request using:

   ```text
   HttpClient.PostAsync()
   ```

9. Wait synchronously for the response using:

   ```text
   GetAwaiter().GetResult()
   ```

10. Print the HTTP response status

11. Sleep for:

    ```text
    15000 ms
    ```

12. Repeat the POST request sequence for 10 iterations

13. Exit cleanly after the final telemetry request.
