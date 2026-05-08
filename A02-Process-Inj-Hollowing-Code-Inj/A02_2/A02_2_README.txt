A02_2 Remote Thread Injection

Summary
Injects raw shellcode into a running target process by 
allocating memory inside the remote process, copying 
position-independent machine code into that memory region, 
changing the memory permissions to executable, and starting 
execution through a remotely created thread. Unlike DLL injection, 
this technique bypasses the Windows loader and executes directly 
from attacker-controlled memory.

Payload Summary
The payload consists of 64-bit msfvenom shellcode generated with:
msfvenom -p windows/x64/exec CMD=calc.exe EXITFUNC=thread -f c
The shellcode is embedded directly within the injector executable as 
a raw byte array. Once injected into the target process and executed 
via CreateRemoteThread, the payload launches calc.exe as a controlled 
proof of successful remote code execution.

To generate payload shellcode
msfvenom -p windows/x64/exec CMD=calc.exe EXITFUNC=thread -f c

To execute remote_thread_inject.exe:

.\remote_thread_inject.exe <target_process>

.\remote_thread_inject.exe Notepad.exe



