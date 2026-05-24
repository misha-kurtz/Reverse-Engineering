
# Malware Reverse Engineering RAG Pipeline

## Project Objective

A retrieval-augmented malware reverse-engineering assistant that combines static and dynamic analysis artifacts to improve semantic recovery from binaries.

The goal of this project is to generate high-level, semantically enriched reconstructions of malicious program behavior that are more understandable and actionable for reverse engineers than baseline decompiler output alone.


## Malware Behavior Classes

- **A01. Ransomware / Cryptor**
- **A02. Process Injection / Hollowing / Code injection**
- **A03. Dropper / Loader / Stager**
- **A04. Credential theft / Spyware / Keylogger**
- **A05. Beaconing / Interactive C2**
- **A06. Backdoor / Persistence / Service-Foothold**

## Dataset Design

The research uses a two-dataset design to balance semantic clarity with real-world realism.

| Dataset|Size|Goal|Characteristics|
|---|---|---|---|
|**Dataset A (Controlled)**|24-32 binaries|Ground truth and semantic validation|Clean, intentionally constructed behavior|
|**Dataset B (Wild)**|48–96 binaries|External validity and realism| Noisy, diverse, real-world specimens|


## Research Goals

This project aims to answer several core questions:

- Can RAG improve semantic recovery from malware binaries?
- Does combining static and dynamic analysis improve reconstruction quality?
- Can behavioral similarity retrieval improve reverse engineering workflows?
- How effectively can malware intent be reconstructed from artifacts alone?

## Planned Pipeline Components

### Static Analysis
- Ghidra Headless
- Decompiled functions
- P-Code extraction
- CFG generation
- Import analysis
- String extraction

### Dynamic Analysis
- Procmon / Noriben
- Sysmon
- Regshot
- Wireshark / PCAP
- INetSim telemetry
- Process and network monitoring

### Storage & Retrieval
- MinIO object storage
- SHA256-indexed artifact organization
- Embedding pipeline for semantic retrieval
- Metadata-driven sample classification

### Semantic Reconstruction
- Retrieval-Augmented Generation (RAG)
- Behavior-aware context retrieval
- Semantic labeling of recovered components
- High-level behavioral summarization

