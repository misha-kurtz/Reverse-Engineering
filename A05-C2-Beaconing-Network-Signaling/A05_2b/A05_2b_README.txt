A05_2b DNS Beacon via DnsQuery_A

Summary

Performs periodic DNS beaconing using the Windows DNS 
API function DnsQuery_A to generate and resolve a sequence 
of controlled hostname queries. The sample constructs 
counter-based subdomains such as tick-0001.beacon02.lab.local, 
performs explicit DNS A-record lookups through the Windows DNS 
resolver subsystem, pauses for a fixed interval, and repeats for 
a fixed number of iterations.

This demonstrates controlled DNS signaling behavior commonly 
associated with malware beaconing, DNS-based command-and-control 
discovery, covert heartbeat traffic, and low-bandwidth network 
signaling techniques that rely on repeated DNS queries as the 
observable communication mechanism.

Payload Summary
No secondary payload is delivered or executed by this sample.

To execute A05_2b_DnsQueryA_dns_signal:
.\A05-Beaconing-C2-Networking\A05_2b\bin\A05_2b_DnsQueryA_dns_signal.exe

#########################################################################

High-Level Beacon Flow:

1. Initialize Winsock using WSAStartup
2. Configure base domain as beacon02.lab.local
3. Generate sequential DNS query names:
    tick-0001.beacon02.lab.local
    tick-0002.beacon02.lab.local
    tick-0003.beacon02.lab.local
4. Perform DNS A-record lookup using DnsQuery_A
5. Free DNS record structures using DnsRecordListFree
6. Sleep for 15 seconds
7. Repeat for 10 total iterations
8. Clean up Winsock using WSACleanup

#########################################################################

No additional Inetsim configuration beyond the initial DNS config.
No fakefiles required to generate DNS responses. 