A05_1b HTTPS Beacon with Command Polling

Summary
Performs recurring HTTPS-based command-and-control check-ins using the 
WinHTTP API. The sample initializes a C2 client, tests connectivity to 
/api/ping, then starts a dedicated beacon thread that periodically posts 
host metadata to /api/checkin. The check-in includes a generated agent 
identifier, system information, and sample identifier. The server response 
is parsed for benign lab commands, including noop, ping, and sleep|seconds.

The sample demonstrates interactive beaconing behavior commonly 
associated with command-and-control implants, including HTTPS 
transport, browser-like User-Agent usage, JSON-based check-ins, 
task polling, command parsing, result submission, retry behavior, 
and remotely adjustable beacon timing.

Payload Summary
No secondary executable payload is delivered or executed by this sample.
The payload activity is limited to controlled command polling and benign 
command handling. Supported commands return results only:

noop            -> SUCCESS|NOOP
ping            -> SUCCESS|PONG
sleep|seconds   -> Updates beacon interval, clamped between 5 & 300 seconds

Command execution results are posted back to /api/result as JSON.

To execute A05_1b_https_beacon_w_command_polling:
.\A05-Beaconing-C2-Networking\A05_1b\bin\A05_1b_https_beacon_w_command_polling.exe

#########################################################################

High-Level Beacon Flow:

1. Initialize C2 server URL as https://c2.lab.local
2. Generate an agent ID from hostname and username
3. Test HTTPS connectivity using /api/ping
4. Start beacon thread using CreateThread
5. POST system check-in data to /api/checkin
6. Parse JSON response for command and optional arguments
7. Process supported benign lab commands:
    noop
    ping
    sleep|seconds
8. POST command result JSON to /api/result
9. Sleep for the configured beacon interval
10. Repeat until beacon loop exits

#########################################################################

INetSim HTTPS Server Config for A05_1b

Create fake response files:

sudo nano /var/lib/inetsim/http/fakefiles/ping.json
{"status":"ok","message":"PONG"}

sudo nano /var/lib/inetsim/http/fakefiles/checkin.json
{"status":"ok","command":"noop","args":""}
{"command":"sleep","args":"15"}
{"command":"ping","args":""}

Create a response for result submissions:
sudo nano /var/lib/inetsim/http/fakefiles/result.json
{"status":"ok","message":"result received"}

Set ownership:
sudo chown inetsim:inetsim /var/lib/inetsim/http/fakefiles/ping.json
sudo chown inetsim:inetsim /var/lib/inetsim/http/fakefiles/checkin.json
sudo chown inetsim:inetsim /var/lib/inetsim/http/fakefiles/result.json

Add static fakefile mappings to /etc/inetsim/inetsim.conf:
https_static_fakefile /api/ping ping.json application/json
https_static_fakefile /api/checkin checkin.json application/json
https_static_fakefile /api/result result.json application/json

Restart inetsim:
sudo systemctl restart inetsim