To run A02_5_process_hollowing.exe:

From PowerShell:

.\A02_5_process_hollowing.exe [payload exe] [target process]

.\A02_5_process_hollowing.exe \
C:\Users\misha.kurtz\Reverse-Engineering\A02-Process-Inj-Hollowing-Code-Inj\A02_5\bin\A02_5_loaded_exe.exe \
C:\Windows\System32\cmd.exe

1. Launcher runs
2. Launcher creates suspended cmd.exe
3. Payload image is mapped (A02_5_loaded_exe.exe)
4. Executed payload creates the marker file as cmd.exe (trusted process)
5. File artifact can be found at C:\Users\Public