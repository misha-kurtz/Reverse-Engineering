A02_4 Asynchronous Procedure Call Injection

Summary
Injects code into a target process by queuing an 
Asynchronous Procedure Call (APC) to a target thread. 
The queued payload executes only when the target thread 
enters an alertable state, allowing attacker-controlled 
code to run without immediately creating a new execution 
thread. This technique leverages normal Windows asynchronous 
execution mechanisms to trigger malicious code within the 
target process context.

Payload Summary
A02_4_marker.dll is a controlled marker DLL used to verify 
successful APC-based code execution. When the APC-triggered 
DLL load succeeds, the DLL executes through DllMain, spawns a 
worker thread, and creates a file artifact at C:\Users\Public. 
The marker records the timestamp, process ID, and process path 
of the host process, proving that the injected DLL executed 
within the target process context after APC delivery.

To execute A02_4_APC_injector.exe:

1. Run the target process: A02_4_target_process.exe
2. Execute the injector from the command line:

.\A02_4_APC_injector.exe [target process] [malicious DLL]

.\A02_4_APC_injector.exe A02_4_target_process C:\Users\misha.kurtz\Reverse-Engineering\A02-Process-Inj-Hollowing-Code-Inj\A02_4\bin\A02_4_marker.dll