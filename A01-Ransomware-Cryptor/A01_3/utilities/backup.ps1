$Source = "C:\Users\Public\A01_TestData\"
$Destination = "Z:\Backups\A01_TestData_Backup\"
$LogDir = "C:\ProgramData\BackupLogs"
$LogFile = "$LogDir\A01_TestData_Backup.log"

# Ensure the log directory exists before running Robocopy
if (-not (Test-Path -Path $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
}

# Execute Robocopy
robocopy $Source $Destination /MIR /COPYALL /ZB /R:3 /W:5 /LOG:$LogFile /TEE