
# Malware Reverse Engineering RAG Pipeline

### Project Objective

A retrieval-augmented malware reverse-engineering assistant that combines static and dynamic analysis artifacts to improve semantic recovery from binaries.

The goal of this project is to generate high-level, semantically enriched reconstructions of malicious program behavior that are more understandable and actionable for reverse engineers than baseline decompiler output alone.


### Malware Behavior Classes

- **A01. Ransomware / Cryptor**
- **A02. Process Injection / Hollowing / Code injection**
- **A03. Dropper / Loader / Stager**
- **A04. Credential theft / Spyware / Keylogger**
- **A05. Beaconing / Interactive C2**
- **A06. Backdoor / Persistence / Service-Foothold**

### Dataset Design

The research uses a two-dataset design to balance semantic clarity with real-world realism.

| Dataset|Goal|Characteristics|
|---|---|---|
|**Dataset A (Controlled)**|Ground truth and semantic validation|Clean, intentionally constructed behavior|
| **Dataset B (Wild)**|External validity and realism| Noisy, diverse, real-world specimens|


##### Dataset A: Controlled specimens

**Size**
24-32 binaries

These are intentionally selected or self-compiled behavior-controlled specimens.

Purpose:

- establish behavioral ground truth
- evaluate semantic reconstruction against known source/intent
- test whether RAG recovers meaning better than baseline

##### Dataset B: Wild/real-world specimens

**Size**
48–96 binaries

These consist of real malware or malware-like samples obtained from public research sources and curated repositories.

Purpose:
- show external validity
- demonstrate pipeline scalability
- test realistic noisy artifacts

Introduce:
- Python
- PowerShell
- .NET (heavier frameworks)
- Packed binaries
- Mixed techniques

### Research Goals

This project aims to answer several core questions:

- Can RAG improve semantic recovery from malware binaries?
- Does combining static and dynamic analysis improve reconstruction quality?
- Can behavioral similarity retrieval improve reverse engineering workflows?
- How effectively can malware intent be reconstructed from artifacts alone?

### Planned Pipeline Components

##### Static Analysis
Ghidra Headless
Decompiled functions
P-Code extraction
CFG generation
Import analysis
String extraction

##### Dynamic Analysis
Procmon / Noriben
Sysmon
Regshot
Wireshark / PCAP
INetSim telemetry
Process and network monitoring

##### Storage & Retrieval
MinIO object storage
SHA256-indexed artifact organization
Embedding pipeline for semantic retrieval
Metadata-driven sample classification

##### Semantic Reconstruction
Retrieval-Augmented Generation (RAG)
Behavior-aware context retrieval
Semantic labeling of recovered components
High-level behavioral summarization

**Disclaimer**

This project is intended strictly for:
- Academic research
- Defensive cybersecurity research
- Malware analysis education
- Controlled laboratory experimentation

All testing should be performed in isolated environments using properly secured virtual infrastructure.