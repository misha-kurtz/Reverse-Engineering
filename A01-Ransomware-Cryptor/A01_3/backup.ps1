$Source = "C:\Users\Public\A01_TestData"
$Destination = "Z:\Backups\A01_TestData_Backup" # Updated to Z:
$LogFile = "C:\ProgramData\BackupLogs\A01_TestData_Backup.log"

robocopy $Source $Destination /MIR /COPYALL /ZB /R:3 /W:5 /LOG:$LogFile /TEE