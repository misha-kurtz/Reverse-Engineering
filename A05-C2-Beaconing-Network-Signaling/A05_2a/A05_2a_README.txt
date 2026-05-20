A05_2a DNS Beacon via getaddrinfo

Summary

Performs periodic DNS beaconing by generating a sequence of unique 
hostname queries and resolving them with the Winsock getaddrinfo API. 
Each query uses a predictable counter-based subdomain format such as 
tick-0001.beacon01.lab.local, waits for a fixed interval, and repeats 
for a fixed number of iterations.

This demonstrates controlled DNS beaconing behavior commonly associated 
with malware implants, command-and-control discovery, heartbeat signaling,
and low-bandwidth network check-ins where DNS queries are used as the 
observable communication channel.

Payload Summary
No secondary payload is delivered or executed by this sample.

To execute A05_2a_dns_beacon:
.\A05-Beaconing-C2-Networking\A05_2a\bin\A05_2a_dns_beacon.exe

#########################################################################

High-Level Beacon Flow:

1. Initialize Winsock using WSAStartup
2. Configure base domain as beacon01.lab.local
3. Generate sequential DNS query names:
    tick-0001.beacon01.lab.local
    tick-0002.beacon01.lab.local
    tick-0003.beacon01.lab.local
4. Perform IPv4 DNS lookup using getaddrinfo
5. Free resolver results with freeaddrinfo
6. Sleep for 15 seconds
7. Repeat for 10 total iterations
8. Clean up Winsock using WSACleanup


#########################################################################

No additional Inetsim configuration beyond the initial DNS config.
No fakefiles required to generate DNS responses. 
