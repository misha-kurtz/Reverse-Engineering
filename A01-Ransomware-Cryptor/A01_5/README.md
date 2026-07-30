# A01.5 `C++` EDA2 Variant: Double Extortion

## Summary

A01_5 demonstrates a controlled double-extortion ransomware workflow inspired by EDA2.

Unlike A01_4, this specimen **does not exfiltrate the encryption key**. Instead, it:

1. Generates a random AES-256 encryption key.
2. Copies selected user documents into a staging directory.
3. Compresses the staged files into a 7-Zip archive.
4. Uploads the archive to the simulated C2 server.
5. Encrypts the original files.
6. Downloads a ransom image.
7. Sets the desktop wallpaper.

The plaintext AES key is written to:

```text
C:\Users\Public\A01_5_Lab_Encryption_Key.txt
```

for laboratory validation only.

---

# Laboratory Setup

**Required Software**

Windows VM

* 7-Zip installed

Default location:

```text
C:\Program Files\7-Zip\7z.exe
```

---

### 1. Configure INetSim

Enable

```text
DNS
HTTP
```

---

### 2. Install HTTP Fake Files

Copy

```text
A01_5_ransomnote.jpg
```

into

```text
/var/lib/inetsim/http/fakefiles/
```

---

### 3. Configure HTTP Mappings in `/etc/inetsim/inetsim.conf`

```text
http_fakemode YES

http_static_fakefile /panel/ransomnote A01_5_ransomnote.jpg image/jpeg

http_static_fakefile /panel/exfil exfil_OK.json application/json
```

---

### 4. Config Exfil Response

Create

```text
exfil_OK.json
```

Contents:

```json
{
  "status":"ok",
  "task":"Archive received"
}
```

---

### 5. Verify Endpoints Prior to Executing Sample

Confirm:

```text
GET  /panel/ransomnote

POST /panel/exfil
```

---

### 6. Execute the Sample

Create:

```text
C:\Users\Public\A01_TestData
```

Populate with representative documents.

Run:

```powershell
.\A01_5_Ransomware_DoubleExtortion.cpp
```

---

## Execution Workflow

```text
Generate AES Key
        │
        ▼
Stage Files
        │
        ▼
Create 7z Archive
        │
        ▼
HTTP Upload Archive
        │
        ▼
Delete Staging Directory
        │
        ▼
Delete Archive
        │
        ▼
Encrypt Original Files
        │
        ▼
Download Ransom Image
        │
        ▼
Set Wallpaper
```

---

## Expected Artifacts

Filesystem

```text
C:\Users\Public\A01_5_Lab_Encryption_Key.txt

C:\Users\Public\A01_5_Staging   (temporary)

C:\Users\Public\A01_5_Exfil.7z  (temporary)

*.locked

C:\Users\<user>\ransom.jpg
```

During successful execution:

* staging directory created
* archive created
* archive uploaded
* staging directory removed
* archive removed
* originals encrypted

---

## Expected Network Activity

DNS

```text
c2.lab.local
```

HTTP

```text
POST /panel/exfil

GET /panel/ransomnote
```

Unlike A01_4, there is **no**:

* public key download
* RSA key exchange
* encrypted AES key POST

The HTTP POST instead contains a multipart/form-data upload of the compressed archive.

---

## Expected Logs

DNS

```text
/var/log/inetsim/dns.log
```

HTTP

```text
/var/log/inetsim/http.log
```

The HTTP log should show:

```text
POST /panel/exfil

GET /panel/ransomnote
```

The upload uses:

```text
Content-Type:
multipart/form-data
```

with the file:

```text
A01_5_Exfil.7z
```

---

## Validation

Confirm:

* staging directory created
* archive created
* archive successfully uploaded
* staging directory removed
* archive removed
* original files renamed to `.locked`
* wallpaper changed
* AES key written to:

```text
C:\Users\Public\A01_5_Lab_Encryption_Key.txt
```

---

# Recovering the Exfiltrated Archive

The uploaded archive is stored by INetSim as the raw HTTP POST body, including the multipart/form-data headers and trailing MIME boundary.

The following procedure demonstrates how to recover the original archive from the captured POST data for laboratory validation.

### 1. Locate the Uploaded POST Body

Uploaded HTTP POST requests are stored in:

```text
/var/lib/inetsim/http/postdata/
```

Identify the file corresponding to the `/panel/exfil` request.

---

### 2. Locate the Beginning of the 7-Zip Archive

Search the POST body for the 7-Zip file signature:

```bash
# Carve archive out of POST body

# Find archive offset
sudo grep -abo $'\x37\x7a\xbc\xaf\x27\x1c' \
/var/lib/inetsim/http/postdata/1f102f1e20dea4ec06030d6d1510866ecee760927a610d71eed26169a0504921
```

Example output:

```text
146:7z...
```

The reported offset marks the beginning of the embedded archive.

---

### 3. Extract the Embedded Archive

Use the reported offset with `dd` to carve the archive from the POST body.

```bash
sudo dd \
if=/var/lib/inetsim/http/postdata/1f102f1e20dea4ec06030d6d1510866ecee760927a610d71eed26169a0504921 \
of=A01_5_Exfil.7z \
bs=1 \
skip=146
```

Example:

```text
3525894+0 records in
3525894+0 records out
3525894 bytes (3.5 MB, 3.4 MiB) copied
```

---

### 4. Verify the Carved Archive

Confirm that the archive was successfully recovered.

```bash
7z t A01_5_Exfil.7z
7z l A01_5_Exfil.7z
```

Because the carved file still contains the trailing multipart MIME boundary, extraction may produce warnings while still successfully recovering the archived files.

---

### 5. Rebuild the Archive

Extract the recovered files and create a clean 7-Zip archive without the trailing multipart data.

```bash
7z x A01_5_Exfil.7z -oextracted_clean

cd extracted_clean

7z a ../A01_5_Exfil_clean.7z *

ls
```

The resulting archive contains only the exfiltrated files and can be retained as a laboratory artifact.

---

### 6. Laboratory Cleanup

After each dynamic analysis run, remove the recovered artifacts and clear the stored POST data.

```bash
# Delete recovered artifacts
sudo rm -rf <extracted files> <original_archive> <clean_archive>

# Clear captured POST bodies
sudo find /var/lib/inetsim/http/postdata \
    -mindepth 1 \
    -maxdepth 1 \
    -type f \
    -delete
```

---
**Purpose**

This specimen demonstrates the double extortion ransomware model commonly observed in modern ransomware families. Rather than transmitting the encryption key to a remote server, the malware first stages and exfiltrates selected victim documents before encrypting the originals.

The sample is intended to generate controlled static and dynamic analysis artifacts that illustrate the complete data theft and encryption workflow while remaining suitable for safe execution in a laboratory environment.
