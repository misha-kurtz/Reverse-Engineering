
``` powershell
# To tear down all SMB connections
Restart-Service LanmanWorkstation

# Confirm no active SMB connections
Get-SmbConnection
```