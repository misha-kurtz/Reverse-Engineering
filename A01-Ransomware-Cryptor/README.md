## A01 Ransomware / Cryptor Class

|ID|Technique|Core Transition|Dominant APIs / Mechanisms|Static Artifacts (Ghidra)|Dynamic Artifacts (Procmon / PCAP / Sysmon)|Semantic Meaning|
|---|---|---|---|---|---|---|
|**A01_1**|Local File Encryption|plaintext files → encrypted files|`CryptAcquireContext`, `CryptCreateHash`, `CryptDeriveKey`, `CryptEncrypt`, file I/O (`CreateFile`, `ReadFile`, `WriteFile`)|AES encryption routines, key generation logic, targeted file extensions, file traversal loops, `.locked` extension strings|High-volume file read/write operations, sequential file renaming, creation of encrypted files, no outbound network activity|Encrypts victim files locally to deny access while preserving the encrypted data on disk|
|**A01_2**|Encryption + Ransom Note|encrypted files → victim notification|AES encryption routines, file I/O, `URLDownloadToFile`/`WinHTTP`, `SystemParametersInfo` (or equivalent wallpaper APIs)|Ransom note strings, wallpaper URL/path, encryption logic, image download routines|File encryption followed by ransom note creation or wallpaper modification, limited outbound HTTP activity to retrieve ransom image|Combines file encryption with visible victim notification to communicate ransom demands and coerce payment|
|**A01_3**|Recovery Inhibition|recovery mechanisms → disabled|`CreateProcess`, `ShellExecute`, `DeleteFile`, execution of `vssadmin`, `wbadmin`, `bcdedit`, `vssadmin delete shadows`|Hardcoded recovery-related commands, Volume Shadow Copy strings, backup deletion logic|Process creation of Windows recovery utilities, shadow copy deletion, backup removal, system configuration changes|Prevents restoration of encrypted files by disabling or removing native Windows recovery mechanisms|
|**A01_4**|Encryption Key Exfiltration|AES key → RSA protection → C2 upload|AES encryption, RSA (`RSACryptoServiceProvider`/CryptoAPI), `WinHttpSendRequest`, HTTP POST|Public key retrieval URL, RSA encryption routines, HTTP POST parameters (`aesencrypted`), C2 endpoint strings|HTTP GET for public key, HTTP POST containing RSA-encrypted AES key, subsequent file encryption and wallpaper update|Protects the encryption key using attacker-controlled public-key cryptography before transmitting it to a remote server, preventing local key recovery|
|**A01_5**|Double Extortion|victim files → staged archive → C2 upload → encrypted originals|File I/O, `CreateProcess` (7-Zip), `WinHttpSendRequest`, multipart/form-data upload, AES encryption|Staging directory paths, archive creation logic, multipart HTTP upload routines, targeted file extensions, C2 endpoint strings|Creation of temporary staging directory, generation of `.7z` archive, outbound HTTP file upload, cleanup of staging artifacts, subsequent encryption of original files|Steals victim data before encryption by staging, compressing, and exfiltrating selected files, modeling modern double-extortion ransomware behavior|


### Control Samples 

1. [`C#` Hidden-Tear variant](https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A01-Ransomware-Cryptor/A01_1)
	- local file encryption only
2. [`C++` Hidden-Tear variant](https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A01-Ransomware-Cryptor/A01_2)
	- local file encryption + coercion via ransomnote
3. [`C` Hidden-Tear variant](https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A01-Ransomware-Cryptor/A01_3) 
	- Encryption + Recovery Inhibition
		- local file encryption
		- enum/encryption of backup drive
		- shadow copy deletion
		- disable recovery mechanisms
		- interfere with backup services
4. [`C#` EDA2 variant](https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A01-Ransomware-Cryptor/A01_4)
	- local file encryption
	- encryption of generated symmetric key with RSA public key
	- exfil of encrypted key to C2
5. [`C++` EDA2 variant](https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A01-Ransomware-Cryptor/A01_5)
	- double extortion: 
		- exfil of files to C2 prior to encryption
		- local encryption 