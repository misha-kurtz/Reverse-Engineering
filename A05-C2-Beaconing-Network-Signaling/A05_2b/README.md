# A05_2b DNS Beacon Using DnsQuery_A

## Summary

Generates a sequence of DNS hostnames and resolves them using the
Windows DNS API function:

```text
DnsQuery_A()
```

The sample demonstrates DNS beaconing behavior commonly associated
with command-and-control signaling, DNS-based telemetry, periodic
callback mechanisms, and low-noise network communication channels.

Unlike `A05_2a`, which uses the higher-level Winsock resolver API
`getaddrinfo()`, this specimen interacts directly with the Windows DNS
resolver subsystem through the native DNS API.

The sample generates structured DNS queries at fixed intervals using
a counter-based subdomain naming scheme.

Example generated queries:

```text
tick-0001.beacon02.lab.local
tick-0002.beacon02.lab.local
tick-0003.beacon02.lab.local
```

The sample performs a fixed number of DNS lookups and exits cleanly.

No HTTP traffic, payload download, command execution, persistence,
credential theft, or destructive functionality is performed.

---

## Beacon Summary

The beacon behavior is implemented as a repeated DNS A-record lookup
loop.

The base domain is:

```text
beacon02.lab.local
```

The generated query format is:

```text
tick-<counter>.beacon02.lab.local
```

The timing behavior uses:

* Iterations: `10`
* Sleep interval: `15000` milliseconds
* Query type: `DNS_TYPE_A`
* Resolver API: `DnsQuery_A()`

Example generated hostname:

```text
tick-0001.beacon02.lab.local
```

The counter is formatted as a four-digit value using leading zeroes.

---

## To Execute A05_2b

Make sure the Windows analysis VM is configured to use the lab DNS
resolver, typically the INetSim server:

```text
192.168.67.5
```

Then run the DNS beacon sample directly:

```powershell
.\A05-Beaconing-Interactive-C2\A05_2b\bin\A05_2b_dns_beacon_DnsQuery_A.exe
```

Successful execution should show repeated DNS lookup attempts followed
by:

```text
[*] Finished.
```

---

## Expected Beacon Artifacts

### DNS Queries

Example generated queries:

```text
tick-0001.beacon02.lab.local
tick-0002.beacon02.lab.local
tick-0003.beacon02.lab.local
tick-0004.beacon02.lab.local
tick-0005.beacon02.lab.local
```

### Expected Console Output

```text
[*] Starting A05_2b DNS beacon...
[*] Base domain: beacon02.lab.local
[*] Query 1: tick-0001.beacon02.lab.local -> resolved
[*] Query 2: tick-0002.beacon02.lab.local -> resolved
[*] Finished.
```

### Expected Dynamic Signals

Dynamic analysis tools may observe:

* Winsock initialization using `WSAStartup()`

* Direct Windows DNS API usage

* Repeated DNS A-record queries

* Sequential subdomain generation

* Fixed 15-second interval between queries

* Calls to:

  ```text
  DnsQuery_A()
  DnsRecordListFree()
  ```

* No HTTP or HTTPS traffic

* No file creation

* No persistence behavior

---

## INetSim Configuration for A05_2b

No additional INetSim configuration beyond the initial DNS setup is
required for this sample.

No fakefiles are required to generate DNS responses.

The sample only requires DNS resolution services to be available
inside the isolated lab network.

Make sure the Windows analysis VM is configured to use the INetSim
server as its DNS resolver.

Expected resolver:

```text
192.168.67.5
```

Expected INetSim DNS service:

```text
dns_53_tcp_udp
```

---

### Expected INetSim DNS Log Activity

Successful execution should generate DNS activity inside:

```text
/var/log/inetsim/service.log
```

Example:

```text
[dns_53_tcp_udp] recv: Query Type A, Class IN, Name tick-0001.beacon02.lab.local
[dns_53_tcp_udp] send: tick-0001.beacon02.lab.local 3600 IN A 192.168.67.5

[dns_53_tcp_udp] recv: Query Type A, Class IN, Name tick-0002.beacon02.lab.local
[dns_53_tcp_udp] send: tick-0002.beacon02.lab.local 3600 IN A 192.168.67.5
```

---

#########################################################################

# High-Level DNS Beacon Flow

1. Start the DNS beacon sample:

   ```text
   A05_2b_dns_beacon_DnsQuery_A.exe
   ```

2. Initialize Winsock using:

   ```text
   WSAStartup()
   ```

3. Configure the base domain:

   ```text
   beacon02.lab.local
   ```

4. Configure loop parameters:

   ```text
   iterations = 10
   sleep_ms = 15000
   ```

5. Generate the first DNS query name:

   ```text
   tick-0001.beacon02.lab.local
   ```

6. Perform a DNS A-record lookup using:

   ```text
   DnsQuery_A()
   ```

7. Receive DNS results through the Windows DNS API

8. Free returned DNS records using:

   ```text
   DnsRecordListFree()
   ```

9. Print whether the hostname resolved successfully

10. Sleep for:

    ```text
    15000 ms
    ```

11. Repeat the lookup sequence for 10 iterations

12. Clean up Winsock using:

    ```text
    WSACleanup()
    ```

13. Exit cleanly after the final DNS beacon iteration.
