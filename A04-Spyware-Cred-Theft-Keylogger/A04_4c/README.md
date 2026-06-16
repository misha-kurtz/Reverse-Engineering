### Inetsim config for A04_4b

``` bash
sudo nano /etc/inetsim/inetsim.conf

service_bind_address 192.168.67.5

start_service dns
start_service http

dns_default_ip 192.168.67.5
dns_static c2.lab.local 192.168.67.5
```

``` bash
# Config standard HTTP response 
sudo nano /var/lib/inetsim/http/fakefiles/report_OK.json
{"status":"ok","task":"telemetry accepted"}

http_static_fakefile /api/v1/report report_OK.json application/json
```

``` powershell
# Test standard HTTP response
curl.exe -k http://c2.lab.local/api/v1/report

# Test HTTP POST request
curl.exe -k -X POST https://c2.lab.local/api/v1/report `
  -H "Content-Type: text/plain" `
  -d "eyJ1c2VybmFtZSI6Im1pc2hhLmt1cnR6IiwiY29tcHV0ZXJfbmFtZSI6IldJTl9WTSIsInN0YXR1cyI6ImxvY2F0aW9uX2hlYXJ0YmVhdCIsInRpbWUiOiIyMDI2LTA2LTE2IDEyOjA0OjMzIn0="
```

``` bash
# Confirm expected HTTPS traffic
sudo tail -f /var/log/inetsim/service.log

# View captured POST data
sudo cat /var/lib/inetsim/http/postdata/*

# Decode captured POST data
echo "eyJ1c2VybmFtZSI6Im1pc2hhLmt1cnR6IiwiY29tcHV0ZXJfbmFtZSI6IldJTl9WTSIsInN0YXR1cyI6ImxvY2F0aW9uX2hlYXJ0YmVhdCIsInRpbWUiOiIyMDI2LTA2LTE2IDEyOjA0OjMzIn0=" | base64 -d
{"username":"misha.kurtz","computer_name":"WIN_VM","status":"location_heartbeat","time":"2026-06-16 12:04:33"}
```

``` bash
# To delete old POST data
sudo find /var/lib/inetsim/http/postdata -type f -delete
```