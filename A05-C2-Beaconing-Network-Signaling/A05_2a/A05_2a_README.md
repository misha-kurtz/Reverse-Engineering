# A05_2a DNS Beacon Using getaddrinfo

## Summary

Generates a sequence of DNS hostnames and resolves them using
`getaddrinfo()`.

The sample demonstrates DNS beaconing behavior commonly associated
with command-and-control signaling, low-volume periodic callbacks,
domain-based host check-ins, and DNS-based telemetry channels.

Unlike HTTP or HTTPS beacon samples, this specimen does not send web
requests or submit HTTP payloads. Instead, it generates structured DNS
queries at fixed intervals and relies on DNS resolution activity as the
observable behavior.

Each query uses a predictable counter-based subdomain format:

```text
tick-0001.beacon01.lab.local
tick-0002.beacon01.lab.local
tick-0003.beacon01.lab.local
```

The sample performs a fixed number of DNS lookups and then exits
cleanly.

No payload download, command execution, persistence, credential theft,
or destructive functionality is performed.

---

## Beacon Summary

The beacon behavior is implemented as a DNS lookup loop.

The base domain is:

```text
beacon01.lab.local
```

The generated query format is:

```text
tick-<counter>.beacon01.lab.local
```

The timing behavior uses:

* Iterations: `10`
* Sleep interval: `15000` milliseconds
* Query type: IPv4 `A` record
* Resolver API: `getaddrinfo()`

Example generated hostname:

```text
tick-0001.beacon01.lab.local
```

The counter is formatted as a four-digit value using leading zeroes.

---

## To Execute A05_2a

Make sure the Windows analysis VM is configured to use the lab DNS
server, typically the INetSim host:

```text
192.168.67.5
```

Then run the DNS beacon sample directly:

```powershell
.\A05-Beaconing-Interactive-C2\A05_2a\bin\A05_2a_dns_beacon_getaddrinfo.exe
```

Successful execution should show DNS lookup attempts for generated
hostnames and then exit with:

```text
[*] Finished.
```

---

## Expected Beacon Artifacts

### DNS Queries

Example generated queries:

```text
tick-0001.beacon01.lab.local
tick-0002.beacon01.lab.local
tick-0003.beacon01.lab.local
tick-0004.beacon01.lab.local
tick-0005.beacon01.lab.local
```

### Expected Console Output

```text
[*] Starting A05_2a DNS beacon...
[*] Base domain: beacon01.lab.local
[*] Iterations: 10
[*] Sleep: 15000 ms
[*] Query 1: tick-0001.beacon01.lab.local -> resolved
[*] Query 2: tick-0002.beacon01.lab.local -> resolved
[*] Finished.
```

### Expected Dynamic Signals

Dynamic analysis tools may observe:

* Winsock initialization using `WSAStartup()`
* Repeated DNS queries
* Sequential subdomain pattern
* Fixed 15-second interval between lookups
* IPv4-focused resolution behavior
* No HTTP or HTTPS traffic
* No file creation
* No persistence behavior

---

## INetSim Configuration for A05_2a

No additional INetSim HTTP or HTTPS fakefile configuration is required
for this sample.

The sample only requires DNS resolution to be available inside the lab
network.

Make sure INetSim DNS is enabled and the Windows analysis VM is using
the INetSim server as its DNS resolver.

Expected lab DNS resolver:

```text
192.168.67.5
```

Expected INetSim DNS service:

```text
dns_53_tcp_udp
```

---

### Expected INetSim DNS Log Activity

Successful execution should produce DNS log entries in:

```text
/var/log/inetsim/service.log
```

Example:

```text
[dns_53_tcp_udp] recv: Query Type A, Class IN, Name tick-0001.beacon01.lab.local
[dns_53_tcp_udp] send: tick-0001.beacon01.lab.local 3600 IN A 192.168.67.5
[dns_53_tcp_udp] recv: Query Type A, Class IN, Name tick-0002.beacon01.lab.local
[dns_53_tcp_udp] send: tick-0002.beacon01.lab.local 3600 IN A 192.168.67.5
```

---

#########################################################################

# High-Level DNS Beacon Flow

1. Start the DNS beacon sample:

   ```text
   A05_2a_dns_beacon_getaddrinfo.exe
   ```

2. Initialize Winsock using:

   ```text
   WSAStartup()
   ```

3. Configure the base domain:

   ```text
   beacon01.lab.local
   ```

4. Configure loop parameters:

   ```text
   iterations = 10
   sleep_ms = 15000
   ```

5. Generate the first DNS query name:

   ```text
   tick-0001.beacon01.lab.local
   ```

6. Configure `addrinfo` hints for IPv4 resolution:

   ```text
   AF_INET
   ```

7. Perform the DNS lookup using:

   ```text
   getaddrinfo()
   ```

8. Free returned address information using:

   ```text
   freeaddrinfo()
   ```

9. Print whether the hostname resolved

10. Sleep for:

    ```text
    15000 ms
    ```

11. Repeat the DNS lookup sequence for 10 iterations

12. Clean up Winsock using:

    ```text
    WSACleanup()
    ```

13. Exit cleanly after the final DNS beacon.
