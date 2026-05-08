A03_3 In-memory Fileless Shellcode Loader

Summary
Loads an embedded shellcode payload directly into the 
current process’s memory, marks that memory as executable, 
and runs it using a newly created local thread. This models 
a fileless loader pattern where the second-stage payload is 
never written to disk as a separate executable.

Payload Summary
The payload is custom x64 assembly shellcode designed to 
launch calc.exe. It is embedded in the loader as a raw 
byte array, copied into dynamically allocated memory, 
and executed from that memory region.

To execute A03_3_in_mem_shellcode_loader.exe:
.\A03-Dropper-Loader-Stager\A03_3_In_Mem_Fileless_Loader\bin\A03_3_in_mem_shellcode_loader.exe

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