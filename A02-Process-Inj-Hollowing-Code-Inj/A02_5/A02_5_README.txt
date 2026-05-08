A02_5 Process Hollowing

Summary 
Creates a suspended legitimate Windows process, removes or 
overwrites its original executable image in memory, and replaces 
it with attacker-controlled code before resuming execution. 
This causes malicious code to execute while masquerading as a 
trusted Windows process.

Payload Summary
A02_5_loaded_exe.exe is a controlled marker executable used to 
prove successful process hollowing. Once execution is transferred to 
the injected image, the payload creates a file artifact and records 
execution context information as the contents of the file artifact
in C:\Users\Public to demonstrate that the hollowed process is 
executing attacker-supplied code rather than its original program image.

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

Note: target process must be command-line process (eg. cmd.exe)
