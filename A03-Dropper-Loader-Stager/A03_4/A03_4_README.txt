A03_4 Obfuscated Fileless Shellcode Loader

Summary
Decodes an embedded XOR-obfuscated and Base64-encoded 
payload entirely in memory, allocates executable memory within 
the current process, stages the decoded payload into the allocated
region, changes memory protections from writable to executable, 
and launches the payload using a new thread. This demonstrates 
fileless staged loader behavior commonly associated with malware 
loaders and shellcode runners that attempt to conceal embedded 
payloads from static analysis prior to runtime execution.

Payload Summary
The embedded payload is a handcrafted 64-bit Windows shellcode 
routine originally authored in x64 assembly (calc.asm). Prior 
to embedding, the raw shellcode bytes are XOR-obfuscated using 
key 0x5A and Base64-encoded. 

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

#########################################################################
To generate obfuscated blob from the shellcode 
run place the shellcode in the generator code and 
execute via command-line:

cd A03-Dropper-Loader-Stager\A03_4_Obfuscated_Loader\generator
dotnet run

Place encoded blob in A03_4_obfuscated_loader.cs 
prior to compiling to exeutable.
