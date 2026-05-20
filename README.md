# Project Objective
- A retrieval-augmented malware reverse-engineering assistant that combines static and dynamic artifacts to improve semantic recovery from binaries.
- Generates high-level, semantically enriched reconstruction of malicious program components that is more understandable and more actionable for reverse engineers than baseline decompiler output.

### Malware Behavior Classes

- **A01. Ransomware / Cryptor**
- **A02. Process Injection / Hollowing / Code injection**
- **A03. Dropper / Loader / Stager**
- **A04. Credential theft / Spyware / Keylogger**
- **A05. Beaconing / Interactive C2**
- **A06. Backdoor / Persistence / Service-Foothold**

# Two-dataset design: Control versus Realism

|Dataset|Goal|What you want|
|---|---|---|
|**A (Controlled)**|Ground truth, semantic clarity|Clean, direct behavior|
|**B (Wild)**|Real-world validity|Noise, abstraction, diversity|


## Dataset A: Controlled specimens

**24-32 binaries**

These are intentionally selected or self-compiled behavior-controlled specimens.

Purpose:

- establish ground truth
- evaluate semantic reconstruction against known source/intent
- test whether RAG recovers meaning better than baseline

## Dataset B: Wild/real-world specimens

**48–96 binaries**

These are real malware or malware-like specimens from public research sources or carefully curated repositories.

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
