
``` powershell
# To tear down all SMB connections
Restart-Service LanmanWorkstation

# Confirm no active SMB connections
Get-SmbConnection
```

``` bash
# Check SMB share for dropped .lz file
sudo ls /srv/sharestage/

# Decompress .lz file to retrieve contents
python3 -c "import zlib; print(zlib.decompress(open('/srv/sharestage/report.lz', 'rb').read()[2:], -15).decode('utf-8'))"
{"username":"misha.kurtz","computer_name":"WIN_VM","status":"location_heartbeat","time":"2026-06-16 14:37:06"}

```