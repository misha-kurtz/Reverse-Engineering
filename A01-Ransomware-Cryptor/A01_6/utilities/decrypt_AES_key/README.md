## Recover the AES Key Using the Laboratory Decryption Utility

After the ransomware sample has executed, the `aesencrypted`
parameter captured from the HTTP POST request can be decrypted using the
laboratory AES-key recovery utility.

The utility performs the complete recovery workflow automatically:

* Reads the RSA private key
* Reads the captured `aesencrypted` value
* URL-decodes the POSTed value
* Base64-decodes the ciphertext
* Performs RSA-2048 decryption using PKCS#1 v1.5 padding
* Recovers the original 32-character AES key
* Writes the recovered key to disk

---

### 1. — Place the Required Files

Copy the following files into the utility directory:

```text
A01_4_private_key.xml
posted_aesencrypted.txt
```

The utility expects the matching RSA private key generated during the INetSim configuration process and the captured `aesencrypted` POST parameter recovered from either:

* the INetSim HTTP logs
* a packet capture
* another HTTP capture mechanism

---

### 2. — Utility Directory

The laboratory utility is located at:

A01-Ransomware-Cryptor/
└── A01_4/
    ├── bin/
    ├── utilities/
    │   ├── gen_RSA_keypair/
    │   │   ├── gen_RSA_keypair.cs
    |   │   ├── gen_RSA_keypair.csproj
    │   |   └── ...
    │   └── decrypt_AES_key/
    │       ├── decrypt_AES_key.cs
    │       ├── decrypt_AES_key.csproj
    │       ├── posted_aesencrypted.txt
    │       ├── A01_4_private_key.xml
    │       └── A01_4_Recovered_AES_Key.txt
    └── README.md

The `decrypt_AES_key` directory should contain:

```text
A01_4_private_key.xml
posted_aesencrypted.txt
Program.cs
```

---

### 3. — Execute the Utility

Compile and execute the utility:

```powershell
dotnet run
```

The utility automatically:

1. Loads the RSA private key.
2. Reads the captured POST value.
3. URL-decodes the value.
4. Removes whitespace.
5. Base64-decodes the ciphertext.
6. Performs RSA decryption.
7. Recovers the original AES key.
8. Writes the recovered key

---

### 4. Expected Console Output

```text
Base64 length: 344
Ciphertext bytes: 256
Recovered AES key: zH8rL1qWbP4Y2aM9vN6tKsQeR7xDfUcA
```

---

### 5. Generated Output

The recovered AES key is written to:

```text
A01_4_Recovered_AES_Key.txt
```

---

### Validation

The recovered AES key should exactly match the laboratory key generated during execution:

```text
C:\Users\Public\A01_4_Lab_Encryption_Key.txt
```

Matching values confirm the complete laboratory workflow:

```text
Generate AES key
        │
        ▼
Encrypt files
        │
        ▼
Encrypt AES key using RSA public key
        │
        ▼
Transmit POST request
        │
        ▼
Capture aesencrypted parameter
        │
        ▼
Run laboratory recovery utility
        │
        ▼
Recover original AES key
        │
        ▼
Compare with laboratory key file
```

---