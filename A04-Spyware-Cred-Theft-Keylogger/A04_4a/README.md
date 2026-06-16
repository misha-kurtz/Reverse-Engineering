### Inetsim config for A04_4a

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
curl.exe http://c2.lab.local/api/v1/report

# Test HTTP POST request
curl.exe -X POST http://c2.lab.local/api/v1/report `
  -H "Content-Type: application/json" `
  -d "{\"test\":\"A04_4\"}"
```

``` bash
# Confirm expected HTTP traffic
sudo tail -f /var/log/inetsim/service.log

# View captured POST data
sudo cat /var/lib/inetsim/http/postdata/*
```

``` bash
# To delete old POST data
sudo find /var/lib/inetsim/http/postdata -type f -delete
```