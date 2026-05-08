A02_1 DLL Injection 

Summary
Injects an external DLL into a running target process 
by allocating memory in the remote process, writing the 
DLL path into that memory region, and creating a remote 
thread that invokes LoadLibraryA. This causes the target 
process to load and execute attacker-controlled DLL code 
within its own process context while continuing to 
appear as a legitimate application.

Payload Summary
A02_1_marker.dll is a controlled marker DLL used to verify 
successful DLL injection and remote code execution. When 
loaded by the target process, the DLL executes through DllMain, 
spawns a worker thread, and creates a file artifact at 
C:\Users\Public. The marker records the timestamp, process ID, 
and process path of the host process, proving that the injected 
DLL executed inside the target process context rather than the 
injector process itself.


To execute A02_1_dll_injector.exe:

1. Execute target process: A02_1_target_process.exe
2. Execute the DLL injector: A02_1_dll_injector.exe
3. A01_2_dll_injector.exe injects custom DLL into target process
4. Upon execution of DLL, a file artifact is written to C:\Users\Public



