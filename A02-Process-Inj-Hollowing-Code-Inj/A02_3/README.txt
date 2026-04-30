calc.asm:

1. Find where Kernel32 lives in memory (PEB walk).
2. Search Kernel32 to find the exact address of WinExec (the block you just wrote).
3. Set up the arguments ("calc.exe") and trigger the call.

calc.asm compiled in Kali:

// Compile to object code via Assembler
nasm -f win64 calc.asm -o calc.obj

// Link object code to executable via Linker
objcopy -O binary calc.obj calc.bin

// Convert the binary to shellcode
xxd -i calc.bin


A02_3_thread_hijack.c
