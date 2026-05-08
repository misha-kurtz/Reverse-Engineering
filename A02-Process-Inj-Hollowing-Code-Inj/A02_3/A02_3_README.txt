A02_3 Thread Hijacking

Summary
Hijacks the execution flow of an existing suspended thread 
by modifying its CPU execution context and redirecting the 
instruction pointer (RIP) to attacker-controlled shellcode 
stored in executable memory. Unlike remote thread injection, 
this technique does not create a new execution thread and instead 
repurposes an existing thread to execute malicious code.

Payload Summary
The payload consists of custom x64 assembly compiled into raw 
shellcode. The shellcode dynamically locates kernel32.dll in 
memory, resolves the address of WinExec by manually parsing the 
PE export table, and launches calc.exe. The payload is fully 
position-independent and does not rely on the Windows loader or 
imported API tables.

To execute A02_3_thread_hijack.exe from command-line with no arguments
.\A02-Process-Inj-Hollowing-Code-Inj\A02_3\bin\A02_3_thread_hijack.exe

#########################################################################
calc.asm high-level payload flow:
1. Locate the Process Environment Block (PEB)
2. Walk loaded module structures to locate kernel32.dll
3. Parse the PE export table to resolve WinExec
4. Build the "calc.exe" argument on the stack
5. Invoke WinExec("calc.exe", SW_SHOWNORMAL)

Compile calc.asm and convert to shellcode in Kali:

// Compile to object code via Assembler
nasm -f win64 calc.asm -o calc.obj

// Link object code to executable via Linker
objcopy -O binary calc.obj calc.bin

// Convert the binary to shellcode
xxd -i calc.bin



