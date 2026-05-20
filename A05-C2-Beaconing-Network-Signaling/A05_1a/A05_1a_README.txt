A05_1a HTTP Beacon

Summary

Performs periodic outbound HTTP beaconing to a remote command-and-control-style
endpoint using the .NET WebClient API. The beacon transmits basic host
metadata including the machine name, username, sequence counter, and a UNIX
timestamp through HTTP GET parameters while impersonating a legitimate browser
User-Agent string. The beacon loop incorporates randomized sleep jitter between
requests in order to vary network timing behavior and reduce the predictability
of beacon intervals. This demonstrates controlled HTTP beaconing behavior
commonly associated with malware implants, remote access trojans (RATs), and
command-and-control frameworks that periodically communicate with external
infrastructure to signal host availability and retrieve tasking.

Payload Summary
No secondary payload is delivered or executed by this sample.

To execute A05_1a_http_beacon:
.\A05-Beaconing-C2-Networking\A05_1a\bin\A05_1a_http_beacon.exe

#########################################################################

High-Level Beacon Flow

1. Configure HTTP beacon URI
2. Configure beacon sleep interval and jitter percentage
3. Configure browser-like User-Agent string
4. Build HTTP GET beacon request containing:
    Beacon ID
    Hostname
    Username
    Sequence counter
    UNIX timestamp
5. Send outbound HTTP request using WebClient
6. Receive HTTP response from server
7. Sleep for randomized jitter interval
8. Repeat beacon loop until max beacon count reached

#########################################################################

INetSim HTTP Server Config for A05_1a

Add static fakefile mapping to:
/etc/inetsim/inetsim.conf

http_static_fakefile /beacon beacon.txt text/plain

Create fake response file:
sudo nano /var/lib/inetsim/http/fakefiles/beacon.txt
sudo chown inetsim:inetsim /var/lib/inetsim/http/fakefiles/beacon.txt

Restart inetsim:
sudo systemctl restart inetsim